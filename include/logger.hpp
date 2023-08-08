#pragma once

namespace Logger {
  enum class InitializationError {
    None,
    SDCardError,
    FileSystemError,
  };

  InitializationError Initialize();
  void Log(const char* message);
}
