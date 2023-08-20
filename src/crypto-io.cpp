#include "crypto-io.hpp"

#include "crypto-utils.hpp"
#include "logger.hpp"
#include "sdcard.hpp"

#include <bearssl/bearssl_block.h>
#include <CRC32.h>
#include <EEPROM.h>
#include <SdFat.h>

#include <array>
#include <cstdint>
#include <memory>

static_assert(AES256_BLK_SZ == br_aes_big_BLOCK_SIZE, "AES256 block size mismatch");

constexpr std::size_t FILE_ID_SIZE = 16;
std::array<char, 4> EEPROM_HEADER  = {'A', 'E', 'S', 'K'};

struct CryptoConfig {
  std::array<char, 4> header;
  std::array<std::uint8_t, FILE_ID_SIZE> fileID;
  std::array<std::uint8_t, AES256_KEY_SZ> key;
  std::uint32_t checksum;

  std::uint32_t calculateChecksum() const {
    CRC32 crc;
    crc.update(header.data(), header.size());
    crc.update(fileID.data(), fileID.size());
    crc.update(key.data(), key.size());
    return crc.finalize();
  }

  void initialize() {
    std::memcpy(header.data(), EEPROM_HEADER.data(), EEPROM_HEADER.size());
    CryptoUtils::RandomBytes(fileID);
    CryptoUtils::RandomBytes(key);
    checksum = calculateChecksum();
  }

  bool validate() const {
    if (!std::equal(header.begin(), header.end(), EEPROM_HEADER.begin())) {
      Logger::println("[CryptoConfig] Invalid header");
      return false;
    }

    std::uint32_t calculatedChecksum = calculateChecksum();
    if (checksum != calculatedChecksum) {
      Logger::printlnf("[CryptoConfig] Invalid checksum: %08X != %08X", checksum, calculatedChecksum);
      return false;
    }

    return true;
  }
};

struct CryptoContext {
  br_aes_big_cbcenc_keys encKeys;
  br_aes_big_cbcdec_keys decKeys;
  std::array<std::uint8_t, FILE_ID_SIZE> fileID;

  bool verifyFileID(const std::array<std::uint8_t, FILE_ID_SIZE>& fileID) const {
    return std::memcmp(fileID.data(), this->fileID.data(), FILE_ID_SIZE) == 0;
  }

  void encrypt(std::uint8_t* data, std::size_t length, std::array<std::uint8_t, AES256_IV_SZ>& iv) {
    br_aes_big_cbcenc_run(&encKeys, iv.data(), data, length);
  }

  void decrypt(std::uint8_t* data, std::size_t length, std::array<std::uint8_t, AES256_IV_SZ>& iv) {
    br_aes_big_cbcdec_run(&decKeys, iv.data(), data, length);
  }
};

std::unique_ptr<CryptoContext> s_cryptCtx;

void Initialize() {
  if (s_cryptCtx) {
    return;
  }
  s_cryptCtx = std::make_unique<CryptoContext>();

  EEPROM.begin(sizeof(CryptoConfig));
  const CryptoConfig* constConf = reinterpret_cast<const CryptoConfig*>(EEPROM.getDataPtr());

  if (!constConf->validate()) {
    Logger::println("Invalid CryptoConfig, initializing and writing to EEPROM");
    reinterpret_cast<CryptoConfig*>(EEPROM.getDataPtr())->initialize();
  }

  br_aes_big_cbcenc_init(&s_cryptCtx->encKeys, constConf->key.data(), constConf->key.size());
  br_aes_big_cbcdec_init(&s_cryptCtx->decKeys, constConf->key.data(), constConf->key.size());
  std::memcpy(s_cryptCtx->fileID.data(), constConf->fileID.data(), constConf->fileID.size());

  EEPROM.end();
}

CryptoFileReader::CryptoFileReader(const char* path)
  : _buffer(), _iv(), _bufferRead(0), _bufferWritten(0), _file(SDCard::Open(path, O_READ)) {
  std::size_t fileSize = _file.size();
  if (!_file.isReadable() || fileSize < FILE_ID_SIZE + _iv.size() || (fileSize & 0xFULL) != 0) {
    Logger::printlnf("[CryptoFileReader] Cannot read file \"%s\", readable: %s, size: %d, aligned: %s",
                     path,
                     _file.isReadable() ? "true" : "false",
                     fileSize,
                     (fileSize & 0xFULL) == 0 ? "true" : "false");
    close();
    return;
  }

  Initialize();

  // Verify file ID
  std::array<std::uint8_t, FILE_ID_SIZE> fileID;
  _file.read(fileID);
  if (!s_cryptCtx->verifyFileID(fileID)) {
    Logger::printlnf(
      "[CryptoFileReader] File \"%s\" has different file ID, encryption keys are different and decryption will fail. Aborting.",
      path);
    close();
    return;
  }

  // Read IV
  _file.read(_iv);

  // Fill buffer
  _readIntoBuffer();
}

CryptoFileReader::~CryptoFileReader() {
  close();
}

int CryptoFileReader::read() {
  if (!this->operator bool()) {
    return -1;
  }

  if (!_ensureReadBuffer(1)) {
    return -1;
  }

  return _buffer[_bufferRead++];
}

std::size_t CryptoFileReader::readBytes(char* data, std::size_t length) {
  if (!this->operator bool()) {
    return 0;
  }

  std::size_t nRead = 0;
  while (nRead < length) {
    std::size_t toRead = std::min(length - nRead, _bufferUsed());
    if (toRead == 0) {
      if (_readIntoBuffer() == 0) {
        break;
      }
      continue;
    }
    std::memcpy(data + nRead, _buffer.data() + _bufferRead, toRead);
    _bufferRead += toRead;
    nRead += toRead;
  }

  return nRead;
}

