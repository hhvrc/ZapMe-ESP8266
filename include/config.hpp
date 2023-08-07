#pragma once

#include <memory>
#include <cstdint>

struct Config {
  static const std::uint16_t VERSION = 1;

  static void Migrate();
  static std::shared_ptr<Config> LoadReadWrite();
  static std::shared_ptr<const Config> LoadReadOnly();

  // Not necessary to call Save(), changes will be saved on pointer destruction
  bool Save();

  struct WiFiNetwork {
    std::uint8_t flags;
    char ssid[32 + 1];
    char password[63 + 1];
  };

  struct WiFi {
    char apName[32 + 1];
    char apPassword[63 + 1];
    WiFiNetwork networks[4];
  };

  char __IDENTIFIER[4];
  std::uint16_t __VERSION;

  WiFi wifi;
};
