#include "crypto-io.hpp"

#include "crypto-utils.hpp"
#include "logger.hpp"
#include "sdcard.hpp"

#include <array>
#include <CRC32.h>
#include <EEPROM.h>
#include <memory>

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
  : _buffer(), _iv(), _bufferRead(0), _bufferWritten(0), _file(SDCard::GetInstance()->open(path, O_READ)) {
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
  _file.readBytes(fileID.data(), fileID.size());
  if (!s_cryptCtx->verifyFileID(fileID)) {
    Logger::printlnf(
      "[CryptoFileReader] File \"%s\" has different file ID, encryption keys are different and decryption will fail. Aborting.",
      path);
    close();
    return;
  }

  // Read IV
  _file.readBytes(_iv.data(), _iv.size());

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
    Logger::printlnf("[CryptoFileReader] Moving %d bytes by %d bytes", _bufferUsed(), _bufferRead);
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
  std::size_t curPos       = _file.curPosition();
  std::size_t fileSizeLeft = fileSize - curPos;

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
  _file.readBytes(_buffer.data(), toRead);
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
    : _buffer(), _iv(), _bufferWritten(0), _fileWritten(0), _file(SDCard::GetInstance()->open(path, O_RDWR | O_CREAT)) {
  if (!_file.isWritable()) {
    Logger::printlnf("[CryptoFileWriter] File %s is not writable", path);
    return;
  }

  Initialize();

  // Write file ID and IV to file
  CryptoUtils::RandomBytes(_iv);
  _file.write(s_cryptCtx->fileID.data(), s_cryptCtx->fileID.size());
  _file.write(_iv.data(), _iv.size());
  _fileWritten += s_cryptCtx->fileID.size() + _iv.size();
}

CryptoFileWriter::~CryptoFileWriter() {
  close();
}

std::size_t CryptoFileWriter::write(std::uint8_t data) {
  if (!_file.isWritable()) {
    return 0;
  }

  _flush(false);

  _buffer[_bufferWritten++] = data;

  return 1;
}

std::size_t CryptoFileWriter::write(const std::uint8_t* data, std::size_t length) {
  if (!_file.isWritable()) {
    return 0;
  }

  const std::uint8_t* dataEnd = data + length;
  while (data < dataEnd) {
    _flush(false);

    std::size_t toWrite = std::min(static_cast<std::size_t>(dataEnd - data), _buffer.size() - _bufferWritten);
    std::memcpy(_buffer.data() + _bufferWritten, data, toWrite);

    _bufferWritten += toWrite;
    data += toWrite;
  }

  return length;
}

void CryptoFileWriter::_flush(bool final) {
  if (_bufferWritten == 0 || !_file.isWritable()) {
    return;
  }

  // Buffer is full, encrypt and write
  if (_bufferWritten == _buffer.size()) {
    s_cryptCtx->encrypt(_buffer.data(), _buffer.size(), _iv);
    _file.write(_buffer.data(), _buffer.size());
    _fileWritten += _buffer.size();
    _bufferWritten = 0;
  }

  if (!final) {
    return;
  }

  // Calculate padding
  std::size_t paddedLength  = (_bufferWritten & ~0x0FULL) + 0x10;
  std::size_t paddingLength = paddedLength - _bufferWritten;

  // Add padding
  std::memset(_buffer.data() + _bufferWritten, paddingLength, paddingLength);

  // Encrypt buffer
  s_cryptCtx->encrypt(_buffer.data(), paddedLength, _iv);
  _file.write(_buffer.data(), paddedLength);
  _fileWritten += paddedLength;

  // Truncate file to written length
  _file.sync();
  if (_file.size() > _fileWritten && !_file.truncate(_fileWritten)) {
    Logger::printlnf("[CryptoFileWriter] Failed to truncate file to %d bytes", _fileWritten);
  }

  // We are done, close the file
  _file.close();
}
