#pragma once

#include <WString.h>
#include <span>
#include <concepts>
#include <cstdint>

class SecureConfig {
  SecureConfig() = delete;
public:
  static std::uint32_t Read(std::uint32_t offset, std::uint8_t* buffer, std::uint32_t size);
  static std::uint32_t Read(std::uint32_t offset, std::span<std::uint8_t> buffer) {
    return Read(offset, buffer.data(), buffer.size());
  }
  static std::uint32_t Read(std::uint32_t offset, std::span<char> buffer) {
    return Read(offset, reinterpret_cast<std::uint8_t*>(buffer.data()), buffer.size());
  }
  template <std::integral T>
  static std::uint32_t Read(std::uint32_t offset, T& value) {
    return Read(offset, reinterpret_cast<std::uint8_t*>(&value), sizeof(T));
  }
  static std::uint32_t Read(std::uint32_t offset, String& buffer);

  static std::uint32_t Write(std::uint32_t offset, const std::uint8_t* buffer, std::uint32_t size);
  static std::uint32_t Write(std::uint32_t offset, std::span<const std::uint8_t> buffer) {
    return Write(offset, buffer.data(), buffer.size());
  }
  static std::uint32_t Write(std::uint32_t offset, std::span<const char> buffer) {
    return Write(offset, reinterpret_cast<const std::uint8_t*>(buffer.data()), buffer.size());
  }
  template <std::integral T>
  static std::uint32_t Write(std::uint32_t offset, const T& value) {
    return Write(offset, reinterpret_cast<const std::uint8_t*>(&value), sizeof(T));
  }
  static std::uint32_t Write(std::uint32_t offset, const String& buffer);

  static bool Save();
  static void Clear();
};
