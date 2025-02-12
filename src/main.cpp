#include "button.h"
#include "captive_portal.h"
#include "captive_portal_server.h"
#include "constants.h"
#include "icons.h"
#include "utils.h"
#include <EventInputBase.h>

AttentionButton *btn;

// Create an EventEncoderButton input
EncoderBtn enc(ENC_DAT, ENC_CLK, ENC_SW); // First two should be interrupt pins

IconId extras_icons[] = {RINGTONE};

int icon_id = 0;
unsigned long last_enc_event = 0;

void on_enc_input(InputEventType ev, EventEncoderButton &b) {
    last_enc_event = millis();
    switch (ev) {
        case InputEventType::CLICKED:
            switch (btn->menu_mode) {
                case MAIN_MENU: {
                    btn->schedule_request();
                    break;
                }
                case RINGTONE_SELECT: {
                    btn->set_ringtone((char *)TRACK_NAMES[btn->ringtone_idx]);
                    Serial.printf("Updated ringtone: %s\n",
                                  btn->get_ringtone());
                    enc.stop_music();
                    play_track_by_name("SUCCESS");
                    break;
                }
                case EXTRAS: {
                    if (btn->extras_idx == 0) {
                        btn->menu_mode = RINGTONE_SELECT;
                        btn->mx_clear();
                        btn->ringtone_idx = 0;
                    }
                    break;
                }
            }
            break;
        case InputEventType::CHANGED: {
            if (btn->mode != CLIENT_MODE) break;
            if (!btn->ready) break;
            switch (btn->menu_mode) {
                case MAIN_MENU: {
                    btn->display_updated(ENC_INPUT, millis());
                    int pos = abs(b.position()) % DRAWABLE_LENGTH;
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

                case RINGTONE_SELECT: {
                    int pos = abs(b.position()) % TRACK_COUNT;
                    btn->draw_number(pos);
                    printf("Selected ringtone: %s\n", TRACK_NAMES[pos]);
                    if (pos) {
                        play_track_by_idx((MidiTrackIdx)pos, &enc);
                        btn->ringtone_idx = pos;
                    }
                    break;
                }

                case EXTRAS: {
                    break;
                }
            }
            break;
        }
        case InputEventType::LONG_CLICKED:
            Serial.println("[debug] long press");
            switch (btn->menu_mode) {
                case RINGTONE_SELECT:
                case MAIN_MENU: {
                    btn->menu_mode = EXTRAS;
                    btn->draw_icon_by_id(extras_icons[0]);
                    btn->extras_idx = 0;
                    break;
                }
                case EXTRAS: {
                    btn->menu_mode = MAIN_MENU;
                    btn->draw_icon("READY");
                    break;
                }
            }
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
    enc.setRateLimit(10);
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

    if (btn->menu_mode == MAIN_MENU && btn->needs_reset(now)) {
        Serial.printf("[loop] Time since last event: %lu - resetting display\n",
                      now - btn->last_disp_event_time);
        btn->reset_display();
    }

    if (btn->menu_mode == MAIN_MENU && now - last_enc_event > 30000) {
        enc.resetPosition();
    }

    if (btn->begun) {
        switch (btn->mode) {
            case SETUP_MODE:
                btn->setup_iter();
                delay(DNS_INTERVAL);
                break;
            case CLIENT_MODE:
                btn->do_request(now);
                btn->handle_mqtt(now);
                break;
        }
    }
}
