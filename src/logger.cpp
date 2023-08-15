#include "logger.hpp"

#include "sdcard.hpp"

constexpr bool LOG_TO_SERIAL = true;
#define SERIAL_BEGIN(...) \
  if (LOG_TO_SERIAL) { \
    Serial.begin(__VA_ARGS__); \
  }
#define SERIAL_PRINT(buf) \
  if (LOG_TO_SERIAL) { \
    Serial.print(buf); \
  }
#define SERIAL_PRINTS(buf, len) \
  if (LOG_TO_SERIAL) { \
    Serial.write(buf, len); \
  }
#define SERIAL_PRINTF(...) \
  if (LOG_TO_SERIAL) { \
    Serial.printf(__VA_ARGS__); \
  }
#define SERIAL_PRINTLN(buf) \
  if (LOG_TO_SERIAL) { \
    Serial.println(buf); \
  }
#define SERIAL_PRINTELN() \
  if (LOG_TO_SERIAL) { \
    Serial.println(); \
  }
#define SERIAL_PRINTSLN(buf, len) \
  if (LOG_TO_SERIAL) { \
    Serial.write(buf, len); \
    Serial.println(); \
  }

#define LOGGER_PRINT(buf) \
  SERIAL_PRINT(buf) \
  file.print(buf)
#define LOGGER_PRINTS(buf, len) \
  SERIAL_PRINTS(buf, len) \
  file.write(buf, len)
#define LOGGER_PRINTF(...) \
  SERIAL_PRINTF(__VA_ARGS__) \
  file.printf(__VA_ARGS__)
#define LOGGER_PRINTLN(buf) \
  SERIAL_PRINTLN(buf) \
  file.println(buf)
#define LOGGER_PRINTELN() \
  SERIAL_PRINTELN(buf) \
  file.println()
#define LOGGER_PRINTSLN(buf, len) \
  SERIAL_PRINTSLN(buf, len) \
  file.write(buf, len); \
  file.println()

std::uint64_t LogIndex = 0;
char FileName[32] = {0};

Logger::InitializationError Logger::Initialize() {
  if (LogIndex != 0) {
    return InitializationError::None;
  }

  SERIAL_BEGIN(115200);
  SERIAL_PRINTELN();

  auto sd = SDCard::GetInstance();
  if (!sd->ok()) {
    return InitializationError::SDCardError;
  }

  // Get the name of the latest log file
  sd->mkdir("/log");
  FsFile logDir = sd->open("/log", O_READ);
  if (!logDir) {
    return InitializationError::FileSystemError;
  }

  // Get the id of the latest log file
  while (true) {
    FsFile file = logDir.openNextFile();
    if (!file) {
      break;
    }
    if (!file.isFile()) {
      continue;
    }

    file.getName(FileName, sizeof(FileName));
    std::uint64_t index = 0;
    if (sscanf(FileName, "log_%llu.txt", &index) == 1) {
      if (index > LogIndex) {
        LogIndex = index;
      }
    }
    file.close();
  }

  // Set the name of the next log file
  LogIndex++;
  sprintf(FileName, "/log/log_%llu.txt", LogIndex);

  return InitializationError::None;
}

#define GET_FILE \
  auto sd = SDCard::GetInstance(); \
  if (!sd->ok()) { \
    return; \
  } \
  FsFile file = sd->open(FileName, O_CREAT | O_APPEND | O_WRITE); \
  if (!file.isWritable()) { \
    return; \
  }

int FormatTimestamp(char* buffer, std::size_t bufferSize, std::uint64_t millis) {
  std::uint64_t seconds = millis / 1000;
  millis -= seconds * 1000;

  std::uint64_t minutes = seconds / 60;
  seconds -= minutes * 60;

  std::uint64_t hours = minutes / 60;
  minutes -= hours * 60;

  std::uint64_t days = hours / 24;
  hours -= days * 24;

  return snprintf(buffer, bufferSize, "[%02hu:%02hhu:%02hhu:%02hhu.%03hu] ",
    static_cast<std::uint16_t>(days),
    static_cast<std::uint8_t>(hours),
    static_cast<std::uint8_t>(minutes),
    static_cast<std::uint8_t>(seconds),
    static_cast<std::uint16_t>(millis)
  );
}
int PrintTimestamp(FsFile& file, std::uint64_t millis) {
  char buffer[32];
  int len = FormatTimestamp(buffer, sizeof(buffer), millis);
  LOGGER_PRINTS(buffer, len);
  return len;
}

