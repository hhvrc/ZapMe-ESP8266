#include "sdcard.hpp"

std::shared_ptr<SdFs> s_sd;

std::tuple<std::shared_ptr<SdFs>, SDCardError> SDCard() {
  if (s_sd) {
    return { s_sd, SDCardError::None };
  }
  s_sd = std::make_shared<SdFs>();

  // Initialize the SD card (Pin D8/GPIO15 is the SD card's CS pin, we want to share the SPI bus with other devices, and we want to use the maximum SPI clock speed)
  if (!s_sd->begin(SdSpiConfig(D8, SHARED_SPI, SD_SCK_MHZ(50)))) {
    return { nullptr, SDCardError::InitializationFailed };
  }

  // Change to the root directory
  if (!s_sd->chdir("/")) {
    return { nullptr, SDCardError::RootDirectoryNotFound };
  }

  return { s_sd, SDCardError::None };
}
