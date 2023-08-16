#pragma once

#include <nonstd/span.hpp>

#include <WString.h>

#include <cstdint>

class StringHelpers {
  StringHelpers() = delete;
public:
  static String FilledString(char c, std::size_t length);
  static String FromSpan(nonstd::span<const char> buffer);
  static nonstd::span<char> GetSpan(String& buffer);
  static nonstd::span<const char> GetConstSpan(const String& buffer);
};