void Logger::vprintlnf(const char* format, va_list args) {
  std::uint64_t milli = millis();
  GET_FILE

  char buffer[64];
  char* ptr = buffer;

  int tsLen = FormatTimestamp(buffer, sizeof(buffer), milli);
  int logLen = vsnprintf(buffer + tsLen, sizeof(buffer) - tsLen, format, args);

  int len = tsLen + logLen;
  if (len > static_cast<int>(sizeof(buffer)) - 1) {
    ptr = new char[len + 1];
    if (!ptr) {
      return;
    }

    tsLen = FormatTimestamp(ptr, len + 1, milli);
    logLen = vsnprintf(ptr + tsLen, len + 1, format, args);
    len = tsLen + logLen;
  }

  LOGGER_PRINTSLN(ptr, len);

  if (ptr != buffer) {
      delete[] ptr;
  }

  file.close();
}
void Logger::printlnf(const char* format, ...) {
  std::uint64_t milli = millis();
  GET_FILE

  char buffer[64];
  char* ptr = buffer;

  int tsLen = FormatTimestamp(buffer, sizeof(buffer), milli);

  va_list args;
  va_start(args, format);
  int logLen = vsnprintf(buffer + tsLen, sizeof(buffer) - tsLen, format, args);
  va_end(args);

  int len = tsLen + logLen;
  if (len > static_cast<int>(sizeof(buffer)) - 1) {
    ptr = new char[len + 1];
    if (!ptr) {
      return;
    }

    tsLen = FormatTimestamp(ptr, len + 1, milli);
    va_start(args, format);
    logLen = vsnprintf(ptr + tsLen, len + 1, format, args);
    va_end(args);
    len = tsLen + logLen;
  }

  LOGGER_PRINTSLN(ptr, len);

  if (ptr != buffer) {
      delete[] ptr;
  }

  file.close();
}
void Logger::println(const String& message) {
  std::uint64_t milli = millis();
  GET_FILE
  PrintTimestamp(file, milli);
  LOGGER_PRINTLN(message);
  file.close();
}
void Logger::println(const char* message) {
  std::uint64_t milli = millis();
  GET_FILE
  PrintTimestamp(file, milli);
  LOGGER_PRINTLN(message);
  file.close();
}
void Logger::println() {
  GET_FILE
  LOGGER_PRINTLN();
  file.close();
}

constexpr char hexfmtnibble(std::uint8_t data) {
  data &= 0x0F;
  if (data < 10) {
    return '0' + data;
  }
  return 'A' + (data - 10);
}
std::size_t hexfmt(char* buffer, std::size_t bufferLen, const std::uint8_t* data, std::size_t size) {
  const std::size_t hexlen = size * 2;

  if (bufferLen < hexlen) {
    return hexlen;
  }

  for (std::size_t i = 0; i < size; i++) {
    buffer[(i * 2) + 0] = hexfmtnibble(data[i] >> 4);
    buffer[(i * 2) + 1] = hexfmtnibble(data[i] >> 0);
  }

  return hexlen;
}

void Logger::printhexln(const std::uint8_t* data, std::size_t size) {
  std::uint64_t milli = millis();
  GET_FILE

  char buffer[64];
  char* ptr = buffer;

  int tsLen = FormatTimestamp(buffer, sizeof(buffer), milli);
  int hexLen = size * 2;
  int len = tsLen + hexLen + 2;

  if (len > static_cast<int>(sizeof(buffer)) - 1) {
    ptr = new char[len + 1];
    if (!ptr) {
      return;
    }

    tsLen = FormatTimestamp(ptr, len + 1, milli);
    hexLen = hexfmt(ptr + tsLen, len + 1, data, size);
  } else {
    hexLen = hexfmt(buffer + tsLen, sizeof(buffer) - tsLen, data, size);
  }
  buffer[tsLen + hexLen + 0] = '\r';
  buffer[tsLen + hexLen + 1] = '\n';
  len = tsLen + hexLen + 2;

  LOGGER_PRINTS(buffer, len);

  if (ptr != buffer) {
      delete[] ptr;
  }

  file.close();
}
void Logger::printhexln(const char* message, const std::uint8_t* data, std::size_t size) {
  std::uint64_t milli = millis();
  GET_FILE

  char buffer[64];
  char* ptr = buffer;

  int tsLen = FormatTimestamp(buffer, sizeof(buffer), milli);
  int msgLen = strlen(message);
  int hexLen = size * 2;
  int len = tsLen + msgLen + hexLen + 2;

  if (len > static_cast<int>(sizeof(buffer)) - 1) {
    ptr = new char[len + 1];
    if (!ptr) {
      return;
    }

    tsLen = FormatTimestamp(ptr, len + 1, milli);
    memcpy(ptr + tsLen, message, msgLen);
    hexLen = hexfmt(ptr + tsLen + msgLen, len + 1, data, size);
  } else {
    memcpy(buffer + tsLen, message, msgLen);
    hexLen = hexfmt(buffer + tsLen + msgLen, sizeof(buffer) - tsLen - msgLen, data, size);
  }

  buffer[tsLen + msgLen + hexLen + 0] = '\r';
  buffer[tsLen + msgLen + hexLen + 1] = '\n';
  len = tsLen + msgLen + hexLen + 2;

  LOGGER_PRINTS(buffer, len);

  if (ptr != buffer) {
      delete[] ptr;
  }

  file.close();
}
