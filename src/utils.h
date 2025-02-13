#include "icons.h"
#include <ArduinoJson.h>
#include <Encoder.h>
#include <EventEncoderButton.h>
#include <MATRIX7219.h>

#define BAUD_RATE 9600

#if defined(ESP8266)
#define WIFI_AUTH_OPEN AUTH_OPEN
#define WIFI_AUTH_WEP AUTH_WEP
#define WIFI_AUTH_WPA_PSK AUTH_WPA_PSK
#define WIFI_AUTH_WPA2_PSK AUTH_WPA2_PSK
#define WIFI_AUTH_WPA_WPA2_PSK AUTH_WPA_WPA2_PSK
#endif

#ifndef __CLIENT_UTILS
#define __CLIENT_UTILS

#define REMOTE_URL "https://theattentionbutton.in"
#define ROUTE(x) (REMOTE_URL x)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(value, min, max) (MAX((min), MIN((value), (max))))

typedef enum button_mode_t { SETUP_MODE = 1, CLIENT_MODE } EButtonMode;

#ifndef ESP_ARDUINO_VERSION_VAL
#define ESP_ARDUINO_VERSION_VAL(major, minor, patch)                           \
    ((major << 16) | (minor << 8) | (patch))
#endif

void set_clock() {
    configTime(0, 0, "pool.ntp.org");

    Serial.print(F("Waiting for NTP time sync: "));
    time_t nowSecs = time(nullptr);
    while (nowSecs < 8 * 3600 * 2) {
        delay(500);
        Serial.print(F("."));
        yield();
        nowSecs = time(nullptr);
    }

    Serial.println();
    struct tm timeinfo;
    gmtime_r(&nowSecs, &timeinfo);
    Serial.print(F("Current time: "));
    Serial.print(asctime(&timeinfo));
}

const char *wifi_auth_mode(int auth_mode) {
    switch (auth_mode) {
        case WIFI_AUTH_OPEN:
            return "Open";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA/WPA2";
        default:
            return "Unknown";
    }
}

const char *wifi_frequency(int channel) {
    if (channel >= 1 && channel <= 14) {
        return "2.4 GHz";
    } else if ((channel >= 36 && channel <= 64) ||
               (channel >= 100 && channel <= 144) ||
               (channel >= 149 && channel <= 165)) {
        return "5 GHz";
    } else {
        return "Invalid channel";
    }
}

String scan_wifi_networks() {
    JsonDocument doc;
    JsonArray results = doc["scan_results"].to<JsonArray>();
    int n = WiFi.scanNetworks();
    // cap networks to 20 even if found
    for (uint8_t i = 0; i < 20 && i < n; i++) {
        JsonObject result = results.add<JsonObject>();
        result["rssi"] = WiFi.RSSI(i);
        result["ssid"] = WiFi.SSID(i);
        result["security"] = wifi_auth_mode(WiFi.encryptionType(i));
        result["band"] = wifi_frequency(WiFi.channel(i));
    }
    String scan_results;
    serializeJson(doc, scan_results);
    WiFi.scanDelete();
    return scan_results;
}

void draw_icon(IconId id, MATRIX7219 *mx) {
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t b = ICON(id)[i];
        mx->setRow(i + 1, b, 0);
    }
}

void fatal_error(IconId id, MATRIX7219 *mx) {
    draw_icon(id, mx);
    while (1) {
        delay(50);
        wdt_reset();
    }
}

int sign(int num) {
    if (num > 0) return 1;
    if (num < 0) return -1;
    return 0;
}

class EncoderBtn : public EventEncoderButton {
    bool music_stopped = false;

  public:
    using EventEncoderButton::EventEncoderButton;
    void stop_music() { music_stopped = true; }
    void allow_music() { music_stopped = false; }
    bool is_music_stopped() { return music_stopped; }
};

#endif
