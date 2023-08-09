#include "wifi-ap-server.hpp"

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <memory>
#include <vector>

#include "sdcard_webhandler.hpp"
#include "logger.hpp"
#include "constants.hpp"

struct AP_Backend {
  AP_Backend(const std::vector<AsyncWebHandler>& handlers);

  AsyncWebServer _webServer;
  AsyncWebSocket _webSocket;
  std::vector<AsyncWebHandler> _handlers;
};

std::unique_ptr<AP_Backend> Instance = nullptr;
AccessPoint::WebSocketCallbacks Callbacks = {
  [](AsyncWebSocketClient *client) {},
  [](AsyncWebSocketClient *client) {},
  [](AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {},
  [](AsyncWebSocketClient *client) {},
  [](AsyncWebSocketClient *client, uint16_t code, const String &message) {}
};

void AccessPoint::SetWebSocketCallbacks(WebSocketCallbacks callbacks) {
  Callbacks = callbacks;
}

bool AccessPoint::Start(const char* ssid, const char* password, const std::vector<AsyncWebHandler>& handlers) {
  Logger::Log("Starting access point");
  if (Instance != nullptr) {
    return true;
  }

  Logger::Log("Configuring access point");
  if (!WiFi.softAPConfig(IPAddress(10,0,0,2), IPAddress(10,0,0,1), IPAddress(255,255,255,0))) {
    Logger::Log("Failed to configure access point");
    return false;
  }

  Logger::Log("Starting access point");
  if (!WiFi.softAP(ssid, password)) {
    Logger::Log("Failed to start access point");
    return false;
  }

  WiFi.onSoftAPModeStationConnected([](const WiFiEventSoftAPModeStationConnected& event) {
    char buf[64];
    sprintf(buf, "Station connected: %02X:%02X:%02X:%02X:%02X:%02X", event.mac[0], event.mac[1], event.mac[2], event.mac[3], event.mac[4], event.mac[5]);
    Logger::Log(buf);
  });
  WiFi.onSoftAPModeStationDisconnected([](const WiFiEventSoftAPModeStationDisconnected& event) {
    char buf[64];
    sprintf(buf, "Station disconnected: %02X:%02X:%02X:%02X:%02X:%02X", event.mac[0], event.mac[1], event.mac[2], event.mac[3], event.mac[4], event.mac[5]);
    Logger::Log(buf);
  });
  WiFi.onSoftAPModeProbeRequestReceived([](const WiFiEventSoftAPModeProbeRequestReceived& event) {
    char buf[64];
    sprintf(buf, "Probe request received: %02X:%02X:%02X:%02X:%02X:%02X", event.mac[0], event.mac[1], event.mac[2], event.mac[3], event.mac[4], event.mac[5]);
    Logger::Log(buf);
  });

  Instance = std::make_unique<AP_Backend>(handlers);

  Logger::Log("Starting web server");
  Instance->_webServer.begin();

  return true;
}

void AccessPoint::Stop() {
  if (Instance == nullptr) {
    return;
  }

  Logger::Log("Stopping web server");
  Instance->_webServer.end();

  Logger::Log("Stopping access point");
  WiFi.softAPdisconnect(true);

  Instance = nullptr;
}

bool AccessPoint::IsRunning() {
  return Instance != nullptr;
}

void AccessPoint::WebSocketBroadcast(const String& message) {
  if (Instance == nullptr) {
    return;
  }

  Instance->_webSocket.textAll(message);
}

void AccessPoint::RunChores() {
  if (Instance == nullptr) {
    return;
  }

  Instance->_webSocket.cleanupClients();
}

void HandleWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Callbacks.onConnect(client);
      break;
    case WS_EVT_DISCONNECT:
      Callbacks.onDisconnect(client);
      break;
    case WS_EVT_DATA:
      Callbacks.onMessage(client, type, arg, data, len);
      break;
    case WS_EVT_PONG:
      Callbacks.onPong(client);
      break;
    case WS_EVT_ERROR:
      Callbacks.onError(client, *((uint16_t*)arg), String((char*)data));
      break;
  }
}

AP_Backend::AP_Backend(const std::vector<AsyncWebHandler>& handlers) : _webServer(80), _webSocket("/ws"), _handlers(handlers) {
  _webSocket.onEvent(HandleWebSocketEvent);
  _webServer.addHandler(&_webSocket);

  //_handlers.push_back(SDCardWebHandler());

  for (AsyncWebHandler& handler : _handlers) {
    _webServer.addHandler(&handler);
  }
}
