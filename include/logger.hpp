#pragma once

class Logger {
  Logger() = delete;
public:
  enum class InitializationError {
    None,
    SDCardError,
    FileSystemError,
  };

  static InitializationError Initialize();
  static void Log(const char* message);
};
