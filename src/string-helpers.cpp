#include "string-helpers.hpp"

class ExtendedString : public String {
public:
  ExtendedString(char c, std::size_t length)
  {
    init();
    setLen(length);
    memset(wbuffer(), c, length);
    wbuffer()[length] = 0;
  }

  ExtendedString(std::span<const char> buffer)
  {
    init();
    setLen(buffer.size());
    memcpy_P(wbuffer(), (PGM_P)buffer.data(), buffer.size());
    wbuffer()[buffer.size()] = 0;
  }
};

String StringHelpers::FilledString(char c, std::size_t length) {
  return ExtendedString(c, length);
}

String StringHelpers::FromSpan(std::span<const char> buffer) {
  return ExtendedString(buffer);
}

std::span<char> StringHelpers::GetSpan(String& buffer) {
  return std::span<char>(buffer.begin(), buffer.length());
}

std::span<const char> StringHelpers::GetConstSpan(const String& buffer) {
  return std::span<const char>(buffer.begin(), buffer.length());
}
