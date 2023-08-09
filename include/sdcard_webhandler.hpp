#pragma once

#include "sdcard.hpp"

#include <ESPAsyncWebServer.h>

class SDCardWebHandler : public AsyncWebHandler {
  std::shared_ptr<SDCard> _sd;
public:
  SDCardWebHandler();
  ~SDCardWebHandler() override;

  bool canHandle(AsyncWebServerRequest *request) override;
  void handleRequest(AsyncWebServerRequest *request) override;

  SDCardWebHandler(SDCardWebHandler const&) = delete;
  void operator=(SDCardWebHandler const&) = delete;
};
