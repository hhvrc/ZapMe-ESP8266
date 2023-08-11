#pragma once

#include "sdcard_webhandler.hpp"

#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

struct WebServices {
  struct WebSocketCallbacks {
    std::function<void(std::uint8_t socketId, IPAddress ipAddress)> onConnect;
    std::function<void(std::uint8_t socketId)> onDisconnect;
    std::function<void(std::uint8_t socketId, WStype_t type, std::uint8_t* data, std::size_t len)> onMessage;
    std::function<void(std::uint8_t socketId)> onPing;
    std::function<void(std::uint8_t socketId)> onPong;
    std::function<void(std::uint8_t socketId, std::uint16_t code, const String &message)> onError;
  };

  WebServices(WebSocketCallbacks webSocketCallbacks);
  ~WebServices();

  ESP8266WebServer& webServer() { return _webServer; }
  WebSocketsServer& socketServer() { return _socketServer; }

  void update();
private:
  void handleWebSocketEvent(std::uint8_t socketId, WStype_t type, std::uint8_t* data, std::size_t len);

  ESP8266WebServer _webServer;
  WebSocketsServer _socketServer;
  SDCardWebHandler _sdWebHandler;
  WebSocketCallbacks _webSocketCallbacks;
};
