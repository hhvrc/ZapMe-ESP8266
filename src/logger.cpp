#include "logger.hpp"

#include "sdcard.hpp"

constexpr bool LOG_TO_SERIAL = true;
#define SERIAL_BEGIN(...) if (LOG_TO_SERIAL) Serial.begin(__VA_ARGS__)
#define SERIAL_PRINT(...) if (LOG_TO_SERIAL) Serial.print(__VA_ARGS__)
#define SERIAL_PRINTF(...) if (LOG_TO_SERIAL) Serial.printf(__VA_ARGS__)
#define SERIAL_PRINTLN(...) if (LOG_TO_SERIAL) Serial.println(__VA_ARGS__)

std::uint64_t LogIndex = 0;
char FileName[32] = {0};

Logger::InitializationError Logger::Initialize() {
  if (LogIndex != 0) {
    return InitializationError::None;
  }

  SERIAL_BEGIN(115200);
  SERIAL_PRINTLN();

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
  std::uint64_t milli = millis(); \
  auto sd = SDCard::GetInstance(); \
  if (!sd->ok()) { \
    return; \
  } \
  FsFile file = sd->open(FileName, O_CREAT | O_APPEND | O_WRITE); \
  if (!file.isWritable()) { \
    return; \
  }

void PreLog(FsFile& file, std::uint64_t milli) {
  // Print the message as [DAYS:HOURS:MINUTES:SECONDS.MILLIS] message (0 padded 2 digits, except for millis which is 3 digits)
  std::uint64_t seconds = milli / 1000;
  milli -= seconds * 1000;

  std::uint64_t minutes = seconds / 60;
  seconds -= minutes * 60;

  std::uint64_t hours = minutes / 60;
  minutes -= hours * 60;

  std::uint64_t days = hours / 24;
  hours -= days * 24;

  file.printf("[%02llu:%02llu:%02llu:%02llu.%03llu] ", days, hours, minutes, seconds, milli);
  SERIAL_PRINTF("[%02llu:%02llu:%02llu:%02llu.%03llu] ", days, hours, minutes, seconds, milli);
}

void Logger::vprintlnf(const char* format, va_list args) {
  char temp[64];
  char* buffer = temp;
  std::size_t len = vsnprintf(temp, sizeof(temp), format, args);
  if (len > sizeof(temp) - 1) {
      buffer = new char[len + 1];
      if (!buffer) {
          return;
      }
      vsnprintf(buffer, len + 1, format, args);
  }

  Logger::println(buffer);

  if (buffer != temp) {
      delete[] buffer;
  }
}
void Logger::printlnf(const char* format, ...) {
  char temp[64];
  char* buffer = temp;
  va_list args;

  va_start(args, format);
  std::size_t len = vsnprintf(temp, sizeof(temp), format, args);
  va_end(args);

  if (len > sizeof(temp) - 1) {
      buffer = new char[len + 1];
      if (!buffer) {
          return;
      }

      va_start(args, format);
      vsnprintf(buffer, len + 1, format, args);
      va_end(args);
  }

  Logger::println(buffer);

  if (buffer != temp) {
      delete[] buffer;
  }
}
void Logger::println(const String& message) {
  GET_FILE
  PreLog(file, milli);
  file.println(message);
  SERIAL_PRINTLN(message);
  file.close();
}
void Logger::println(const char* message) {
  GET_FILE
  PreLog(file, milli);
  file.println(message);
  SERIAL_PRINTLN(message);
  file.close();
}
void Logger::println() {
  GET_FILE
  file.println();
  SERIAL_PRINTLN();
  file.close();
}

void Logger::printhexln(const char* message, const std::uint8_t* data, std::size_t size) {
  GET_FILE
  PreLog(file, milli);
  file.print(message);
  SERIAL_PRINT(message);
  for (std::size_t i = 0; i < size; i++) {
    file.printf("%02X", data[i]);
    SERIAL_PRINTF("%02X", data[i]);
  }
  file.println();
  SERIAL_PRINTLN();
  file.close();
}
void Logger::printhexln(const std::uint8_t* data, std::size_t size) {
  GET_FILE
  PreLog(file, milli);
  for (std::size_t i = 0; i < size; i++) {
    file.printf("%02X", data[i]);
    SERIAL_PRINTF("%02X", data[i]);
  }
  file.println();
  SERIAL_PRINTLN();
  file.close();
}
