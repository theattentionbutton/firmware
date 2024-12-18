#include "captive_portal.h"
#include "captive_portal_server.h"
#include "cert.h"
#include "constants.h"
#include "depends/ESPAsyncWebSrv.h"
#include "littlefs_kv.h"
#include "midis.h"
#include "utils.h"
#include <DNSServer.h>
#include <ESP8266MQTTClient.h>
#include <MATRIX7219.h>
#include <WiFiClientSecure.h>

const char fingerprint[] =
    "5E D1 8B 32 7C BA EC A0 AB 29 7A 3A 45 C2 2F 79 1C 6F 4B BC";

X509List root_ca_x509(ROOT_CA_CERT);

#define FETCH(variable, max_len, yes, no)                                      \
    kv_get(#variable, variable, max_len) ? yes : no

class AttentionButton {
    MATRIX7219 *mx;
    MQTTClient *mqtt = NULL;
    AsyncWebServer *server;
    unsigned long last_button_press = 0;
    unsigned long last_poll_time = 0;
    unsigned long last_req_time = 0;
    bool was_ap_setup = false;
    bool should_post = false;
    bool client_wifi_connected = false;
    char ssid[64] = {0};
    char psk[64] = {0};
    char secret[64] = {0};
    char username[255] = {0};

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

    void mqtt_connect() {
        if (!mqtt) mqtt = new MQTTClient();
        mqtt->onSecure([](WiFiClientSecure *client, String host) {
            Serial.printf("Secure: %s\r\n", host.c_str());
            return client->setFingerprint(fingerprint);
        });

        mqtt->onData([this](String topic, String data, bool cont) {

        });

        mqtt->onSubscribe([this](int sub_id) {
            Serial.printf("Subscribe topic id: %d ok\r\n", sub_id);
            mqtt->publish("/attentionbutton/testing", "qos0", 0, 0);
        });

        mqtt->onConnect([this]() {
            Serial.printf("MQTT: Connected\r\n");
            mqtt->subscribe("/attentionbutton/testing", 0);
        });

        mqtt->begin("mqtts://mqtt-dashboard.com:8883");
    }

    int setup_wifi(char *ssid, char *psk) {
        if (client_wifi_connected) return 1;
        WiFi.begin(ssid, psk);
        int result = WiFi.waitForConnectResult();
        if (result != WL_CONNECTED) return 0;
        set_clock();
        mqtt_connect();
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

        draw_icon("READY");
    }

    void mx_init() {
        mx->begin();
        mx->clear();
        mx->setBrightness(3);
        mx->setSwap(1);
        mx->setReverse(1);
    }

    void schedule_request() { should_post = true; }

    void do_request(unsigned long now) {
        if (!should_post || (now - last_req_time) < MIN_ATTENTION_INTERVAL)
            return;
        last_req_time = now;
        should_post = 0;
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