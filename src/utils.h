#include "icons.h"
#include <ArduinoJson.h>
#ifndef __CLIENT_UTILS
#define __CLIENT_UTILS

#define REMOTE_URL "https://theattentionbutton.in"
#define ROUTE(x) (REMOTE_URL x)

typedef enum button_mode_t { SETUP_MODE = 0, CLIENT_MODE } EButtonMode;

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
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WPA2 (enterprise)";
        case WIFI_AUTH_WPA3_PSK:
            return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WPA2/WPA3";
        case WIFI_AUTH_WAPI_PSK:
            return "WAPI";
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
    for (int i = 0; i < n; i++) {
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

void draw_icon(IconId id) {}

void fatal_error(IconId id) {
    Serial.printf("%d\n", id);
    while (1);
}

void loading_screen(const char *msg) {
    Serial.printf("DEBUG: loading screen\n");
}

#endif
