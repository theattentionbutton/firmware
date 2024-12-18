#include "button.h"
#include "captive_portal.h"
#include "captive_portal_server.h"
#include "constants.h"
#include "utils.h"
#include <LeanTask.h>
#include <Scheduler.h>
#include <Task.h>

AttentionButton *btn;

class MusicTask : public Task {
  protected:
    void loop() {
        const MidiTrack *t = btn->current_track;
        if (!t) return delay(10);
        for (int i = 0; i < t->length; i++) {
            int *note = t->first[i];
            if (note[2] >= 5) delay((unsigned long)note[2]);
            tone(BZ1, note[0]);
            delay((unsigned long)note[1] + 10);
            noTone(BZ1);
        }
        btn->current_track = NULL;
        btn->playing = false;
        delay(50);
    }
} music_task;

class MainTask : public LeanTask {
  protected:
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
                    break;
                case CLIENT_MODE:
                    btn->request_attention(now);
                    btn->poll(now);
                    break;
            }
        }

        delay(DNS_INTERVAL);
    }

  private:
    bool was_ap_setup = false;
} main_task;

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
    Scheduler.start(&main_task);
    Scheduler.start(&music_task);
    Scheduler.begin();
}

void loop() {};
