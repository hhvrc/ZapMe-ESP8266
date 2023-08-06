#include "wifi-ap-server.hpp"

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <memory>
#include <vector>

#include "constants.hpp"

struct AP_Backend {
    AP_Backend(const std::vector<AsyncCallbackWebHandler>& handlers);

    AsyncWebServer _webServer;
    AsyncWebSocket _webSocket;
    std::vector<AsyncCallbackWebHandler> _handlers;
};

std::unique_ptr<AP_Backend> Instance = nullptr;
WebSocketCallbacks Callbacks = {
    [](AsyncWebSocketClient *client) {},
    [](AsyncWebSocketClient *client) {},
    [](AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {},
    [](AsyncWebSocketClient *client) {},
    [](AsyncWebSocketClient *client, uint16_t code, const String &message) {}
};

void AccessPoint::SetWebSocketCallbacks(WebSocketCallbacks callbacks) {
    Callbacks = callbacks;
}

bool AccessPoint::Start(const std::vector<AsyncCallbackWebHandler>& handlers) {
    if (Instance != nullptr) {
        return true;
    }

    if (!WiFi.softAPConfig(IPAddress(10,0,0,2), IPAddress(10,0,0,1), IPAddress(255,255,255,0))) {
        return false;
    }

    if (!WiFi.softAP(APP_NAME, DEFAULT_PASSWORD)) {
        return false;
    }

    Instance = std::make_unique<AP_Backend>(handlers);

    Instance->_webServer.begin();

    return true;
}

void AccessPoint::Stop() {
    if (Instance == nullptr) {
        return;
    }

    WiFi.softAPdisconnect(true);

    Instance = nullptr;
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

AP_Backend::AP_Backend(const std::vector<AsyncCallbackWebHandler>& handlers) : _webServer(80), _webSocket("/ws"), _handlers(handlers) {
  _webSocket.onEvent(HandleWebSocketEvent);
  _webServer.addHandler(&_webSocket);

    for (AsyncCallbackWebHandler& handler : _handlers) {
        _webServer.addHandler(&handler);
    }
}