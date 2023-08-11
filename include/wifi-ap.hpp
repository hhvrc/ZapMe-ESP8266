#pragma once

#include <IPAddress.h>

class WiFi_AP {
  WiFi_AP() = delete;
public:
  static bool Initialize(IPAddress ip, IPAddress subnet);
  static bool Start(const char* ssid, const char* psk = nullptr);
  static void Stop();
  static void Loop();

  static std::uint8_t GetClientCount();
};