void CryptoFileReader::_alignBuffer() {
  if (_bufferRead > 0) {
    std::memmove(_buffer.data(), _buffer.data() + _bufferRead, _bufferUsed());
    _bufferWritten -= _bufferRead;
    _bufferRead = 0;
  }
}

std::size_t CryptoFileReader::_readIntoBuffer() {
  if (!_file.isReadable()) {
    return 0;
  }

  // Check buffer space
  std::size_t fileSize     = _file.size();
  std::size_t position     = _file.position();
  std::size_t fileSizeLeft = fileSize - position;

  std::size_t toRead = std::min(_bufferFree(), fileSizeLeft) & ~0xFULL;

  if (toRead == 0) {
    Logger::printlnf("[CryptoFileReader] Not enough space in buffer (%d) or file (%d) left to read a block (%d)",
                     _bufferFree(),
                     fileSizeLeft,
                     AES256_BLK_SZ);
    return 0;
  }

  // Align buffer before reading
  _alignBuffer();

  // Read data from stream, add padding and encrypt
  _file.read(_buffer.data(), toRead);
  s_cryptCtx->decrypt(_buffer.data(), toRead, _iv);
  _bufferWritten += toRead;

  if (fileSizeLeft == toRead) {
    std::uint8_t paddingSize = _buffer[_bufferWritten - 1];
    for (std::size_t i = 1; i <= paddingSize; ++i) {
      if (_buffer[_bufferWritten - i] != paddingSize) {
        Logger::println("[CryptoFileReader] Padding is invalid");
        close();
        return 0;
      }
    }
    _bufferWritten -= paddingSize;
    _file.close();
  }

  return fileSizeLeft;
}

bool CryptoFileReader::_ensureReadBuffer(std::size_t length) {
  if (_bufferUsed() >= length) {
    return true;
  }

  if (!_file.isReadable()) {
    return false;
  }

  _readIntoBuffer();
  if (_bufferUsed() >= length) {
    return true;
  }

  return false;
}

CryptoFileWriter::CryptoFileWriter(const char* path)
  : _buffer(), _iv(), _bufferWritten(0), _fileWritten(0), _file(SDCard::Open(path, O_CREAT | O_TRUNC | O_WRITE)) {
  if (!_file.isWritable()) {
    Logger::printlnf("[CryptoFileWriter] File %s is not writable", path);
    return;
  }

  Initialize();

  std::size_t nWritten;

  // Write file ID to file
  CryptoUtils::RandomBytes(_iv);
  nWritten = _file.write(s_cryptCtx->fileID);
  if (nWritten != s_cryptCtx->fileID.size()) {
    Logger::println("[CryptoFileWriter::CryptoFileWriter()] Failed to write file ID to file");
    close();
    return;
  }

  // Write IV to file
  nWritten = _file.write(_iv.data(), _iv.size());
  if (nWritten != _iv.size()) {
    Logger::println("[CryptoFileWriter::CryptoFileWriter()] Failed to write IV to file");
    close();
    return;
  }

  _fileWritten += s_cryptCtx->fileID.size() + _iv.size();
}

CryptoFileWriter::~CryptoFileWriter() {
  close();
}

std::size_t CryptoFileWriter::write(std::uint8_t data) {
  if (!_file.isWritable()) {
    return 0;
  }

  flush(false);

  _buffer[_bufferWritten++] = data;

  return 1;
}

std::size_t CryptoFileWriter::write(const std::uint8_t* data, std::size_t length) {
  if (!_file.isWritable()) {
    return 0;
  }

  const std::uint8_t* dataEnd = data + length;
  while (data < dataEnd) {
    flush(false);

    std::size_t toWrite = std::min(static_cast<std::size_t>(dataEnd - data), _buffer.size() - _bufferWritten);
    std::memcpy(_buffer.data() + _bufferWritten, data, toWrite);

    _bufferWritten += toWrite;
    data += toWrite;
  }

  return length;
}

bool CryptoFileWriter::flush(bool closeFile) {
  bool bufferEmpty = _bufferWritten == 0;
  bool bufferFull  = _bufferWritten == _buffer.size();

  if ((bufferEmpty || !bufferFull) && !closeFile) {
    return true;
  }

  if (!_file.isWritable()) {
    return false;
  }

  if (bufferFull) {
    Logger::println("[CryptoFileWriter::_flush(bool)] Buffer is full, encrypting and writing");
    s_cryptCtx->encrypt(_buffer.data(), _buffer.size(), _iv);
    std::size_t nWritten = _file.write(_buffer.data(), _buffer.size());
    if (nWritten != _buffer.size()) {
      Logger::printlnf("[CryptoFileWriter::_flush(bool)] Failed to write buffer to file: %d != %d", nWritten, _buffer.size());
    }
    _fileWritten += _buffer.size();
    _bufferWritten = 0;
  }

  if (!closeFile) {
    return true;
  }

  // Calculate padding
  std::size_t paddedLength  = (_bufferWritten & ~0x0FULL) + 0x10;
  std::size_t paddingLength = paddedLength - _bufferWritten;

  // Add padding
  std::memset(_buffer.data() + _bufferWritten, paddingLength, paddingLength);

  // Encrypt buffer
  s_cryptCtx->encrypt(_buffer.data(), paddedLength, _iv);

  // Write buffer to file
  std::size_t nWritten = _file.write(_buffer.data(), paddedLength);
  _fileWritten += nWritten;
  if (nWritten != paddedLength) {
    Logger::printlnf("[CryptoFileWriter::_flush(bool)] Failed to write buffer to file: %d != %d", nWritten, _buffer.size());
    _file.close();
    return false;
  }

  // We are done, close the file
  return _file.close();
}
