#include "wifi-ap.hpp"

#include "logger.hpp"

#include <DNSServer.h>
#include <ESP8266WiFi.h>

IPAddress apIP;

const byte DNS_PORT = 53;
DNSServer dnsServer;

bool running = false;

bool WiFi_AP::Start(const char* ssid, const char* psk) {
  WiFi.enableAP(true);
  WiFi.softAPdisconnect(true);

  IPAddress apIP(10, 0, 0, 1);
  if (!WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0))) {
    Logger::println("[WiFi_AP] Failed to enable access point");
    return false;
  }

  if (!WiFi.softAP(ssid, psk)) {
    Logger::printlnf("[WiFi_AP] Failed to start access point with SSID %s", ssid);

    return false;
  }

  if (!dnsServer.start(DNS_PORT, "*", apIP)) {
    Logger::println("[WiFi_AP] Failed to start DNS server");
    WiFi.softAPdisconnect(true);
    return false;
  }

  running = true;

  return true;
}

void WiFi_AP::Stop() {
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  running = false;
}

void WiFi_AP::Update() {
  if (!running) {
    return;
  }

  dnsServer.processNextRequest();
}

std::uint8_t WiFi_AP::GetClientCount() {
  return WiFi.softAPgetStationNum();
}
