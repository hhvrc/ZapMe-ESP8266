#pragma once

#include "nonstd/span.hpp"

#include <SdFat.h>

#include <memory>

class SDCardFile {
  SDCardFile() : _sd(nullptr), _file() { }
  SDCardFile(std::shared_ptr<SdFs> sd, FsFile&& file) : _sd(sd), _file(std::move(file)) { }

  friend class SDCard;

public:
  ~SDCardFile() { }  // FsFile destructor already closes the file

  inline bool isDir() const { return _baseFile().isDir(); }
  inline bool isFile() const { return _baseFile().isFile(); }
  inline bool isOpen() const { return _baseFile().isOpen(); }
  inline bool isHidden() const { return _baseFile().isHidden(); }
  inline bool isReadable() const { return _baseFile().isReadable(); }
  inline bool isWritable() const { return _baseFile().isWritable(); }

  inline std::size_t size() const { return _baseFile().size(); }
  inline std::size_t position() const { return _baseFile().curPosition(); }

  inline bool seekBeg(std::size_t pos) { return _baseFile().seekSet(pos); }
  inline bool seekCur(std::size_t pos) { return _baseFile().seekCur(pos); }
  inline bool seekEnd(std::size_t pos) { return _baseFile().seekEnd(pos); }

  template<typename T>
  inline std::size_t read(T* buf, std::size_t nbyte) {
    static_assert(sizeof(T) == 1, "T must be 1 byte in size");
    static_assert(std::is_standard_layout_v<T>, "T must be standard layout");
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    return _baseFile().read(reinterpret_cast<std::uint8_t*>(buf), nbyte);
  }
  template<typename T>
  inline std::size_t read(nonstd::span<T> buf) {
    return read(buf.data(), buf.size_bytes());
  }
  template<typename T, std::size_t N>
  inline std::size_t read(std::array<T, N>& buf) {
    return read(buf.data(), buf.size());
  }

  template<typename T>
  inline std::size_t write(const T* buf, std::size_t nbyte) {
    static_assert(sizeof(T) == 1, "T must be 1 byte in size");
    static_assert(std::is_standard_layout_v<T>, "T must be standard layout");
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    return _baseFile().write(reinterpret_cast<const std::uint8_t*>(buf), nbyte);
  }
  template<typename T>
  inline std::size_t write(nonstd::span<const T> buf) {
    return write(buf.data(), buf.size_bytes());
  }
  template<typename T, std::size_t N>
  inline std::size_t write(const std::array<T, N>& buf) {
    return write(buf.data(), buf.size());
  }

  inline SDCardFile openNextFile(oflag_t mode = (oflag_t)0U) {
    FsFile file;
    file.openNext(&_file, mode);
    return SDCardFile(_sd, std::move(file));
  }

  inline std::size_t getName(char* name, std::size_t size) { return _baseFile().getName(name, size); }

  inline bool close() { return _baseFile().close(); }

  inline Stream& GetStream() { return _file; }

  inline operator bool() const { return isOpen(); }

private:
  constexpr FsBaseFile& _baseFile() { return _file; }
  constexpr const FsBaseFile& _baseFile() const { return _file; }

  std::shared_ptr<SdFs> _sd;
  FsFile _file;
};

class SDCard {
public:
  SDCard();
  ~SDCard();

  inline bool ok() const { return _sd != nullptr; }

  SDCardFile open(const char* path, oflag_t oflag = (oflag_t)0U);
  inline static SDCardFile Open(const char* path, oflag_t oflag = (oflag_t)0U) { return SDCard().open(path, oflag); }

  inline bool exists(const char* path) {
    if (!_sd) return false;
    return _sd->exists(path);
  }
  inline static bool Exists(const char* path) { return SDCard().exists(path); }

  inline bool mkdir(const char* path, bool mkParents = false) {
    if (!_sd) return false;
    return _sd->mkdir(path, mkParents);
  }
  inline static bool MkDir(const char* path, bool mkParents = false) { return SDCard().mkdir(path, mkParents); }

  inline bool remove(const char* path) {
    if (!_sd) return false;
    return _sd->remove(path);
  }
  inline static bool Remove(const char* path) { return SDCard().remove(path); }

  inline operator bool() const { return ok(); }

  SDCard(const SDCard&)            = delete;
  SDCard& operator=(const SDCard&) = delete;

private:
  std::shared_ptr<SdFs> _sd;
};
