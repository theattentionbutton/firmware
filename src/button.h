#include "captive_portal_server.h"
#include "cert.h"
#include "constants.h"
#include "depends/ESPAsyncWebSrv.h"
#include "littlefs_kv.h"
#include "utils.h"
#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
#include <MATRIX7219.h>
#include <WiFiClientSecure.h>
#define NetworkClientSecure WiFiClientSecure
X509List root_ca_x509(ROOT_CA_CERT);

#include "captive_portal.h"
#include "midis.h"

#define FETCH(variable, max_len, yes, no)                                      \
    kv_get(#variable, variable, max_len) ? yes : no

template <typename ResponseHandler>
int hit_server(const char *route, ResponseHandler handler, const char *username,
               const char *secret) {
    NetworkClientSecure *client = new NetworkClientSecure();
    int id = 0;
    if (!client) return id;
    client->setTrustAnchors(&root_ca_x509);
    {
        HTTPClient https;
        if (!https.begin(*client, route)) goto cleanup;
        char body[512] = {0};
        snprintf(body, 512 - 1, R"({"secret":"%63s", "username": "%254s"})",
                 secret, username);
        int code = https.POST(body);

        Serial.printf("server responded with %d\n", code);
        id = handler(https, code);
    }

cleanup:
    delete client;
    return id;
}

int response_action(HTTPClient &https, int code) {
    if (code != 200) return 0;
    String response = https.getString();
    JsonDocument doc;
    deserializeJson(doc, response);
    if (!doc["icon_id"].is<int>()) return 0;
    int id = doc["icon_id"];
    return id;
}

class AttentionButton {
    MATRIX7219 *mx;
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
        draw_icon(SMILEY);
        Serial.begin(BAUD_RATE);
        while (!Serial);
        Serial.print("\n[init] AttentionButton start...\n");
    }

    void draw_icon(IconId i) { ::draw_icon(i, mx); }

    void server_setup(const String &scan_results) {
        set_up_webserver(*server, scan_results, local_ip);
        server->begin();
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

        draw_icon(READY);
    }

    void mx_init() {
        mx->begin();
        mx->clear();
        mx->setBrightness(3);
        mx->setSwap(1);
        mx->setReverse(1);
    }

    void schedule_request() { should_post = true; }

    void request_attention(unsigned long now) {
        if (!should_post || (now - last_req_time) < MIN_ATTENTION_INTERVAL)
            return;
        last_req_time = now;
        should_post = 0;
        Serial.println("[client mode] requesting attention");
        int ok = hit_server(
            ROUTE("/request-attention"),
            [](HTTPClient &https, int code) { return code == 200; }, username,
            secret);
    }

    void poll(unsigned long now) {
        if (now - last_poll_time < POLL_DELAY) return;
        last_poll_time = now;
        Serial.println("[client mode] querying for messages");
        int icon = hit_server(ROUTE("/list-messages"), response_action,
                              username, secret);
    }

    void setup_iter() {
        draw_icon(SETTINGS);
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
};