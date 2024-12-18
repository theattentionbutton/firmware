#include "FS.h"
#include "captive_portal.h"
#include "captive_portal_index.h"
#include "littlefs_kv.h"
#include "utils.h"
#include <ESPAsyncWebSrv.h>
#include <flash_hal.h>

#ifndef __CAPTIVE_PORTAL_SERVER
#define __CAPTIVE_PORTAL_SERVER

const char *JSON_RESP_OK = "{\"message\": \"ok\"}";
const char *JSON_RESP_INVALID_VALUE = "{\"message\": \"invalid-value\"}";
const char *JSON_RESP_MISSING_VALUE = "{\"message\": \"missing-value\"}";

#define DEFINE_RU_ROUTE(path, key, max_len)                                    \
    server.on(path, HTTP_ANY, [](AsyncWebServerRequest *request) {             \
        handle_ru_route(request, key, max_len);                                \
    });

// RU = Read, Update
void handle_ru_route(AsyncWebServerRequest *request, const char *param,
                     size_t max_len) {
    auto respond = [&request](int code, const char *content) {
        AsyncWebServerResponse *res =
            request->beginResponse(code, "application/json", content);
        request->send(res);
    };

    if (request->method() == HTTP_POST) {
        if (request->hasParam("value", true)) {
            String s = request->getParam("value", true)->value();
            if (s.length() > max_len) {
                return respond(400, JSON_RESP_INVALID_VALUE);
            }

            kv_put(param, s.c_str());
            return respond(200, JSON_RESP_OK);
        }

        return respond(400, JSON_RESP_MISSING_VALUE);
    }

    char value[KV_SIZE_MAX] = {0};
    size_t len = kv_get(param, value, KV_SIZE_MAX - 1);
    if (!len) {
        respond(404, JSON_RESP_MISSING_VALUE);
    }
    char resp[512] = {0};
    sprintf(resp, "{\"value\":\"%s\", \"message\": \"ok\"}", value);
    return respond(200, resp);
}

char routes_to_local_ip[][32] PROGMEM = {
    "/generate_204",        // android captive portal redirect
    "/redirect",            // microsoft redirect
    "/hotspot-detect.html", // apple call home
    "/canonical.html",      // firefox captive portal call home
    "/ncsi.txt",            // windows call home
    "/startpage"            // general redirect
};

char routes_200[][32] PROGMEM = {
    "/success.txt",            // firefox captive portal call home
    "/chrome-variations/seed", // chrome captive portal call home
    "/service/update2/json"    // firefox?
};

char routes_404[][32] = {
    "/wpad.dat", // Honestly don't understand what this is but a 404 stops win
                 // 10 keep calling this repeatedly and panicking the esp32 :)
    "/chat",     // No stop asking Whatsapp, there is no internet connection
    "/favicon.ico"};

void handleUpdateUpload(AsyncWebServerRequest *request, String filename,
                        size_t index, uint8_t *data, size_t len, bool final) {
    size_t content_length = request->contentLength();
    if (!index) {
        if (!request->hasParam("MD5", true)) {
            return request->send(400, "text/plain", "MD5 parameter missing");
        }

        if (!Update.setMD5(request->getParam("MD5", true)->value().c_str())) {
            return request->send(400, "text/plain", "MD5 parameter invalid");
        }

        if (!Update.begin(content_length, U_FLASH)) {
            Update.printError(Serial);
        }
    }

    for (size_t i = 0; i < len; i++) {
        if (Update.write(data, len) != len) {
            Update.printError(Serial);
        }

        else {
            Serial.printf("[update] Progress: %d%%\n",
                          (Update.progress() * 100) / Update.size());
        }
    }

    if (final) {
        AsyncWebServerResponse *response = request->beginResponse(
            302, "application/json", "Please wait while the device reboots");
        response->addHeader("Refresh", "20");
        response->addHeader("Location", "/");
        request->send(response);
        if (!Update.end(true)) {
            Update.printError(Serial);
        } else {
            Serial.println("[update] Update complete");
            Serial.flush();
            ESP.restart();
        }
    }
}

void set_up_webserver(AsyncWebServer &server, const String &scanResults,
                      const IPAddress &local_ip) {
    //======================== Webserver ========================
    // WARNING IOS (and maybe macos) WILL NOT POP UP IF IT CONTAINS THE WORD
    // "Success" https://www.esp8266.com/viewtopic.php?f=34&t=4398 SAFARI (IOS)
    // IS STUPID, G-ZIPPED FILES CAN'T END IN .GZ
    // https://github.com/homieiot/homie-esp8266/issues/476 this is fixed by the
    // webserver serve static function. SAFARI (IOS) there is a 128KB limit to
    // the size of the HTML. The HTML can reference external resources/images
    // that bring the total over 128KB SAFARI (IOS) popup browser has some
    // severe limitations (javascript disabled, cookies disabled)

    auto local_ip_handler = [](AsyncWebServerRequest *request) {
        request->redirect(LOCAL_IP_URL);
    };

    for (auto &route : routes_to_local_ip) {
        server.on(route, local_ip_handler);
    }

    auto send_404 = [](AsyncWebServerRequest *request) { request->send(404); };

    for (auto &route : routes_404) {
        server.on(route, send_404);
    }

    auto send_200 = [](AsyncWebServerRequest *request) { request->send(200); };

    for (auto &route : routes_200) {
        server.on(route, send_200);
    }

    // Required
    server.on("/connecttest.txt", [](AsyncWebServerRequest *request) {
        request->redirect("http://logout.net");
    }); // windows 11 captive portal workaround

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

    server.on("/scan", HTTP_ANY, [scanResults](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *res =
            request->beginResponse(200, "application/json", scanResults);
        request->send(res);
    });

    server.on(
        "/update", HTTP_POST,
        [](AsyncWebServerRequest *request) { request->send(200); },
        handleUpdateUpload);

    DEFINE_RU_ROUTE("/secret", "secret", 64 - 1);
    DEFINE_RU_ROUTE("/psk", "psk", 64 - 1);
    DEFINE_RU_ROUTE("/ssid", "ssid", 32 - 1);
    DEFINE_RU_ROUTE("/username", "username", 255 - 1);

    server.onNotFound(local_ip_handler);
}

#endif