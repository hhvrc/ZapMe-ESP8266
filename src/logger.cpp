#include "logger.hpp"

#include "resizable-buffer.hpp"
#include "sdcard.hpp"

#define LOG_TO_SERIAL true
#define SERIAL_BEGIN(...)      \
  if (LOG_TO_SERIAL) {         \
    Serial.begin(__VA_ARGS__); \
  }
#define SERIAL_PRINT(buf) \
  if (LOG_TO_SERIAL) {    \
    Serial.print(buf);    \
  }
#define SERIAL_PRINTS(buf, len) \
  if (LOG_TO_SERIAL) {          \
    Serial.write(buf, len);     \
  }
#define SERIAL_PRINTF(...)      \
  if (LOG_TO_SERIAL) {          \
    Serial.printf(__VA_ARGS__); \
  }
#define SERIAL_PRINTLN(buf) \
  if (LOG_TO_SERIAL) {      \
    Serial.println(buf);    \
  }
#define SERIAL_PRINTELN() \
  if (LOG_TO_SERIAL) {    \
    Serial.println();     \
  }
#define SERIAL_SPRINTLN(buf, len) \
  if (LOG_TO_SERIAL) {            \
    Serial.write(buf, len);       \
    Serial.println();             \
  }

#define LOGGER_PRINT(buf) \
  SERIAL_PRINT(buf)       \
  file.print(buf)
#define LOGGER_PRINTS(buf, len) \
  SERIAL_PRINTS(buf, len)       \
  file.write(buf, len)
#define LOGGER_PRINTF(...)   \
  SERIAL_PRINTF(__VA_ARGS__) \
  file.printf(__VA_ARGS__)
#define LOGGER_PRINTLN(buf) \
  SERIAL_PRINTLN(buf)       \
  file.println(buf)
#define LOGGER_PRINTELN() \
  SERIAL_PRINTELN(buf)    \
  file.println()
#define LOGGER_SPRINTLN(buf, len) \
  SERIAL_SPRINTLN(buf, len)       \
  file.write(buf, len);           \
  file.println()

char* LogPath = nullptr;

bool InitializeLogPath() {
  static char FileName[40] {0};
  static std::uint32_t LogIndex    = 0;
  static std::uint32_t BucketIndex = 0;

  if (LogIndex != 0 && BucketIndex != 0) {
    if (LogPath == nullptr) {
      sprintf(FileName, "/log/%u/log_%u.txt", BucketIndex, LogIndex);
      LogPath = FileName;
    }

    return true;
  }

  auto sd = SDCard::GetInstance();
  if (sd == nullptr) {
    return false;
  }

  // Get the name of the latest bucket
  FsFile logDir = sd->open("/log", O_READ);
  if (!logDir) {
    sd->mkdir("/log");
    logDir = sd->open("/log", O_READ);
    if (!logDir) {
      SERIAL_PRINTLN("[Logger] Failed to create log directory");
      return false;
    }
  }
  while (true) {
    FsFile file = logDir.openNextFile();
    if (!file) {
      break;
    }

    if (!file.isFile()) {
      file.getName(FileName, sizeof(FileName));
      std::uint32_t index = 0;
      if (sscanf(FileName, "%u", &index) > 0) {
        SERIAL_PRINTF("[Logger] Found bucket: %u\n", index);
        if (index > BucketIndex) {
          BucketIndex = index;
        }
      }
    } else {
      SERIAL_PRINTF("[Logger] Unexpected file in log directory: %s\n", FileName);
    }
    file.close();
  }
  logDir.close();
  if (BucketIndex == 0) {
    BucketIndex = 1;
  }

  // Get the id of the latest log file
  sprintf(FileName, "/log/%u", BucketIndex);
  logDir = sd->open(FileName, O_READ);
  if (!logDir) {
    sd->mkdir(FileName);
    logDir = sd->open(FileName, O_READ);
    if (!logDir) {
      SERIAL_PRINTLN("[Logger] Failed to create log bucket");
      return false;
    }
  }
  while (true) {
    FsFile file = logDir.openNextFile();
    if (!file) {
      break;
    }

    if (file.isFile()) {
      SERIAL_PRINTF("[Logger] Found log file: %s\n", FileName);
      file.getName(FileName, sizeof(FileName));
      std::uint32_t index = 0;
      if (sscanf(FileName, "log_%u.txt", &index) > 0) {
        if (index > LogIndex) {
          LogIndex = index;
        }
      }
    } else {
      SERIAL_PRINTF("[Logger] Unexpected directory in log bucket: %s\n", FileName);
    }
    file.close();
  }
  logDir.close();

  // Set the name of the next log file
  if (LogIndex >= 100) {
    LogIndex = 1;
    BucketIndex++;
  } else {
    LogIndex++;
  }

  sprintf(FileName, "/log/%u/log_%u.txt", BucketIndex, LogIndex);
  LogPath = FileName;

  return true;
}

