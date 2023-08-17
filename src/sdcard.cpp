#include "sdcard.hpp"

std::weak_ptr<SDCard> s_instance = std::weak_ptr<SDCard>();

SDCard::SDCard() : _sd(), _error(SDCardError::None) {
  // Initialize the SD card (Pin D8/GPIO15 is the SD card's CS pin, we want to share the SPI bus with other devices, and we want
  // to use the maximum SPI clock speed)
  if (!_sd.begin(SdSpiConfig(D8, SHARED_SPI, SD_SCK_MHZ(50)))) {
    _error = SDCardError::InitializationFailed;
    return;
  }

  // Change to the root directory
  if (!_sd.chdir("/")) {
    _error = SDCardError::RootDirectoryNotFound;
    return;
  }
}

std::shared_ptr<SDCard> SDCard::GetInstance() {
  auto instance = s_instance.lock();
  if (!instance) {
    instance   = std::shared_ptr<SDCard>(new SDCard());
    s_instance = instance;
  }
  return instance;
}

SDCard::~SDCard() {
  _sd.end();
}

FsFile SDCard::open(const char* path, oflag_t oflag) {
  if (_error != SDCardError::None) {
    return FsFile();
  }

  return _sd.open(path, oflag);
}

bool SDCard::exists(const char* path) {
  if (_error != SDCardError::None) {
    return false;
  }

  return _sd.exists(path);
}

bool SDCard::mkdir(const char* path) {
  if (_error != SDCardError::None) {
    return false;
  }

  return _sd.mkdir(path);
}

bool SDCard::remove(const char* path) {
  if (_error != SDCardError::None) {
    return false;
  }

  return _sd.remove(path);
}
