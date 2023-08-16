#pragma once

#include <bearssl/bearssl_block.h>
#include <SdFat.h>

#include <array>
#include <cstdint>

static constexpr std::size_t AES256_KEY_SZ = 32;
static constexpr std::size_t AES256_IV_SZ = 16;
static constexpr std::size_t AES256_BLK_SZ = br_aes_ct_BLOCK_SIZE;

class CryptoFileReader
{
public:
  CryptoFileReader(const char* path);
  ~CryptoFileReader();

  int read();
  std::size_t readBytes(char* data, std::size_t length);
  std::size_t readBytes(std::uint8_t* data, std::size_t length) {
    return readBytes(reinterpret_cast<char*>(data), length);
  }

  void close() {
    _file.close();
  }

  operator bool() const {
    return _file.isReadable() || _bufferRead < _bufferWritten;
  }
private:
  void _alignBuffer();
  std::size_t _readIntoBuffer();
  bool _ensureReadBuffer(std::size_t length);
  constexpr std::size_t _bufferUsed() const {
    return _bufferWritten - _bufferRead;
  }
  constexpr std::size_t _bufferFree() const {
    return _buffer.size() - _bufferUsed();
  }

  std::array<std::uint8_t, AES256_BLK_SZ * 32> _buffer;
  std::array<std::uint8_t, AES256_IV_SZ> _iv;
  std::size_t _bufferRead;
  std::size_t _bufferWritten;
  FsFile _file;
};

class CryptoFileWriter
{
public:
  CryptoFileWriter(const char* path);
  ~CryptoFileWriter();

  std::size_t write(std::uint8_t data);
  std::size_t write(const std::uint8_t* data, std::size_t length);
  std::size_t write(const char* data, std::size_t length) {
    return write(reinterpret_cast<const std::uint8_t*>(data), length);
  }
  std::size_t write(const char* string) {
    return write(string, std::strlen(string));
  }
  std::size_t write(const String& string) {
    return write(string.c_str(), string.length());
  }

  void flush() {
    _flush(false);
  }

  void close() {
    _flush(true);
    _file.close();
  }

  operator bool() const {
    return _file.isWritable();
  }
private:
  void _flush(bool final);

  std::array<std::uint8_t, AES256_BLK_SZ * 32> _buffer;
  std::array<std::uint8_t, AES256_IV_SZ> _iv;
  std::size_t _bufferWritten;
  std::size_t _fileWritten;
  FsFile _file;
};
