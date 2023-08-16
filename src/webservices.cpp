#include "webservices.hpp"

#include "sdcard-webhandler.hpp"
#include "logger.hpp"

#include <memory>

#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

constexpr std::uint16_t HTTP_PORT = 80;
constexpr std::uint16_t WEBSOCKET_PORT = 81;

struct WebServicesInstance {
  WebServicesInstance() : webServer(HTTP_PORT), socketServer(WEBSOCKET_PORT), sdWebHandler() {}

  ESP8266WebServer webServer;
  WebSocketsServer socketServer;
  SDCardWebHandler sdWebHandler;
};
std::unique_ptr<WebServicesInstance> s_webServices = nullptr;

void handleWebSocketEvent(std::uint8_t socketId, WStype_t type, std::uint8_t* data, std::size_t len);

void WebServices::Start() {
  if (s_webServices != nullptr) {
    Logger::println("[WebServices] Already started");
    return;
  }

  Logger::println("[WebServices] Starting");

  s_webServices = std::make_unique<WebServicesInstance>();

  s_webServices->socketServer.onEvent(handleWebSocketEvent);
  s_webServices->socketServer.begin();

  s_webServices->webServer.addHandler(&s_webServices->sdWebHandler);
  s_webServices->webServer.begin();
}
void WebServices::Stop() {
  if (s_webServices == nullptr) {
    Logger::println("[WebServices] Already stopped");
    return;
  }

  Logger::println("[WebServices] Stopping");

  s_webServices->webServer.close();
  s_webServices->socketServer.close();

  s_webServices = nullptr;
}
bool WebServices::IsRunning() {
  return s_webServices != nullptr;
}
void WebServices::Update() {
  if (s_webServices == nullptr) {
    return;
  }

  s_webServices->webServer.handleClient();
  s_webServices->socketServer.loop();
}

void handleWebSocketClientConnected(std::uint8_t socketId) {
  Logger::printlnf("WebSocket client #%u connected from %s", socketId, s_webServices->socketServer.remoteIP(socketId).toString().c_str());
}
void handleWebSocketClientDisconnected(std::uint8_t socketId) {
  Logger::printlnf("WebSocket client #%u disconnected", socketId);
}
void handleWebSocketClientMessage(std::uint8_t socketId, WStype_t type, std::uint8_t* data, std::size_t len) {
  (void)socketId;
  
  if (type == WStype_t::WStype_TEXT) {
    Logger::printlnf("WebSocket client #%u sent text message", socketId);
  } else if (type == WStype_t::WStype_BIN) {
    Logger::printlnf("WebSocket client #%u sent binary message", socketId);
  } else {
    Logger::printlnf("WebSocket client #%u sent unknown message type %u", socketId, type);
  }

  if (type != WStype_t::WStype_TEXT) {
    return;
  }

  StaticJsonDocument<256> doc;
  auto err = deserializeJson(doc, data, len);
  if (err) {
    Logger::printlnf("Failed to deserialize message: %s", err.c_str());
    return;
  }

  String str;
  serializeJsonPretty(doc, str);
  Logger::printlnf("Message: %s", str.c_str());
}
void handleWebSocketClientPing(std::uint8_t socketId) {
  Logger::printlnf("WebSocket client #%u ping received", socketId);
}
void handleWebSocketClientPong(std::uint8_t socketId) {
  Logger::printlnf("WebSocket client #%u pong received", socketId);
}
void handleWebSocketClientError(std::uint8_t socketId, uint16_t code, const char* message) {
  Logger::printlnf("WebSocket client #%u error %u: %s", socketId, code, message);
}

void handleWebSocketEvent(std::uint8_t socketId, WStype_t type, std::uint8_t* data, std::size_t len) {
  Logger::printlnf("WebSocket event: %u", type);
  switch (type) {
    case WStype_CONNECTED:
      handleWebSocketClientConnected(socketId);
      break;
    case WStype_DISCONNECTED:
      handleWebSocketClientDisconnected(socketId);
      break;
    case WStype_BIN:
    case WStype_TEXT:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      handleWebSocketClientMessage(socketId, type, data, len);
      break;
    case WStype_PING:
      handleWebSocketClientPing(socketId);
      break;
    case WStype_PONG:
      handleWebSocketClientPong(socketId);
      break;
    case WStype_ERROR:
      handleWebSocketClientError(socketId, len, reinterpret_cast<char*>(data));
      break;
    default:
      Logger::printlnf("Unknown WebSocket event type: %d", type);
      break;
  }
}
