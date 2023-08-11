#include "wifi-ap.hpp"

#include <ESP8266WiFi.h>
#include <DNSServer.h>

IPAddress apIP;
IPAddress apSubnet;

const byte DNS_PORT = 53;
DNSServer dnsServer;

bool WiFi_AP::Initialize(IPAddress ip, IPAddress subnet) {
  apIP = ip;
  apSubnet = subnet;
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

  return true;
}

void WiFi_AP::Stop() {
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
}

void WiFi_AP::Loop() {
  dnsServer.processNextRequest();
}

std::uint8_t WiFi_AP::GetClientCount() {
  return WiFi.softAPgetStationNum();
}
