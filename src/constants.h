#include <DNSServer.h>

#ifndef __CONSTANTS_H
#define __CONSTANTS_H

#define MATRIX_CNT 1 // matrix count

#define ENC_DAT 14
#define ENC_CLK 10
#define ENC_SW 13
#define BZ1 12

#if defined(ESP32)

#define MATRIX_DAT 17
#define MATRIX_SEL 16
#define MATRIX_CLK 15
#define FORMAT_LITTLEFS_ON_ERR true

#elif defined(ESP8266)

#define MATRIX_CLK 4
#define MATRIX_DAT 5
#define MATRIX_SEL 16
#define FORMAT_LITTLEFS_ON_ERR

#else
#error "Building for unknown platform, bailing..."
#endif

#define MIN_ATTENTION_INTERVAL 1000
#define POLL_DELAY 10000
/* goes from 0 to 16 */
#define DISPLAY_BRIGHTNESS 7

const IPAddress local_ip(4, 3, 2, 1);   // the IP address for the web server
const IPAddress gateway_ip(4, 3, 2, 1); // IP address of the network
const IPAddress subnet_mask(255, 255, 255, 0);

#endif