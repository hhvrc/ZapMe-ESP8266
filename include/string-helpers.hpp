#pragma once

#include <WString.h>

#include <span>
#include <cstdint>

class StringHelpers {
  StringHelpers() = delete;
public:
  static String FilledString(char c, std::size_t length);
  static String FromSpan(std::span<const char> buffer);
  static std::span<char> GetSpan(String& buffer);
  static std::span<const char> GetConstSpan(const String& buffer);
};
