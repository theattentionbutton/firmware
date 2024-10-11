#include "MATRIX7219.h"
#include "SPIFFS.h"
#include "captive_portal.h"
#include "captive_portal_server.h"
#include "cert.h"
#include "utils.h"
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebSrv.h>
#include <esp_wifi.h>

MATRIX7219 mx(17 /* data pin */, 16 /* select pin */, 15 /* clock pin */,
              1 /* number of matrices */);

#define BUTTON 22
#define FORMAT_SPIFFS_IF_FAILED true

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

void setup()
{
  Serial.begin(9600);

  while (!Serial)
    ;
  Serial.println("Starting...");

  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(2, OUTPUT);

  if (!SPIFFS.begin(true))
  {
    Serial.printf("Error initialising SPIFFS!\n");
    while (1)
      ;
  }

  attachInterrupt(BUTTON, attn_request, FALLING);
}

int wifi_connected = 0;

char ssid[64] = {0};
char psk[64] = {0};
char secret[64] = {0};
char username[255] = {0};

#define FETCH_IF_EMPTY(variable, max_len, error)       \
  if (!variable[0])                                    \
  {                                                    \
    size_t len = kv_get(#variable, variable, max_len); \
    if (!len)                                          \
      fatal_error(error);                              \
  }

int setup_wifi(char *ssid, char *psk)
{
  if (wifi_connected)
    return 1;
  WiFi.begin(ssid, psk);
  loading_screen("...");
  int result = WiFi.waitForConnectResult();
  if (result != WL_CONNECTED)
    return 0;
  set_clock();
  return wifi_connected = 1;
}

unsigned long last_poll_time = 0;

template <typename ResponseHandler>
int hit_server(const char *route, ResponseHandler handler)
{
  NetworkClientSecure *client = new NetworkClientSecure();
  int id = 0;
  if (!client)
    return id;
  client->setCACert(ROOT_CA_CERT);
  {
    HTTPClient https;
    if (!https.begin(*client, route))
      goto cleanup;
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

void loop()
{
  unsigned long now = millis();
  int bstate = !digitalRead(BUTTON);
  if (now < 2000 && mode != SETUP_MODE && bstate)
  {
    Serial.println("Entering wifi mode...");
    digitalWrite(2, HIGH);
    mode = SETUP_MODE;
    return;
  }

  switch (mode)
  {
  case SETUP_MODE:
  {
    if (!was_wifi_setup)
    {
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

  case CLIENT_MODE:
  {
    FETCH_IF_EMPTY(ssid, 63, CONNECTION_ERROR);
    FETCH_IF_EMPTY(psk, 63, CONNECTION_ERROR);
    FETCH_IF_EMPTY(secret, 63, CREDENTIALS_ERROR);
    FETCH_IF_EMPTY(username, 254, CREDENTIALS_ERROR);
    if (!wifi_connected && !setup_wifi(ssid, psk))
      return;

    if (need_to_post &&
        now - last_request_time > MIN_ATTENTION_INTERVAL)
    {
      last_request_time = now;
      need_to_post = 0;
      Serial.println("requesting attention");
      int status = hit_server(
          ROUTE("/request-attention"),
          [](HTTPClient &https, int code)
          { return code == 200; });
    }
    else if (now - last_poll_time >=
             POLL_DELAY)
    { // poll every n seconds
      last_poll_time = now;
      Serial.println("querying for messages");
      int icon = hit_server(ROUTE("/list-messages"),
                            [](HTTPClient &https, int code)
                            {
                              if (code != 200)
                                return 0;
                              String response = https.getString();
                              JsonDocument doc;
                              deserializeJson(doc, response);
                              if (!doc.containsKey("icon_id"))
                                return 0;
                              int id = doc["icon_id"];
                              return id;
                            });
    }

    break;
  }
  }
}
