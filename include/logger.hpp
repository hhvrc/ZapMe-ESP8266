#pragma once

#include <cstdarg>
#include <cstdint>
#include <nonstd/span.hpp>
#include <WString.h>

class Logger {
  Logger() = delete;

public:
  enum class InitializationError {
    None,
    SDCardError,
    FileSystemError,
  };

  static InitializationError Initialize();

  static void vprintlnf(const char* format, va_list args);
  static void printlnf(const char* format, ...);
  static void println(const String& message);
  static void println(const char* message);
  static void println();

  static void printhexln(const std::uint8_t* data, std::size_t size);
  static void printhexln(nonstd::span<const std::uint8_t> data) { printhexln(data.data(), data.size()); }
  static void printhexln(const char* message, const std::uint8_t* data, std::size_t size);
  static void printhexln(const char* message, nonstd::span<const std::uint8_t> data) {
    printhexln(message, data.data(), data.size());
  }
};
