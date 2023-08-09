#include "sdcard_webhandler.hpp"

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
  }

  char buffer[256];
  sprintf(buffer, "Unknown file extension: %s", extension);
  Logger::Log(buffer);

  return "application/octet-stream";
}

SDCardWebHandler::SDCardWebHandler() : _sd(SDCard::GetInstance()) {
}

SDCardWebHandler::~SDCardWebHandler() {
}

bool SDCardWebHandler::canHandle(AsyncWebServerRequest *request) {
  return request->method() == HTTP_GET && request->url() != "/ws" && _sd->ok();
}

void SDCardWebHandler::handleRequest(AsyncWebServerRequest *request) {
  String path = request->url();
  FsFile file;
  String contentType;

  if (path == "/" || path == "/index") {
    file = _sd->open("/www/index.html");
    contentType = "text/html";
  } else {
    // If the last part of the path doesn't end in a file extension, add .html to it
    int lastSlash = path.lastIndexOf('/');
    int lastDot = path.lastIndexOf('.');
    if (lastDot < 0 || (lastDot < lastSlash && lastDot != lastSlash + 1 && (unsigned int)lastDot + 1u != path.length())) {
      path = "/www" + path + ".html";
      contentType = "text/html";
    } else {
      path = "/www" + path;
      contentType = GetMime(path.c_str() + lastDot);
    }
    file = _sd->open(path.c_str());
  }

  Logger::Log((request->url() + " -> " + path + "(" + contentType + ")").c_str());

  if (!file) {
    request->send(404, "text/plain", "File not found");
    return;
  }

  request->send(file, contentType, file.size());
}
