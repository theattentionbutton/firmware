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

#define EXTRAS_COUNT 2
IconId extras_icons[EXTRAS_COUNT] = {RINGTONE, LIGHTBULB};

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
                case BRIGHTNESS_SELECT: {
                    Serial.printf("[debug] Updated brightness value: %d\n",
                                  btn->brightness_value);
                    btn->set_brightness(btn->brightness_value);
                    play_track_by_name("SUCCESS");
                    break;
                }
                case EXTRAS: {
                    btn->menu_mode = (MenuMode)(EXTRAS + (1 + btn->extras_idx));
                    btn->mx_clear();
                    if (btn->extras_idx == 0) {
                        btn->mx_clear();
                        btn->ringtone_idx = 0;
                    } else if (btn->extras_idx == 1) {
                        btn->draw_number(btn->brightness_value);
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

                case BRIGHTNESS_SELECT: {
                    btn->brightness_value =
                        CLAMP(btn->brightness_value + b.increment(), 1, 15);
                    btn->mx_update_brightness();
                    btn->draw_number(btn->brightness_value);
                    break;
                }

                case EXTRAS: {
                    btn->extras_idx += b.increment();
                    btn->extras_idx =
                        CLAMP(btn->extras_idx, 0, EXTRAS_COUNT - 1);
                    const char *name = icon_name(extras_icons[btn->extras_idx]);
                    btn->draw_icon(name);
                    break;
                }
            }
            break;
        }
        case InputEventType::LONG_CLICKED:
            switch (btn->menu_mode) {
                case RINGTONE_SELECT:
                case BRIGHTNESS_SELECT:
                case MAIN_MENU: {
                    btn->menu_mode = EXTRAS;
                    btn->draw_icon_by_id(extras_icons[0]);
                    btn->extras_idx = 0;
                    btn->brightness_value = btn->get_stored_brightness();
                    btn->mx_update_brightness();
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

    bool fs_init_success = LittleFS.begin(FORMAT_LITTLEFS_ON_ERR);

    btn = new AttentionButton(fs_init_success);

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
