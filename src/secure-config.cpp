#include "secure-config.hpp"

#include "string-helpers.hpp"

#include <EEPROM.h>

extern "C" {
#include "spi_flash_geometry.h"
}

std::uint8_t* dataPtr = nullptr;
std::uint8_t* GetDataPtr() {
  if (dataPtr) {
    return dataPtr;
  }

  EEPROM.begin(SPI_FLASH_SEC_SIZE);
  dataPtr = EEPROM.getDataPtr();
  return dataPtr;
}

std::uint32_t SecureConfig::Read(std::uint32_t offset, std::uint8_t* buffer, std::uint32_t size) {
  if (offset + size > SPI_FLASH_SEC_SIZE) {
    return 0;
  }

  std::uint8_t* dataPtr = GetDataPtr();
  memcpy(buffer, dataPtr + offset, size);

  return size;
}

std::uint32_t SecureConfig::Read(std::uint32_t offset, String& buffer) {
  std::uint16_t length;
  if (!SecureConfig::Read(offset, length)) {
    return 0;
  }

  if (offset + sizeof(length) + length > SPI_FLASH_SEC_SIZE) {
    return 0;
  }

  if (buffer.length() != length) {
    buffer = StringHelpers::FilledString('\0', length);
  }

  SecureConfig::Read(offset + sizeof(length), StringHelpers::GetSpan(buffer));

  return sizeof(length) + length;
}

std::uint32_t SecureConfig::Write(std::uint32_t offset, const std::uint8_t* buffer, std::uint32_t size) {
  if (offset + size > SPI_FLASH_SEC_SIZE) {
    return 0;
  }

  std::uint8_t* dataPtr = GetDataPtr();
  memcpy(dataPtr + offset, buffer, size);

  return size;
}

std::uint32_t SecureConfig::Write(std::uint32_t offset, const String& buffer) {
  std::uint16_t length = buffer.length();
  if (offset + sizeof(length) + length > SPI_FLASH_SEC_SIZE) {
    return 0;
  }

  std::uint32_t lengthWritten = SecureConfig::Write(offset, length);
  if (lengthWritten != sizeof(length)) {
    return 0;
  }

  std::uint32_t bufferWritten = SecureConfig::Write(offset + sizeof(length), StringHelpers::GetConstSpan(buffer));
  if (bufferWritten != length) {
    return 0;
  }

  return sizeof(length) + length;
}

bool SecureConfig::Save() {
  return EEPROM.commit();
}

void SecureConfig::Clear() {
  memset(GetDataPtr(), 0, SPI_FLASH_SEC_SIZE);
  SecureConfig::Save();
}
