#include "sdcard.hpp"

#include "resizable-buffer.hpp"

std::weak_ptr<SDCard> s_instance = std::weak_ptr<SDCard>();

bool _ensureDir(SdFs& sd, char* path, std::size_t pathLen) {
  if (pathLen == 0) {
    return true;
  }

  char* end = path + pathLen;

  for (char* pos = *path == '/' ? path + 1 : path; pos < end; pos++) {
    pos  = std::find(pos, end, '/');
    *pos = '\0';

    if (!sd.exists(path)) {
      if (!sd.mkdir(path)) {
        // TODO: SERIAL LOG ERROR
        return false;
      }
    }

    *pos = '/';
  }

  return true;
}

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

  if (oflag & O_CREAT) {
    std::size_t parentLen = std::strrchr(path, '/') - path;

    ResizableBuffer<char, 64> buffer = ResizableBuffer<char, 64>(parentLen + 1);

    std::memcpy(buffer.ptr(), path, parentLen);
    buffer.ptr()[parentLen] = '\0';

    if (!_sd.mkdir(buffer.ptr())) {
      if (!_ensureDir(_sd, buffer.ptr(), parentLen)) {
        // TODO: SERIAL LOG ERROR
        return FsFile();
      }
    }
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

  if (_sd.exists(path)) {
    return true;
  }

  if (_sd.mkdir(path)) {
    return true;
  }

  std::size_t pathLen = std::strlen(path);

  ResizableBuffer<char, 64> buffer = ResizableBuffer<char, 64>(pathLen + 1);

  std::memcpy(buffer.ptr(), path, pathLen);

  return _ensureDir(_sd, buffer.ptr(), pathLen);
}

bool SDCard::remove(const char* path) {
  if (_error != SDCardError::None) {
    return false;
  }

  return _sd.remove(path);
}
