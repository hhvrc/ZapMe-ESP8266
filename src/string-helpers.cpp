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

  ExtendedString(nonstd::span<const char> buffer)
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

String StringHelpers::FromSpan(nonstd::span<const char> buffer) {
  return ExtendedString(buffer);
}

nonstd::span<char> StringHelpers::GetSpan(String& buffer) {
  return nonstd::span<char>(buffer.begin(), buffer.length());
}

nonstd::span<const char> StringHelpers::GetConstSpan(const String& buffer) {
  return nonstd::span<const char>(buffer.begin(), buffer.length());
}
