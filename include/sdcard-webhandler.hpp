#pragma once

#include "sdcard.hpp"

#include <ESP8266WebServer.h>

class SDCardWebHandler : public RequestHandler {
  std::shared_ptr<SDCard> _sd;
  using WebServerType = esp8266webserver::ESP8266WebServerTemplate<WiFiServer>;

public:
  SDCardWebHandler();

  bool canHandle(HTTPMethod method, const String& uri) override;
  bool handle(WebServerType& server, HTTPMethod requestMethod, const String& requestUri) override;

  SDCardWebHandler(SDCardWebHandler const&) = delete;
  void operator=(SDCardWebHandler const&)   = delete;
};
