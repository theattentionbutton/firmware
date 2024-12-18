#include "button.h"
#include "captive_portal.h"
#include "captive_portal_server.h"
#include "constants.h"
#include "utils.h"

AttentionButton *btn;

void IRAM_ATTR attn_request() { btn->schedule_request(); }

void setup() {
    for (auto &mode : pin_modes) {
        pinMode(mode[0], mode[1]);
    }

    btn = new AttentionButton();

    if (!LittleFS.begin(FORMAT_LITTLEFS_ON_ERR)) {
        Serial.printf("! [error] Error initialising filesystem!\n");
        while (1);
    }

    attachInterrupt(digitalPinToInterrupt(ENC_SW), attn_request, FALLING);
}

void loop() {
    unsigned long now = millis();
    int bstate = !digitalRead(ENC_SW);
    if (now < 2000) {
        if (bstate && !btn->begun && btn->mode != SETUP_MODE) {
            btn->begin_setup_mode();
        }
    } else {
        if (!btn->begun) btn->begin_client_mode();
    }

    if (btn->begun) {
        switch (btn->mode) {
            case SETUP_MODE:
                btn->setup_iter();
                delay(DNS_INTERVAL);
                break;
            case CLIENT_MODE:
                btn->request_attention(now);
                btn->poll(now);
                break;
        }
    }
}
