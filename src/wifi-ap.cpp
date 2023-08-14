#include "wifi-ap.hpp"

#include <ESP8266WiFi.h>
#include <DNSServer.h>

IPAddress apIP;

const byte DNS_PORT = 53;
DNSServer dnsServer;

bool running = false;

bool WiFi_AP::Initialize(IPAddress ip, IPAddress subnet) {
  apIP = ip;
  return WiFi.softAPConfig(ip, ip, subnet);
}

bool WiFi_AP::Start(const char* ssid, const char* psk) {
  if (!WiFi.softAP(ssid, psk)) {
    return false;
  }

  if (!dnsServer.start(DNS_PORT, "*", apIP)) {
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
