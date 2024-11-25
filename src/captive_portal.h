#if defined(ESP8266)

#include <ESPAsyncTCP.h>
#define WIFI_MODE_APSTA WIFI_AP_STA

#elif defined(ESP32)

#include <AsyncTCP.h>
#include <esp_wifi.h>

#else
#error "Tried to compile for unsupported board, bailing"
#endif

#include <DNSServer.h>
#include <ESPAsyncWebSrv.h>

#ifndef __CAPTIVE_PORTAL
#define __CAPTIVE_PORTAL

#define CAPTIVE_SSID "AttentionButtonSetup"
#define CAPTIVE_PWD NULL
#define LOCAL_IP_URL "http://4.3.2.1"

#define MAX_CLIENTS 1 // We only want 1 person connected to setup at a time
#define WIFI_CHANNEL                                                           \
    6 // 2.4ghz channel 6
      // https://en.wikipedia.org/wiki/List_of_WLAN_channels#2.4_GHz_(802.11b/g/n/ax)

// Define the DNS interval in milliseconds between processing DNS requests
#define DNS_INTERVAL 30

void setup_dns_server(DNSServer &dns, const IPAddress &local_ip) {
    // Set the TTL for DNS response and start the DNS server
    dns.setTTL(3600);
    dns.start(53, "*", local_ip);
}

void start_soft_ap(const char *ssid, const char *password,
                   const IPAddress &local_ip, const IPAddress &gateway_ip) {
    // Set the WiFi mode to access point and station
    WiFi.mode(WIFI_MODE_APSTA);

    // Define the subnet mask for the WiFi network
    const IPAddress subnet_mask(255, 255, 255, 0);

    // Configure the soft access point with a specific IP and subnet mask
    WiFi.softAPConfig(local_ip, gateway_ip, subnet_mask);

    // Start the soft access point with the given ssid, password, channel, max
    // number of clients
    WiFi.softAP(ssid, password, WIFI_CHANNEL, 0, MAX_CLIENTS);

#if defined(ESP32)
    // Disable AMPDU RX on the ESP32 WiFi to fix a bug on Android
    esp_wifi_stop();
    esp_wifi_deinit();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.ampdu_rx_enable = false;
    esp_wifi_init(&cfg);
    esp_wifi_start();
#endif

    delay(100);
}

#endif
