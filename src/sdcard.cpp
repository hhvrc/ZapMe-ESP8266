#include "sdcard.hpp"

#include "resizable-buffer.hpp"

std::weak_ptr<SdFs> s_instance = std::weak_ptr<SdFs>();
std::shared_ptr<SdFs> GetInstance() {
  auto sd = s_instance.lock();
  if (!sd) {
    s_instance = sd = std::make_shared<SdFs>();
  }

  if (!sd) {
    return nullptr;
  }

  // Initialize the SD card
  // - Pin D8/GPIO15 is the SD card's CS pin
  // - We want to share the SPI bus with other devices
  // - We want to use the maximum SPI clock speed
  if (!sd->begin(SdSpiConfig(D8, SHARED_SPI, SD_SCK_MHZ(50)))) {
    return nullptr;
  }

  // Change to the root directory
  if (!sd->chdir("/")) {
    return nullptr;
  }

  return sd;
}

SDCard::SDCard() : _sd(GetInstance()) { }

SDCard::~SDCard() {
  _sd->end();
}

SDCardFile SDCard::open(const char* path, oflag_t oflag) {
  if (!_sd) {
    return SDCardFile();
  }

  if (oflag & O_CREAT) {
    std::size_t parentLen = std::strrchr(path, '/') - path;
    char* parentPath      = new char[parentLen + 1];
    std::memcpy(parentPath, path, parentLen);
    parentPath[parentLen] = '\0';

    if (!_sd->exists(parentPath)) {
      if (!_sd->mkdir(parentPath, true)) {
        // TODO: SERIAL LOG ERROR
        return SDCardFile();
      }
    }
  }

  FsFile file = _sd->open(path, oflag);
  if (!file) {
    return SDCardFile();
  }

  return SDCardFile(_sd, std::move(file));
}
