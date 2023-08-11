#include "webservices.hpp"
#include "wifi-ap.hpp"
#include "config.hpp"
#include "sdcard.hpp"
#include "logger.hpp"
#include "serializers/caixianlin-serialize.hpp"

#include <ESP8266mDNS.h>
bool ledState = false;
std::shared_ptr<WebServices> webServices = nullptr;

String processor(const String& var){
  if(var == "STATE"){
    if (ledState){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
  return String();
}

void handleWebSocketClientConnected(std::uint8_t socketId, IPAddress remoteIP) {
  char buf[64];
  sprintf(buf, "WebSocket client #%u connected from %s", socketId, remoteIP.toString().c_str());
  Logger::Log(buf);
}
void handleWebSocketClientDisconnected(std::uint8_t socketId) {
  char buf[64];
  sprintf(buf, "WebSocket client #%u disconnected", socketId);
  Logger::Log(buf);
}
void handleWebSocketClientMessage(std::uint8_t socketId, WStype_t type, std::uint8_t* data, std::size_t len) {
  if (type == WStype_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) {
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState);
      String stateStr = String(ledState);
      webServices->socketServer().broadcastTXT(stateStr);
    }
  }
}
void handleWebSocketClientPing(std::uint8_t socketId) {
  char buf[64];
  sprintf(buf, "WebSocket client #%u ping received", socketId);
  Logger::Log(buf);
}
void handleWebSocketClientPong(std::uint8_t socketId) {
  char buf[64];
  sprintf(buf, "WebSocket client #%u pong received", socketId);
  Logger::Log(buf);
}
void handleWebSocketClientError(std::uint8_t socketId, uint16_t code, const String& message) {
  char buf[64];
  sprintf(buf, "WebSocket client #%u error %u: %s", socketId, code, message.c_str());
  Logger::Log(buf);
}

// Blinks forever in a error pattern
void blinkHalt(int i) {
  while (true) {
    for (int j = 0; j < i; j++) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(200);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(200);
    }
    delay(2000);
  }
}

void setup(){
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  auto sd = SDCard::GetInstance();
  switch (sd->error())
  {
  case SDCard::SDCardError::None:
    break;
  case SDCard::SDCardError::InitializationFailed:
    blinkHalt(1);
  case SDCard::SDCardError::RootDirectoryNotFound:
    blinkHalt(2);
  default:
    blinkHalt(3);
  }

  auto loggerError = Logger::Initialize();
  switch (loggerError)
  {
  case Logger::InitializationError::None:
    break;
  case Logger::InitializationError::SDCardError:
    blinkHalt(4);
  case Logger::InitializationError::FileSystemError:
    blinkHalt(5);
  default:
    blinkHalt(6);
  }

  Logger::Log("ZapMe starting up");

  Logger::Log("Configuring DNS Multicast");
  if (!MDNS.begin("zapme")) {
    Logger::Log("Failed to configure DNS Multicast");
    // mDNS is not critical, so we can continue
  }

  Logger::Log("Configuring WiFi AP");
  if (!WiFi_AP::Initialize(IPAddress(10,0,0,1), IPAddress(255,255,255,0))) {
    Logger::Log("Failed to configure access point");
    blinkHalt(7);
  }

  Logger::Log("Configuring web services");
  webServices = std::make_shared<WebServices>(WebServices::WebSocketCallbacks {
    .onConnect = handleWebSocketClientConnected,
    .onDisconnect = handleWebSocketClientDisconnected,
    .onMessage = handleWebSocketClientMessage,
    .onPing = handleWebSocketClientPing,
    .onPong = handleWebSocketClientPong,
    .onError = handleWebSocketClientError
  });

/*
  // Overwrite the flash memory with the contents of flash_overwrite.bin if it exists
  FsFile flashOverwriteFile = sd->open("/flash_overwrite.bin", O_READ);
  if (flashOverwriteFile) {
    if (flashOverwriteFile.size() > 4096) {
      Logger::Log("flash_overwrite.bin is too large");
      blinkError(1);
    }

    // If the file exists, overwrite the flash memory with its contents
    Logger::Log("Overwriting flash memory with contents of flash_overwrite.bin");

    EEPROM.begin(flashOverwriteFile.size());
    flashOverwriteFile.readBytes(EEPROM.getDataPtr(), flashOverwriteFile.size());
    EEPROM.end();
    flashOverwriteFile.close();

    Logger::Log("Done");

    // Delete the file
    sd->remove("/flash_overwrite.bin");

    Logger::Log("Deleted flash_overwrite.bin");
  }
*/
  Config::Migrate();
  auto config = Config::LoadReadOnly();

}

void loop() {
  MDNS.update();
  WiFi_AP::Loop();
  webServices->update();
}
