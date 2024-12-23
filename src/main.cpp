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
void on_enc_input(InputEventType ev, EventEncoderButton &b) {
    switch (ev) {
        case InputEventType::CLICKED:
            btn->schedule_request();
            break;
        case InputEventType::CHANGED: {
            btn->display_updated(ENC_INPUT, millis());
            int pos = b.position() % DRAWABLE_LENGTH;
            int counter = 0;
            for (int i = 0; i < DRAWABLE_LENGTH; i++) {
                if (pos == counter) break;
                counter += 1;
            }
            IconId id = DRAWABLE_ICONS[counter];
            btn->draw_icon_by_id(id);
            btn->set_selected_icon(id);

            const char *name = icon_name(id);
            Serial.printf("[debug] selected icon %s\n", name);
            break;
        }
        case InputEventType::LONG_CLICKED:
            Serial.println("[debug] long press");
            play_track_by_idx(SOMETHINGS, &enc);
            break;
        default:
            break;
    }
}

void setup() {
    for (auto &mode : pin_modes) {
        pinMode(mode[0], mode[1]);
    }

    btn = new AttentionButton();

    if (!LittleFS.begin(FORMAT_LITTLEFS_ON_ERR)) {
        Serial.printf("[!!! error !!!] could not init filesystem\n");
        while (1);
    }

    enc.setCallback(on_enc_input);
    enc.setPositionDivider(2);
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

    if (btn->needs_reset(now)) {
        Serial.printf("[loop] Time since last event: %lu - resetting display\n",
                      now - btn->last_disp_event_time);
        btn->reset_display();
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
