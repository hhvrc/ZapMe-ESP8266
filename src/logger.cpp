#include "logger.hpp"

#include "sdcard.hpp"

std::uint64_t LogIndex = 0;

Logger::InitializationError Logger::Initialize() {
  if (LogIndex != 0) {
    return InitializationError::None;
  }

  auto [sd, sdError] = SDCard();
  if (sdError != SDCardError::None) {
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

    char fileName[64];
    file.getName(fileName, sizeof(fileName));
    std::uint64_t index = 0;
    if (sscanf(fileName, "log_%u.txt", &index) == 1) {
      if (index > LogIndex) {
        LogIndex = index;
      }
    }
    file.close();
  }

  LogIndex++;

  return InitializationError::None;
}

void Logger::Log(const char* message) {
  auto [sd, sdError] = SDCard();
  if (sdError != SDCardError::None) {
    return;
  }

  char fileName[64];
  sprintf(fileName, "/log/log_%u.txt", LogIndex);

  FsFile file = sd->open(fileName, O_CREAT | O_APPEND | O_WRITE);
  if (!file) {
    return;
  }

  file.println(message);
  file.close();
}
