#pragma once

#include <AsyncWebSocket.h>
#include <ESPAsyncWebServer.h>

namespace AccessPoint {
  struct WebSocketCallbacks {
    std::function<void(AsyncWebSocketClient *client)> onConnect;
    std::function<void(AsyncWebSocketClient *client)> onDisconnect;
    std::function<void(AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)> onMessage;
    std::function<void(AsyncWebSocketClient *client)> onPong;
    std::function<void(AsyncWebSocketClient *client, uint16_t code, const String &message)> onError;
  };
  void SetWebSocketCallbacks(WebSocketCallbacks callbacks);

  bool Start(const char* ssid, const char* password, const std::vector<AsyncCallbackWebHandler>& handlers);
  void Stop();

  bool IsRunning();

  void WebSocketBroadcast(const String& message);

  void RunChores();
}
