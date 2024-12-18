#include "button.h"
#include "captive_portal.h"
#include "captive_portal_server.h"
#include "constants.h"
#include "utils.h"
#include <Encoder.h>
#include <EventEncoderButton.h>
#include <EventInputBase.h>

AttentionButton *btn;

// Create an EventEncoderButton input
EventEncoderButton enc(ENC_DAT, ENC_CLK,
                       ENC_SW); // First two should be interrupt pins

int icon_id = 0;
unsigned long pos_change = 0;
bool disp_was_reset = true;
void on_enc_input(InputEventType ev, EventEncoderButton &b) {
    switch (ev) {
        case InputEventType::CLICKED:
            btn->schedule_request();
            break;
        case InputEventType::CHANGED: {
            pos_change = millis();
            disp_was_reset = false;
            int pos = b.position() % DRAWABLE_LENGTH;
            int counter = 0;
            for (int i = 0; i < DRAWABLE_LENGTH; i++) {
                if (pos == counter) break;
                counter += 1;
            }
            btn->draw_icon_by_id(DRAWABLE_ICONS[counter]);
            break;
        }
        default:
            break;
    }
}

#define MQTT_HOST IPAddress(192, 168, 0, 104)

void setup() {
    for (auto &mode : pin_modes) {
        pinMode(mode[0], mode[1]);
    }

    btn = new AttentionButton();

    if (!LittleFS.begin(FORMAT_LITTLEFS_ON_ERR)) {
        Serial.printf("! [error] Error initialising filesystem!\n");
        while (1);
    }

    enc.setCallback(on_enc_input);
}

void loop() {
    enc.update();
    unsigned long now = millis();
    if (now < 2000) {
        if (enc.isPressed() && enc.currentDuration() > 500 && !btn->begun &&
            btn->mode != SETUP_MODE) {
            btn->begin_setup_mode();
        }
    } else {
        if (!btn->begun) btn->begin_client_mode();
    }

    if (now - pos_change > 3000 && !disp_was_reset) {
        Serial.printf("[loop] Time since last event: %lu - resetting display\n",
                      now - pos_change);
        btn->draw_icon("READY");
        disp_was_reset = true;
    }

    if (btn->begun) {
        switch (btn->mode) {
            case SETUP_MODE:
                btn->setup_iter();
                delay(DNS_INTERVAL);
                break;
            case CLIENT_MODE:
                btn->do_request(now);
                btn->handle_mqtt();
                break;
        }
    }
}
