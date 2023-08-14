#include "sdcard-webhandler.hpp"

#include "logger.hpp"
#include "sdcard.hpp"

const char* GetMime(const char* extension) {
  if (strcmp(extension, ".html") == 0) {
    return "text/html";
  } else if (strcmp(extension, ".css") == 0) {
    return "text/css";
  } else if (strcmp(extension, ".js") == 0) {
    return "application/javascript";
  } else if (strcmp(extension, ".png") == 0) {
    return "image/png";
  } else if (strcmp(extension, ".jpg") == 0) {
    return "image/jpeg";
  } else if (strcmp(extension, ".ico") == 0) {
    return "image/x-icon";
  } else if (strcmp(extension, ".svg") == 0) {
    return "image/svg+xml";
  } else if (strcmp(extension, ".ttf") == 0) {
    return "font/ttf";
  } else if (strcmp(extension, ".woff") == 0) {
    return "font/woff";
  } else if (strcmp(extension, ".woff2") == 0) {
    return "font/woff2";
  } else if (strcmp(extension, ".otf") == 0) {
    return "font/otf";
  } else if (strcmp(extension, ".txt") == 0) {
    return "text/plain";
  } else if (strcmp(extension, ".json") == 0) {
    return "application/json";
  } else if (strcmp(extension, ".pdf") == 0) {
    return "application/pdf";
  }

  Logger::printlnf("Unknown file extension: %s", extension);

  return "application/octet-stream";
}

SDCardWebHandler::SDCardWebHandler() : _sd(SDCard::GetInstance()) {
}

bool SDCardWebHandler::canHandle(HTTPMethod method, const String& uri) {
  return method == HTTP_GET && uri != "/ws" && _sd->ok();
}

bool SDCardWebHandler::handle(WebServerType& server, HTTPMethod requestMethod, const String& requestUri) {
  const char* contentType;

  char cPath[256];
  if (requestUri == "/" || requestUri == "/index") {
    strcpy(cPath, "/www/index.html");
    contentType = "text/html";
  } else {
    // If the last part of the path doesn't end in a file extension, add .html to it
    int lastSlash = requestUri.lastIndexOf('/');
    int lastDot = requestUri.lastIndexOf('.');
    if (lastDot < 0 || (lastDot < lastSlash && lastDot != lastSlash + 1 && (unsigned int)lastDot + 1u != requestUri.length())) {
      strcpy(cPath, "/www");
      strcpy(cPath + 4, requestUri.c_str());
      strcpy(cPath + 4 + requestUri.length(), ".html");
      contentType = "text/html";
    } else {
      strcpy(cPath, "/www");
      strcpy(cPath + 4, requestUri.c_str());
      contentType = GetMime(requestUri.c_str() + lastDot);
    }
  }

  FsFile file = _sd->open(cPath, O_READ);
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return true;
  }

  server.sendHeader("Cache-Control", "max-age=86400");

  server.send(200, contentType, file, file.size());

  return true;
}
