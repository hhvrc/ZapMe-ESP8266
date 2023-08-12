#pragma once

#include <WiFiUdp.h>
#include <ctime>
#include <cstdint>

class NtpClient
{
  static constexpr std::size_t NTP_PORT = 123;
  static constexpr std::size_t NTP_PACKET_SIZE = 48;
  static constexpr std::size_t NTP_UPDATE_INTERVAL_SECONDS = 30;
public:
  NtpClient();
  ~NtpClient();

  bool begin();
  void end();

  void update();

  bool isTimeValid() const;
  time_t getEpochTime() const;
private:
  WiFiUDP _udp;
  std::uint8_t _buffer[NTP_PACKET_SIZE];
  std::size_t _serverIndex;
  std::uint64_t _lastTransmit;
  std::uint64_t _lastReceive;
  time_t _epochTime;


  void sendNtpPacket();
  bool handleNtpPacket();
};
