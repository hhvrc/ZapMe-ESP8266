#include "crypto-utils.hpp"

#include <ESP8266TrueRandom.h>

void CryptoUtils::RandomBytes(std::uint8_t* buffer, std::size_t length) {
  ESP8266TrueRandom.memfill(reinterpret_cast<char*>(buffer), length);
}
