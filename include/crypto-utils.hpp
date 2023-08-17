#pragma once

#include <cstdint>
#include <nonstd/span.hpp>

class CryptoUtils {
  CryptoUtils() = delete;

public:
  // Fill a buffer with random data
  static void RandomBytes(std::uint8_t* buffer, std::size_t length);
  static void RandomBytes(nonstd::span<std::uint8_t> buffer) { RandomBytes(buffer.data(), buffer.size()); }
};
