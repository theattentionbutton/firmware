#include <pgmspace.h>

#ifndef __ICON_H
#define __ICON_H

typedef enum icon_id_t {
    SUN = 1,
    READY,
    CONNECTION_ERROR,
    SMILEY,
    FROWN,
    HEART,
    CREDENTIALS_ERROR,
    LOADING,
    KISS,
    EXCLAMATION,
    SETTINGS
} IconId;

const int DRAWABLE_ICONS[] PROGMEM = {
    SUN,
    SMILEY,
    FROWN,
    HEART,
    KISS,
};

#define DRAWABLE_LENGTH (sizeof(DRAWABLE_ICONS)/sizeof(DRAWABLE_ICONS[0]))

const byte ICONS[][11] PROGMEM = {
    {0, 66, 24, 60, 60, 24, 66, 0}, // SUN
    {0, 0, 49, 82, 74, 140, 0, 0}, // READY
    {1, 62, 70, 137, 60, 98, 88, 128}, // CONNECTION_ERROR
    {0, 36, 36, 0, 0, 66, 60, 0}, // SMILEY
    {0, 36, 36, 0, 0, 60, 66, 0}, // FROWN
    {0, 102, 255, 255, 255, 126, 60, 24}, // HEART
    {24, 36, 32, 32, 36, 60, 60, 60}, // CREDENTIALS_ERROR
    {0, 0, 0, 219, 219, 0, 0, 0}, // LOADING
    {0, 102, 255, 60, 129, 126, 60, 0}, // KISS
    {0, 24, 24, 24, 24, 24, 0, 24}, // EXCLAMATION
    {0, 32, 255, 32, 4, 255, 4, 0} // SETTINGS
};

#define ICONS_LENGTH (sizeof(ICONS)/sizeof(ICONS[0]))

const byte *icon(IconId i) {
    return ICONS[i - 1];
}

#endif
