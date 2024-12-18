#include "captive_portal.h"
#include "captive_portal_server.h"
#include "cert.h"
#include "constants.h"
#include "littlefs_kv.h"
#include "midis.h"
#include "utils.h"
#include <Crypto.h>
#include <DNSServer.h>
#include <ESP8266MQTTClient.h>
#include <ESPAsyncWebSrv.h>
#include <MATRIX7219.h>
#include <WiFiClientSecure.h>

X509List root_ca_x509(ROOT_CA_CERT);

#define FETCH(variable, max_len, yes, no)                                      \
    kv_get(#variable, variable, max_len) ? yes : no

String sha256_to_hex(const uint8_t *hash) {
    const char *hex = "0123456789abcdef";
    char buf[64] = {0};
    for (int i = 0; i < 32; i++) {
        buf[2 * i] = hex[hash[i] >> 4];
        buf[2 * i + 1] = hex[hash[i] & 0xf];
    }
    return String(buf);
}

class AttentionButton {
    MATRIX7219 *mx;
    MQTTClient *mqtt = NULL;
    AsyncWebServer *server;
    unsigned long last_req_time = 0;
    bool was_ap_setup = false;
    bool request_pending = false;
    bool client_wifi_connected = false;
    char ssid[64] = {0};
    char psk[64] = {0};
    char secret[64] = {0};
    char username[255] = {0};
    char topic[128] = {0};

  public:
    DNSServer *dns;
    EButtonMode mode = CLIENT_MODE;
    bool begun = false;
    bool playing = false;
    const MidiTrack *current_track = NULL;

    AttentionButton() {
        mx = new MATRIX7219(MATRIX_DAT, MATRIX_SEL, MATRIX_CLK, MATRIX_CNT);
        mx_init();
        draw_icon("SMILEY");
        Serial.begin(BAUD_RATE);
        while (!Serial);
        Serial.print("\n[init] AttentionButton start...\n");
    }

    void draw_icon(const char *name) {
        int idx = icon_idx(name);
        IconId i = (idx != 0) ? (IconId)idx : EXCLAMATION;
        ::draw_icon(i, mx);
    }

    void server_setup(const String &scan_results) {
        set_up_webserver(*server, scan_results, local_ip);
        server->begin();
    }

    void mqtt_connect(const char *topic) {
        if (!mqtt) mqtt = new MQTTClient();
        mqtt->onSecure([](WiFiClientSecure *client, String host) {
            Serial.printf("Secure: %s\r\n", host.c_str());
            client->setTrustAnchors(&root_ca_x509);
            return client;
        });

        mqtt->onData([this](String t, String data, bool cont) {

        });

        mqtt->onSubscribe([this, topic](int sub_id) {
            Serial.printf("Subscribe topic id: %d ok\r\n", sub_id);
            mqtt->publish(topic, "qos0", 0, 0);
        });

        mqtt->onConnect([this, topic]() {
            Serial.printf("MQTT: Connected\r\n");
            mqtt->subscribe(topic, 0);
        });

        mqtt->begin("mqtts://" MQTT_BROKER ":" MQTT_PORT);
    }

    int setup_wifi(char *ssid, char *psk) {
        if (client_wifi_connected) return 1;
        WiFi.begin(ssid, psk);
        int result = WiFi.waitForConnectResult();
        if (result != WL_CONNECTED) return 0;
        set_clock();
        return client_wifi_connected = 1;
    }

    void begin_setup_mode(bool override = false) {
        mode = SETUP_MODE;
        if (begun && !override) return;
        Serial.println(override
                           ? "[init] Credentials not set, defaulting to setup"
                           : "[init] Entering setup mode...");
        begun = true;
        server = new AsyncWebServer(80);
        dns = new DNSServer();
    }

    void begin_client_mode() {
        mode = CLIENT_MODE;
        if (begun) return;
        Serial.println("[init] Entering client mode...");
        begun = true;

        mode = FETCH(ssid, 63, CLIENT_MODE, SETUP_MODE);
        mode = FETCH(psk, 63, CLIENT_MODE, SETUP_MODE);
        mode = FETCH(secret, 63, CLIENT_MODE, SETUP_MODE);
        mode = FETCH(username, 254, CLIENT_MODE, SETUP_MODE);

        if (mode == SETUP_MODE) return begin_setup_mode(true);

        if (!client_wifi_connected && !setup_wifi(ssid, psk)) {
            fatal_error(CONNECTION_ERROR, mx);
        }

        SHA256 hasher;
        hasher.doUpdate(secret);
        uint8_t hash[SHA256_SIZE];
        hasher.doFinal(hash);
        String s = sha256_to_hex(hash);

        char buf[128] = {0};

        snprintf(buf, sizeof(buf), "attnbtn/messages/%.*s", 64, s.c_str());

        Serial.printf("[debug] topic id %s\n", buf);
        strncpy(topic, buf, 127);
        mqtt_connect(buf);
        draw_icon("READY");
    }

    void mx_init() {
        mx->begin();
        mx->clear();
        mx->setBrightness(3);
        mx->setSwap(1);
        mx->setReverse(1);
    }

    void schedule_request() { request_pending = true; }

    void do_request(unsigned long now) {
        if (!request_pending || (now - last_req_time) < MIN_ATTENTION_INTERVAL)
            return;
        last_req_time = now;
        request_pending = 0;
        Serial.println("[client mode] requesting attention");
        mqtt->publish(topic, "testing attn req");
    }

    void setup_iter() {
        draw_icon("SETTINGS");
        if (!was_ap_setup) {
            was_ap_setup = 1;
            start_soft_ap(CAPTIVE_SSID, CAPTIVE_PWD, local_ip, gateway_ip);
            setup_dns_server(*dns, local_ip);
            String scan_results = scan_wifi_networks();
            Serial.printf("[setup mode] scan results: %s\n",
                          scan_results.c_str());
            server_setup(scan_results);

            return;
        }
        process_dns();
    }

    void play_track(const char *name) { play_track_by_name(name); }

    void process_dns() { dns->processNextRequest(); }

    void handle_mqtt() { mqtt->handle(); }
};