#include "logger.hpp"

#include "sdcard.hpp"

std::uint64_t LogIndex = 0;
char FileName[32] = {0};

Logger::InitializationError Logger::Initialize() {
  if (LogIndex != 0) {
    return InitializationError::None;
  }

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

void Logger::Log(const char* message) {
  auto sd = SDCard::GetInstance();
  if (!sd->ok()) {
    return;
  }

  FsFile file = sd->open(FileName, O_CREAT | O_APPEND | O_WRITE);
  if (!file) {
    return;
  }

  // Print the message as [DAYS:HOURS:MINUTES:SECONDS.MILLIS] message (0 padded 2 digits, except for millis which is 3 digits)
  std::uint64_t milli = millis();
  std::uint64_t seconds = milli / 1000;
  milli -= seconds * 1000;

  std::uint64_t minutes = seconds / 60;
  seconds -= minutes * 60;

  std::uint64_t hours = minutes / 60;
  minutes -= hours * 60;

  std::uint64_t days = hours / 24;
  hours -= days * 24;

  file.printf("[%02llu:%02llu:%02llu:%02llu.%03llu] ", days, hours, minutes, seconds, milli);
  file.println(message);
  file.close();
}
