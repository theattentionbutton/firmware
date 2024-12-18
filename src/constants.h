#include <DNSServer.h>

#ifndef __CONSTANTS_H
#define __CONSTANTS_H

#define MATRIX_CNT 1 // matrix count

#define ENC_DAT 14
#define ENC_CLK 10
#define ENC_SW 13
#define BZ1 12
#define MATRIX_CLK 4
#define MATRIX_DAT 5
#define MATRIX_SEL 16
#define FORMAT_LITTLEFS_ON_ERR

#define MIN_ATTENTION_INTERVAL 1000
#define POLL_DELAY 10000
/* goes from 0 to 16 */
#define DISPLAY_BRIGHTNESS 7

const IPAddress local_ip(4, 3, 2, 1);   // the IP address for the web server
const IPAddress gateway_ip(4, 3, 2, 1); // IP address of the network
const IPAddress subnet_mask(255, 255, 255, 0);

const uint8_t pin_modes[][2] = {{ENC_SW, INPUT_PULLUP}, {ENC_DAT, INPUT},
                                {ENC_CLK, INPUT},       {BZ1, OUTPUT},
                                {MATRIX_DAT, OUTPUT},   {MATRIX_CLK, OUTPUT},
                                {MATRIX_SEL, OUTPUT},   {MATRIX_CNT, OUTPUT}};

#endif