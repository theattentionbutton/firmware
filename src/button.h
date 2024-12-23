#include "captive_portal.h"
#include "captive_portal_server.h"
#include "cert.h"
#include "constants.h"
#include "littlefs_kv.h"
#include "midis.h"
#include "parse_payload_string.h"
#include "utils.h"
#include <Crypto.h>
#include <DNSServer.h>
#include <ESP8266MQTTClient.h>
#include <ESPAsyncWebSrv.h>
#include <MATRIX7219.h>
#include <WiFiClientSecure.h>

const char fingerprint[] =
    "5E D1 8B 32 7C BA EC A0 AB 29 7A 3A 45 C2 2F 79 1C 6F 4B BC";

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

typedef enum display_event_type_t {
    NONE = 0,
    ENC_INPUT,
    MESSAGE
} DisplayEventType;

int disp_event_delay(DisplayEventType t) {
    switch (t) {
        case ENC_INPUT:
            return 5000;
        case MESSAGE:
            return 30000;
    }

    return 0;
}

class AttentionButton {
    MATRIX7219 *mx;
    MQTTClient mqtt;
    AsyncWebServer *server;
    unsigned long last_req_time = 0;
    bool was_ap_setup = false;
    bool request_pending = false;
    bool client_wifi_connected = false;
    char ssid[64] = {0};
    char psk[64] = {0};
    char secret[64] = {0};
    char username[256] = {0};
    char ringtone[20] = "JUNE";
    char topic[128] = {0};

  public:
    DNSServer *dns;
    EButtonMode mode = CLIENT_MODE;
    bool begun = false;
    bool playing = false;
    bool disp_was_reset = true;
    unsigned long last_disp_event_time = 0;
    DisplayEventType last_disp_event_type = NONE;
    bool has_selected_icon = false;
    IconId current_selected_icon = EXCLAMATION;

    unsigned long last_message = 0;
    const MidiTrack *current_track = NULL;

    AttentionButton() {
        mx = new MATRIX7219(MATRIX_DAT, MATRIX_SEL, MATRIX_CLK, MATRIX_CNT);
        mx_init();
        draw_icon("LOADING");
        Serial.begin(BAUD_RATE);
        while (!Serial);
        Serial.print("\n[init] AttentionButton start...\n");
    }

    void draw_icon(const char *name) {
        int idx = icon_idx(name);
        IconId i = (idx != 0) ? (IconId)idx : EXCLAMATION;
        ::draw_icon(i, mx);
    }

    void draw_icon_by_id(IconId i) { ::draw_icon(i, mx); }

    void server_setup(const String &scan_results) {
        set_up_webserver(*server, scan_results, local_ip);
        server->begin();
    }

    void mx_set_all(bool on = true) {
        for (int i = 0; i < 8; i++) {
            mx->setRow(i + 1, on ? 255 : 0, 0);
        }
    }

    void message_received(const char *icon_name) {
        last_message = millis();
        for (int i = 0; i < 3; i++) {
            mx_set_all();
            delay(400);
            mx_set_all(false);
            delay(400);
        }
        draw_icon(icon_name);
        play_track(ringtone);
        disp_was_reset = false;
        last_disp_event_type = MESSAGE;
        last_disp_event_time = millis();
    }

    void mqtt_connect(const char *topic) {
        String t(topic);
        mqtt.onSecure([](WiFiClientSecure *client, String host) {
            Serial.printf("Secure: %s\r\n", host.c_str());
            return client->setFingerprint(fingerprint);
        });

        mqtt.onData([this](String topic, String data, bool cont) {
            char icon[32];
            char email[255];
            int result = parse_payload(data, icon, email);
            if (result == ERROR_TOO_LONG || result == ERROR_INVALID_FORMAT) {
                return;
            }
            if (strcmp(email, username) == 0) {
                Serial.println("[debug] got own message");
            } else {
                Serial.printf("[debug] iconid: %s, email: %s\n", icon, email);
                message_received(icon);
            }
        });

        mqtt.onSubscribe([this, t](int sub_id) {
            Serial.printf("[mqtt] Subscribe topic id: %d ok\r\n", sub_id);
            mqtt.publish(t, "qos0", 0, 0);
        });

        mqtt.onConnect([this, t]() {
            Serial.printf("[mqtt] connected to broker\r\n");
            mqtt.subscribe(t, 0);
            draw_icon("READY");
        });

        mqtt.begin("mqtts://" MQTT_BROKER ":" MQTT_PORT);
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

        char ringtone_buf[20] = {0};
        int len = kv_get("ringtone", ringtone_buf, 16);
        if (len) strncpy(ringtone, ringtone_buf, 16);

        SHA256 hasher;
        hasher.doUpdate(secret);
        uint8_t hash[SHA256_SIZE];
        hasher.doFinal(hash);
        String s = sha256_to_hex(hash);

        char buf[128] = {0};

        snprintf(buf, sizeof(buf), "attnbtn/messages/%.*s", 64, s.c_str());

        Serial.printf("[debug] topic id \"%s\"\n", buf);
        strncpy(topic, buf, 127);
        mqtt_connect(buf);
    }

    void mx_init() {
        mx->begin();
        mx->clear();
        mx->setBrightness(3);
        mx->setSwap(1);
        mx->setReverse(1);
    }

    void schedule_request() {
        if (has_selected_icon) request_pending = true;
    }

    void do_request(unsigned long now) {
        if (!request_pending || (now - last_req_time) < MIN_ATTENTION_INTERVAL)
            return;
        if (!has_selected_icon) return;
        last_req_time = now;
        request_pending = 0;
        Serial.println("[client mode] requesting attention");
        char buf[512] = {0};
        snprintf(buf, 512, "#%.*s#%.*s#", 20, icon_name(current_selected_icon),
                 254, username);
        Serial.printf("[debug] %s\n", buf);
        mqtt.publish(topic, buf);
        play_track_by_name("SUCCESS");
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

    void handle_mqtt() { mqtt.handle(); }
};