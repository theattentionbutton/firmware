#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>

#include <DNSServer.h>

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
#define DNS_INTERVAL 50

void setup_dns_server(DNSServer &dns, const IPAddress &local_ip) {
    // Set the TTL for DNS response and start the DNS server
    dns.setTTL(3600);
    bool started = dns.start(53, "*", local_ip);
    Serial.printf("[setup mode] DNS server startup %s\n",
                  started ? "successful" : "failed");
}

void start_soft_ap(const char *ssid, const char *password,
                   const IPAddress &local_ip, const IPAddress &gateway_ip) {
    WiFi.mode(WIFI_AP_STA);
    const IPAddress subnet_mask(255, 255, 255, 0);
    WiFi.softAPConfig(local_ip, gateway_ip, subnet_mask);
    WiFi.softAP(ssid, password, WIFI_CHANNEL, 0, MAX_CLIENTS);

    delay(100);
}

#endif