Logger::InitializationError Logger::Initialize() {
  if (LogPath != nullptr) {
    return InitializationError::None;
  }

  SERIAL_BEGIN(115'200);
  SERIAL_PRINTELN();

  auto sd = SDCard::GetInstance();
  if (!sd->ok()) {
    return InitializationError::SDCardError;
  }

  if (!InitializeLogPath()) {
    return InitializationError::FileSystemError;
  }

  return InitializationError::None;
}

#define GET_FILE                                                     \
  if (LogPath == nullptr) {                                          \
    if (Logger::Initialize() != Logger::InitializationError::None) { \
      return;                                                        \
    }                                                                \
  }                                                                  \
  auto sd = SDCard::GetInstance();                                   \
  if (!sd->ok()) {                                                   \
    return;                                                          \
  }                                                                  \
  FsFile file = sd->open(LogPath, O_CREAT | O_APPEND | O_WRITE);     \
  if (!file.isWritable()) {                                          \
    return;                                                          \
  }

constexpr const char* TS_FORMAT         = "[%02hu:%02hhu:%02hhu:%02hhu.%03hu] ";
constexpr std::size_t TS_FORMAT_MAX_LEN = 20;

int FormatTimestamp(char* buffer, std::size_t bufferSize, std::uint64_t millis) {
  std::uint64_t seconds = millis / 1000;
  millis -= seconds * 1000;

  std::uint64_t minutes = seconds / 60;
  seconds -= minutes * 60;

  std::uint64_t hours = minutes / 60;
  minutes -= hours * 60;

  std::uint64_t days = hours / 24;
  hours -= days * 24;

  return snprintf(buffer,
                  bufferSize,
                  TS_FORMAT,
                  static_cast<std::uint16_t>(days),
                  static_cast<std::uint8_t>(hours),
                  static_cast<std::uint8_t>(minutes),
                  static_cast<std::uint8_t>(seconds),
                  static_cast<std::uint16_t>(millis));
}

int PrintTimestamp(FsFile& file, std::uint64_t millis) {
  char buffer[32];
  int tsLen = FormatTimestamp(buffer, sizeof(buffer), millis);
  if (tsLen <= 0) return tsLen;
  LOGGER_PRINTS(buffer, tsLen);
  return tsLen;
}

void Logger::vprintlnf(const char* format, va_list args) {
  std::uint64_t milli = millis();
  GET_FILE

  ResizableBuffer<char, 64> buffer = ResizableBuffer<char, 64>();

  int tsLen = FormatTimestamp(buffer.ptr(), buffer.size(), milli);
  if (tsLen <= 0) {
    file.close();
    return;
  }
  int logLen = vsnprintf(buffer.ptr() + tsLen, buffer.size() - tsLen, format, args);

  int len = tsLen + logLen;
  if (len > static_cast<int>(buffer.size()) - 1) {
    buffer.resize(len + 1);

    tsLen = FormatTimestamp(buffer.ptr(), len + 1, milli);
    if (tsLen <= 0) {
      file.close();
      return;
    }

    logLen = vsnprintf(buffer.ptr() + tsLen, len + 1, format, args);

    len = tsLen + logLen;
  }

  LOGGER_SPRINTLN(buffer.ptr(), len);

  file.close();
}

void Logger::printlnf(const char* format, ...) {
  std::uint64_t milli = millis();
  GET_FILE

  ResizableBuffer<char, 64> buffer = ResizableBuffer<char, 64>();

  int tsLen = FormatTimestamp(buffer.ptr(), buffer.size(), milli);
  if (tsLen <= 0) {
    file.close();
    return;
  }

  va_list args;
  va_start(args, format);
  int logLen = vsnprintf(buffer.ptr() + tsLen, buffer.size() - tsLen, format, args);
  va_end(args);

  int len = tsLen + logLen;
  if (len > static_cast<int>(buffer.size()) - 1) {
    buffer.resize(len + 1);

    tsLen = FormatTimestamp(buffer.ptr(), len + 1, milli);
    if (tsLen <= 0) {
      file.close();
      return;
    }

    va_start(args, format);
    logLen = vsnprintf(buffer.ptr() + tsLen, len + 1, format, args);
    va_end(args);

    len = tsLen + logLen;
  }

  LOGGER_SPRINTLN(buffer.ptr(), len);

  file.close();
}

void Logger::println(const String& message) {
  std::uint64_t milli = millis();
  GET_FILE
  int tsLen = PrintTimestamp(file, milli);
  if (tsLen <= 0) {
    file.close();
    return;
  }
  LOGGER_PRINTLN(message);
  file.close();
}

void Logger::println(const char* message) {
  if (message == nullptr || message[0] == '\0') return;
  std::uint64_t milli = millis();
  GET_FILE
  int tsLen = PrintTimestamp(file, milli);
  if (tsLen <= 0) {
      file.close();
      return;
  }
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

void _printhexln(const char* message, const std::uint8_t* data, std::size_t size) {
  if (data == nullptr || size <= 0 || size > 4096) {
    return;
  }

  std::uint64_t milli = millis();
  GET_FILE

  std::size_t strLen = 0;
  if (message != nullptr) {
    strLen = std::strlen(message);
  }

  std::size_t hexLen     = size * 2;
  std::size_t bufferSize = TS_FORMAT_MAX_LEN + strLen + hexLen + 2;
  char* buffer           = new char[bufferSize];

  int tsLen = FormatTimestamp(buffer, bufferSize, milli);
  if (tsLen <= 0 || static_cast<std::size_t>(tsLen) > TS_FORMAT_MAX_LEN) {
    delete[] buffer;
    file.close();
    return;
  }

  if (message != nullptr) {
    std::memcpy(buffer + tsLen, message, strLen);
  }
  hexfmt(buffer + tsLen + strLen, bufferSize - tsLen, data, size);
  buffer[tsLen + strLen + hexLen + 0] = '\r';
  buffer[tsLen + strLen + hexLen + 1] = '\n';
  buffer[tsLen + strLen + hexLen + 2] = '\0';

  LOGGER_PRINTS(buffer, tsLen + strLen + hexLen + 2);

  delete[] buffer;

  file.close();
}

void Logger::printhexln(const std::uint8_t* data, std::size_t size) {
  _printhexln(nullptr, data, size);
}

void Logger::printhexln(const char* message, const std::uint8_t* data, std::size_t size) {
  _printhexln(message, data, size);
}
