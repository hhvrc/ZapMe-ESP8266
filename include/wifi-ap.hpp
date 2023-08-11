#pragma once

#include <IPAddress.h>

namespace WiFi_AP {
  bool Initialize(IPAddress ip, IPAddress subnet);
  bool Start(const char* ssid, const char* psk = nullptr);
  void Stop();
  void Loop();

  std::uint8_t GetClientCount();
}
