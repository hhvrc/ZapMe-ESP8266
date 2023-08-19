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
    Logger::printlnf("[WiFi_AP] STATUS: %d", WiFi.status());
    Logger::printlnf("[WiFi_AP] MODE: %d", WiFi.getMode());
    Logger::printlnf("[WiFi_AP] SSID: %s", WiFi.SSID());
    Logger::printlnf("[WiFi_AP] PSK: %s", WiFi.psk());
    Logger::printlnf("[WiFi_AP] BSSID: %s", WiFi.BSSIDstr());
    Logger::printlnf("[WiFi_AP] MAC: %s", WiFi.macAddress());
    Logger::printlnf("[WiFi_AP] HOSTNAME: %s", WiFi.hostname());
    Logger::printlnf("[WiFi_AP] IP: %s", WiFi.localIP().toString());
    Logger::printlnf("[WiFi_AP] SUBNET: %s", WiFi.subnetMask().toString());
    Logger::printlnf("[WiFi_AP] GATEWAY: %s", WiFi.gatewayIP().toString());
    Logger::printlnf("[WiFi_AP] DNS: %s", WiFi.dnsIP().toString());
    Logger::printlnf("[WiFi_AP] CHANNEL: %d", WiFi.channel());
    Logger::printlnf("[WiFi_AP] PHY: %d", WiFi.getPhyMode());
    Logger::printlnf("[WiFi_AP] RSSI: %d", WiFi.RSSI());

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
