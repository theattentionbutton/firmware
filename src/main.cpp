#include "captive_portal.h"
#include "captive_portal_server.h"
#include "cert.h"
#include "utils.h"
#include <DNSServer.h>
#include <MATRIX7219.h>

#if defined(ESP32)

#include <HTTPClient.h>
#define FORMAT_LITTLEFS_ON_ERR true

#elif defined(ESP8266)

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#define NetworkClientSecure WiFiClientSecure
#define FORMAT_LITTLEFS_ON_ERR

#endif

#define MATRIX_CNT 1 // matrix count

#define ENC_DAT 14
#define ENC_CLK 10
#define ENC_SW 13
#define BZ1 12

#if defined(ESP32)
#define MATRIX_DAT 17
#define MATRIX_SEL 16
#define MATRIX_CLK 15
#elif defined(ESP8266)
#define MATRIX_CLK 4
#define MATRIX_DAT 5
#define MATRIX_SEL 16
#else
#error "Building for unknown platform, bailing..."
#endif

MATRIX7219 mx(MATRIX_DAT, MATRIX_SEL, MATRIX_CLK, MATRIX_CNT);

#define MIN_ATTENTION_INTERVAL 1000
#define POLL_DELAY 10000
/* goes from 0 to 16 */
#define DISPLAY_BRIGHTNESS 7

const IPAddress local_ip(4, 3, 2, 1);   // the IP address for the web server
const IPAddress gateway_ip(4, 3, 2, 1); // IP address of the network
const IPAddress subnetMask(255, 255, 255, 0);

DNSServer dns;
AsyncWebServer server(80);

int was_wifi_setup = 0;
unsigned long button_pressed_at = 0;
EButtonMode mode = CLIENT_MODE;

int need_to_post = 0;

void IRAM_ATTR attn_request() { need_to_post = 1; }

void setup() {
    Serial.begin(9600);

    while (!Serial);
    Serial.println("Starting...");

    pinMode(ENC_SW, INPUT);
    pinMode(ENC_DAT, INPUT);
    pinMode(ENC_CLK, INPUT);
    pinMode(MATRIX_SEL, OUTPUT);
    pinMode(MATRIX_CLK, OUTPUT);
    pinMode(MATRIX_DAT, INPUT);
    pinMode(BZ1, OUTPUT);

    if (!LittleFS.begin(FORMAT_LITTLEFS_ON_ERR)) {
        Serial.printf("Error initialising filesystem!\n");
        while (1);
    }

    attachInterrupt(digitalPinToInterrupt(ENC_SW), attn_request, FALLING);
}

int wifi_connected = 0;

char ssid[64] = {0};
char psk[64] = {0};
char secret[64] = {0};
char username[255] = {0};

#define FETCH_IF_EMPTY(variable, max_len, error)                               \
    if (!variable[0]) {                                                        \
        size_t len = kv_get(#variable, variable, max_len);                     \
        if (!len) fatal_error(error);                                          \
    }

int setup_wifi(char *ssid, char *psk) {
    if (wifi_connected) return 1;
    WiFi.begin(ssid, psk);
    loading_screen("...");
    int result = WiFi.waitForConnectResult();
    if (result != WL_CONNECTED) return 0;
    set_clock();
    return wifi_connected = 1;
}

unsigned long last_poll_time = 0;

#if defined(ESP8266)
X509List root_ca_x509(ROOT_CA_CERT);
#endif

template <typename ResponseHandler>
int hit_server(const char *route, ResponseHandler handler) {
    NetworkClientSecure *client = new NetworkClientSecure();
    int id = 0;
    if (!client) return id;
#if defined(ESP32)
    client->setCACert(ROOT_CA_CERT);
#elif defined(ESP8266)
    client->setTrustAnchors(&root_ca_x509);
#endif
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

unsigned long last_request_time = 0;

void loop() {
    unsigned long now = millis();
    int bstate = !digitalRead(ENC_SW);
    if (now < 2000) {
        if ((mode != SETUP_MODE) && bstate) {
            Serial.println("Entering setup mode...");
            digitalWrite(2, HIGH);
            mode = SETUP_MODE;
            return;
        } else {
            Serial.println("Entering client mode...");
        }
    }

    switch (mode) {
        case SETUP_MODE: {
            if (!was_wifi_setup) {
                was_wifi_setup = 1;
                start_soft_ap(CAPTIVE_SSID, CAPTIVE_PWD, local_ip, gateway_ip);
                setup_dns_server(dns, local_ip);
                String scan_results = scan_wifi_networks();
                Serial.println(scan_results);
                set_up_webserver(server, scan_results, local_ip);
                server.begin();
                return;
            }

            dns.processNextRequest();
            delay(DNS_INTERVAL);
            break;
        }

        case CLIENT_MODE: {
            FETCH_IF_EMPTY(ssid, 63, CONNECTION_ERROR);
            FETCH_IF_EMPTY(psk, 63, CONNECTION_ERROR);
            FETCH_IF_EMPTY(secret, 63, CREDENTIALS_ERROR);
            FETCH_IF_EMPTY(username, 254, CREDENTIALS_ERROR);
            if (!wifi_connected && !setup_wifi(ssid, psk)) return;

            if (need_to_post &&
                now - last_request_time > MIN_ATTENTION_INTERVAL) {
                last_request_time = now;
                need_to_post = 0;
                Serial.println("requesting attention");
                int status = hit_server(
                    ROUTE("/request-attention"),
                    [](HTTPClient &https, int code) { return code == 200; });
            } else if (now - last_poll_time >=
                       POLL_DELAY) { // poll every n seconds
                last_poll_time = now;
                Serial.println("querying for messages");
                int icon = hit_server(ROUTE("/list-messages"),
                                      [](HTTPClient &https, int code) {
                                          if (code != 200) return 0;
                                          String response = https.getString();
                                          JsonDocument doc;
                                          deserializeJson(doc, response);
                                          if (!doc["icon_id"].is<int>())
                                              return 0;
                                          int id = doc["icon_id"];
                                          return id;
                                      });
            }

            break;
        }
    }
}
