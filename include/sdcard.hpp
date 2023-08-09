#pragma once

#include <SdFat.h>
#include <memory>

class SDCard {
  SDCard();
public:
  static std::shared_ptr<SDCard> GetInstance();

  ~SDCard();
  
  enum class SDCardError {
    None,
    InitializationFailed,
    RootDirectoryNotFound,
  };
  SDCardError error() const { return _error; }
  bool ok() const { return _error == SDCardError::None; }

  FsFile open(const char* path, oflag_t oflag = (oflag_t)0U);

  bool exists(const char* path);

  bool mkdir(const char* path);

  bool remove(const char* path);

  operator bool() const { return ok(); }
  
  SDCard(const SDCard&) = delete;
  SDCard& operator = (const SDCard&) = delete;
private:
  SdFs _sd;
  SDCardError _error;
};
