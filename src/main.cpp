#include "button.h"
#include "captive_portal.h"
#include "captive_portal_server.h"
#include "constants.h"
#include "utils.h"
#include <Encoder.h>
#include <EventEncoderButton.h>

// Create an EventEncoderButton input
EventEncoderButton enc(ENC_DAT, ENC_CLK,
                       ENC_SW); // First two should be interrupt pins
// Create a callback handler function
void on_enc_input(InputEventType ev, EventEncoderButton &b) {
    Serial.print("Encoder button event fired. Position is: ");
    Serial.print(b.position());
    Serial.print(" Pressed position is: ");
    Serial.println(b.pressedPosition());
}

#define MQTT_HOST IPAddress(192, 168, 0, 104)

AttentionButton *btn;

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

    if (btn->begun) {
        switch (btn->mode) {
            case SETUP_MODE:
                btn->setup_iter();
                delay(DNS_INTERVAL);
                break;
            case CLIENT_MODE:
                btn->do_request(now);
                break;
        }
    }
}
