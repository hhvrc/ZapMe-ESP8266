#pragma once

#include <SdFat.h>
#include <memory>
#include <tuple>

enum class SDCardError {
  None,
  InitializationFailed,
  RootDirectoryNotFound,
};

std::tuple<std::shared_ptr<SdFs>, SDCardError> SDCard();
