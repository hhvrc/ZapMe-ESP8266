#pragma once

#include "sdcard.hpp"

#include <nonstd/span.hpp>

#include <array>
#include <cstdint>

static constexpr std::size_t AES256_KEY_SZ = 32;
static constexpr std::size_t AES256_IV_SZ  = 16;
static constexpr std::size_t AES256_BLK_SZ = 16;

class CryptoFileReader {
public:
  CryptoFileReader(const char* path);
  ~CryptoFileReader();

  int read();

  std::size_t readBytes(char* data, std::size_t length);
  template<typename T>
  std::size_t readBytes(T* data, std::size_t length) {
    static_assert(sizeof(T) == 1, "T must be 1 byte in size");
    static_assert(std::is_standard_layout_v<T>, "T must be standard layout");
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    return readBytes(reinterpret_cast<char*>(data), length);
  }
  template<typename T>
  std::size_t readBytes(nonstd::span<T> data) {
    return readBytes(data.data(), data.size_bytes());
  }

  bool close() { return _file.close(); }

  operator bool() const { return _file.isReadable() || _bufferRead < _bufferWritten; }

private:
  void _alignBuffer();
  std::size_t _readIntoBuffer();
  bool _ensureReadBuffer(std::size_t length);

  constexpr std::size_t _bufferUsed() const { return _bufferWritten - _bufferRead; }

  constexpr std::size_t _bufferFree() const { return _buffer.size() - _bufferUsed(); }

  std::array<std::uint8_t, AES256_BLK_SZ * 32> _buffer;
  std::array<std::uint8_t, AES256_IV_SZ> _iv;
  std::size_t _bufferRead;
  std::size_t _bufferWritten;
  SDCardFile _file;
};

class CryptoFileWriter {
public:
  CryptoFileWriter(const char* path);
  ~CryptoFileWriter();

  std::size_t write(std::uint8_t data);
  template<typename T>
  std::size_t write(T data) {
    static_assert(sizeof(T) == 1, "T must be 1 byte in size");
    static_assert(std::is_standard_layout_v<T>, "T must be standard layout");
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    return write(reinterpret_cast<std::uint8_t>(data));
  }

  std::size_t write(const std::uint8_t* data, std::size_t length);
  template<typename T>
  std::size_t write(const T* data, std::size_t length) {
    static_assert(sizeof(T) == 1, "T must be 1 byte in size");
    static_assert(std::is_standard_layout_v<T>, "T must be standard layout");
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    return write(reinterpret_cast<const std::uint8_t*>(data), length);
  }
  template<typename T>
  std::size_t write(nonstd::span<const T> data) {
    return write(data.data(), data.size_bytes());
  }
  std::size_t write(const char* string) { return write(string, std::strlen(string)); }

  bool flush(bool closeFile = false);

  inline bool close() { return flush(true); }

  operator bool() const { return _file.isWritable(); }

private:
  std::array<std::uint8_t, AES256_BLK_SZ * 32> _buffer;
  std::array<std::uint8_t, AES256_IV_SZ> _iv;
  std::size_t _bufferWritten;
  std::size_t _fileWritten;
  SDCardFile _file;
};
