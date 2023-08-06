#pragma once

#include <AsyncWebSocket.h>
#include <ESPAsyncWebServer.h>

struct WebSocketCallbacks {
    std::function<void(AsyncWebSocketClient *client)> onConnect;
    std::function<void(AsyncWebSocketClient *client)> onDisconnect;
    std::function<void(AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)> onMessage;
    std::function<void(AsyncWebSocketClient *client)> onPong;
    std::function<void(AsyncWebSocketClient *client, uint16_t code, const String &message)> onError;
};

namespace AccessPoint {
    void SetWebSocketCallbacks(WebSocketCallbacks callbacks);

    bool Start(const std::vector<AsyncCallbackWebHandler>& handlers);
    void Stop();

    void WebSocketBroadcast(const String& message);

    void RunChores();
}