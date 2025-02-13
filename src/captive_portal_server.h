#include "FS.h"
#include "captive_portal.h"
#include "captive_portal_index.h"
#include "littlefs_kv.h"
#include "utils.h"
#include <ESP8266WebServer.h>
#include <flash_hal.h>

#ifndef __CAPTIVE_PORTAL_SERVER
#define __CAPTIVE_PORTAL_SERVER

const char *JSON_RESP_OK = "{\"message\": \"ok\"}";
const char *JSON_RESP_INVALID_VALUE = "{\"message\": \"invalid-value\"}";
const char *JSON_RESP_MISSING_VALUE = "{\"message\": \"missing-value\"}";

#define DEFINE_RU_ROUTE(path, key, max_len)                                    \
    server.on(path, HTTP_ANY,                                                  \
              [&server]() { handle_ru_route(server, key, max_len); });

// RU = Read, Update
void handle_ru_route(ESP8266WebServer &server, const char *param,
                     size_t max_len) {
    auto respond = [&server](int code, const char *content) {
        server.send(code, "application/json", content);
    };

    if (server.method() == HTTP_POST) {
        if (server.hasArg("value")) {
            String s = server.arg("value");
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

char routes_to_local_ip[][32] = {
    "/generate_204",        // android captive portal redirect
    "/redirect",            // microsoft redirect
    "/hotspot-detect.html", // apple call home
    "/canonical.html",      // firefox captive portal call home
    "/ncsi.txt",            // windows call home
    "/startpage"            // general redirect
};

char routes_200[][32] = {
    "/success.txt",            // firefox captive portal call home
    "/chrome-variations/seed", // chrome captive portal call home
    "/service/update2/json"    // firefox?
};

char routes_404[][32] = {
    "/wpad.dat", // Honestly don't understand what this is but a 404 stops win
                 // 10 keep calling this repeatedly and panicking the esp32 :)
    "/chat",     // No stop asking Whatsapp, there is no internet connection
    "/favicon.ico"};

#define UPDATE_DRY_RUN 0 // set to 1 to not actually perform updates

typedef enum update_error_type_t {
    UPDATE_OK,
    UPDATE_UNINITIALIZED,
    MD5_MISSING_OR_INVALID,
    UPDATER_ERROR,
} UpdateErrorType;

typedef struct update_status_t {
    char msg[64];
    UpdateErrorType type;
    unsigned long last_update;
    int file_size;
} UpdateStatus;

UpdateStatus update_status = {.msg = {0},
                              .type = UpdateErrorType::UPDATE_OK,
                              .last_update = millis(),
                              .file_size = -1};

void handleUpdate(ESP8266WebServer &server) {
    if (server.uri() != "/update") return;
    HTTPUpload &upload = server.upload();
    size_t content_length = upload.contentLength;
    if (update_status.file_size < 0 && server.hasArg("file-size")) {
        String val = server.arg("file-size");
        int parsed = atoi(val.c_str());
        update_status.file_size = parsed ? parsed : content_length;
    }
    if (update_status.type != UpdateErrorType::UPDATE_OK) {
        return;
    }
    switch (upload.status) {
        case UPLOAD_FILE_START: {
            if (!server.hasArg("MD5")) {
                update_status.type = MD5_MISSING_OR_INVALID;
                return;
            }

            Serial.printf("[update] Content-Length: %u\n", content_length);
            Serial.printf("[update] Flash chip size: %u\n",
                          ESP.getFlashChipRealSize());
            Serial.printf("[update] Free sketch space: %u\n",
                          ESP.getFreeSketchSpace());

            const char *md5 = server.arg("MD5").c_str();
            Serial.printf("[update] md5: %s\n", md5);
#if UPDATE_DRY_RUN == 0
            if (!Update.setMD5(md5)) {
                update_status.type = UpdateErrorType::MD5_MISSING_OR_INVALID;
                return;
            }

            if (!Update.begin(content_length, U_FLASH)) {
                update_status.type = UpdateErrorType::UPDATER_ERROR;
                strncpy(update_status.msg, "Could not start update!", 63);
                return;
            }
#endif
            Serial.printf("[update] Starting update\n");
            break;
        }
        case UPLOAD_FILE_WRITE: {
            unsigned long now = millis();
            if (now - update_status.last_update > 500) {
                update_status.last_update = now;
                Serial.printf("[update] Free heap: %u bytes\n",
                              ESP.getFreeHeap());
                Serial.printf(
                    "[update] Upload progress: %u bytes, %.2f%%\n",
                    upload.totalSize,
                    ((float)upload.totalSize / update_status.file_size) * 100);
            }
#if UPDATE_DRY_RUN == 0
            // Write chunk
            if (Update.write(upload.buf, upload.currentSize) !=
                upload.currentSize) {
                String err = Update.getErrorString();
                update_status.type = UPDATER_ERROR;
                strncpy(update_status.msg, err.c_str(), 63);
                return;
            }
#endif
            break;
        }
        case UPLOAD_FILE_END: {
#if UPDATE_DRY_RUN == 0
            if (!Update.end(true)) {
                String err = Update.getErrorString();
                strncpy(update_status.msg, err.c_str(), 63);
                update_status.type = UPDATER_ERROR;
            }
#else
            Serial.println("[update] Dry run complete. Not restarting.");
#endif
            break;
        }
        default:
            break;
    }

    esp_yield();
}

void redirect(ESP8266WebServer &server, const char *location) {
    server.sendHeader("Location", location, true);
    server.send(302, "text/plain", "");
}

void set_up_webserver(ESP8266WebServer &server, const String &scan_results,
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

    auto local_ip_handler = [&server]() { redirect(server, LOCAL_IP_URL); };

    for (auto &route : routes_to_local_ip) {
        server.on(route, local_ip_handler);
    }

    auto send_404 = [&server]() {
        server.send(404, "text/plain", "Not Found");
    };

    for (auto &route : routes_404) {
        server.on(route, send_404);
    }

    auto send_200 = [&server]() { server.send(200, "text/plain", "OK"); };

    for (auto &route : routes_200) {
        server.on(route, send_200);
    }

    // Required
    server.on("/connecttest.txt", [&server]() {
        redirect(server, "http://logout.net");
    }); // windows 11 captive portal workaround

    server.on("/", HTTP_GET, [&server]() {
        server.send_P(200, "text/html", index_html, sizeof(index_html));
    });

    server.on("/scan", HTTP_ANY, [&server, scan_results]() {
        server.send(200, "application/json", scan_results);
    });

    server.on(
        "/update", HTTP_POST,
        [&server]() {
            int code = 200;
            const char *msg =
                "Update completed successfully! The device will now restart.";
            switch (update_status.type) {
                case UpdateErrorType::MD5_MISSING_OR_INVALID:
                    msg = "Missing or invalid MD5.";
                    code = 400;
                    break;
                case UpdateErrorType::UPDATER_ERROR:
                    msg = update_status.msg;
                    code = 500;
                    break;
                default:
                    break;
            }

            server.client().setNoDelay(true);
            server.send(code, "text/plain", msg);

            if (code == 200) {
                delay(500);
                server.client().stop();
                ESP.restart();
            }
        },
        [&server]() { handleUpdate(server); });

    DEFINE_RU_ROUTE("/secret", "secret", 64 - 1);
    DEFINE_RU_ROUTE("/psk", "psk", 64 - 1);
    DEFINE_RU_ROUTE("/ssid", "ssid", 32 - 1);
    DEFINE_RU_ROUTE("/username", "username", 255 - 1);

    server.onNotFound(local_ip_handler);
}

#endif