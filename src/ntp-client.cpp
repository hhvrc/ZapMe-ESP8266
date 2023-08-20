#include "ntp-client.hpp"

#include "logger.hpp"

#include <ESP8266WiFi.h>

#include <array>

struct NtpServerRecord {
  const char* hostName;
  IPAddress ipAddr;
};

NtpServerRecord NTP_SERVERS[] = {
  {       "pool.ntp.org", {}},
  {      "time.nist.gov", {}},
  {    "time.google.com", {}},
  {"time.cloudflare.com", {}},
};
std::size_t NTP_SERVER_COUNT = sizeof(NTP_SERVERS) / sizeof(NtpServerRecord);

NtpClient::NtpClient() : _udp(), _buffer(), _serverIndex(0), _lastTransmit(0), _lastReceive(0) { }

NtpClient::~NtpClient() {
  end();
}

bool NtpClient::begin() {
  if (!WiFi.isConnected()) {
    Logger::println("[NTP] Unable to start NTP client: WiFi is not connected");
    return false;
  }

  // Resolve NTP server names to IP addresses
  bool anyResolved = false;
  for (std::size_t i = 0; i < NTP_SERVER_COUNT; ++i) {
    if (WiFi.hostByName(NTP_SERVERS[i].hostName, NTP_SERVERS[i].ipAddr)) {
      anyResolved = true;
    }
  }
  if (!anyResolved) {
    Logger::println("[NTP] Unable to start NTP client: Unable to resolve NTP server names");
    return false;
  }

  if (_udp.begin(NTP_PORT) != 1) {
    Logger::println("[NTP] Unable to start NTP client: Unable to bind UDP socket");
    return false;
  }

  return true;
}

void NtpClient::end() {
  _udp.stop();
}

void NtpClient::update() {
  std::uint64_t now = millis();

  if (now - _lastTransmit >= NTP_UPDATE_INTERVAL_SECONDS * 1000) {
    sendNtpPacket();
    _lastTransmit = now;
  }

  if (handleNtpPacket()) {
    _lastReceive = now;
  }
}

bool NtpClient::isTimeValid() const {
  return _lastReceive != 0 && millis() - _lastReceive < NTP_UPDATE_INTERVAL_SECONDS * 1000 * 2;
}

time_t NtpClient::getEpochTime() const {
  return _epochTime;
}

void NtpClient::sendNtpPacket() {
  memset(_buffer, 0, NTP_PACKET_SIZE);

  _buffer[0] = 0b11100011;  // LI, Version, Mode

  IPAddress ntpServerIP;
  for (std::size_t i = 0; i < NTP_SERVER_COUNT; ++i) {
    if (NTP_SERVERS[i].ipAddr.isSet()) {
      ntpServerIP  = NTP_SERVERS[i].ipAddr;
      _serverIndex = i;
      break;
    }
  }

  _udp.beginPacket(ntpServerIP, NTP_PORT);
  _udp.write(_buffer, NTP_PACKET_SIZE);
  _udp.endPacket();
}

bool NtpClient::handleNtpPacket() {
  if (_udp.parsePacket() <= 0) {
    return false;
  }

  if (_udp.read(_buffer, NTP_PACKET_SIZE) != NTP_PACKET_SIZE) {
    Logger::println("[NTP] Received invalid NTP packet");
    return false;
  }

  std::uint32_t NTPTime = ((std::uint32_t)_buffer[40] << 24) | ((std::uint32_t)_buffer[41] << 16)
                        | ((std::uint32_t)_buffer[42] << 8) | ((std::uint32_t)_buffer[43] << 0);

  constexpr std::uint32_t EPOCH = 2'208'988'800UL;

  _epochTime = NTPTime - EPOCH;

  Logger::printlnf("[NTP] Received packet: %Y-%m-%d %H:%M:%S", localtime(&_epochTime));

  return true;
}
