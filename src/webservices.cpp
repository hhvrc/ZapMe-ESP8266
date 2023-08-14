#include "webservices.hpp"

#include "logger.hpp"

WebServices::WebServices(WebServices::WebSocketCallbacks webSocketCallbacks)
  : _webServer(80)
  , _socketServer(81)
  , _sdWebHandler()
  , _webSocketCallbacks(webSocketCallbacks)
{
  _socketServer.onEvent(std::bind(&WebServices::handleWebSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
  _socketServer.begin();

  _webServer.addHandler(&_sdWebHandler);
  _webServer.begin();
}

WebServices::~WebServices() {
  _webServer.close();
  _socketServer.close();
}

void WebServices::update() {
  _webServer.handleClient();
  _socketServer.loop();
}

void WebServices::handleWebSocketEvent(std::uint8_t socketId, WStype_t type, std::uint8_t* data, std::size_t len) {
  switch (type) {
    case WStype_CONNECTED:
      _webSocketCallbacks.onConnect(socketId, _socketServer.remoteIP(socketId));
      break;
    case WStype_DISCONNECTED:
      _webSocketCallbacks.onDisconnect(socketId);
      break;
    case WStype_BIN:
    case WStype_TEXT:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      _webSocketCallbacks.onMessage(socketId, type, data, len);
      break;
    case WStype_PING:
      _webSocketCallbacks.onPing(socketId);
      break;
    case WStype_PONG:
      _webSocketCallbacks.onPong(socketId);
      break;
    case WStype_ERROR:
      _webSocketCallbacks.onError(socketId, len, String(reinterpret_cast<char*>(data)));
      break;
    default:
      Logger::printlnf("Unknown WebSocket event type: %d", type);
      break;
  }
}
