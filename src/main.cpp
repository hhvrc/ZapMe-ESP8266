#include "webservices.hpp"
#include "wifi-ap.hpp"
#include "ntp-client.hpp"
#include "sdcard.hpp"
#include "logger.hpp"
#include "crypto-utils.hpp"
#include "crypto-io.hpp"
#include "serializers/caixianlin-serialize.hpp"

#include <ESP8266mDNS.h>
#include <ESP8266WiFiMulti.h>

NtpClient ntpClient;
std::shared_ptr<WebServices> webServices = nullptr;

void handleWebSocketClientConnected(std::uint8_t socketId, IPAddress remoteIP) {
  Logger::printlnf("WebSocket client #%u connected from %s", socketId, remoteIP.toString().c_str());
}
void handleWebSocketClientDisconnected(std::uint8_t socketId) {
  Logger::printlnf("WebSocket client #%u disconnected", socketId);
}
void handleWebSocketClientMessage(std::uint8_t socketId, WStype_t type, std::uint8_t* data, std::size_t len) {
  (void)socketId;
  (void)type;
  (void)data;
  (void)len;
}
void handleWebSocketClientPing(std::uint8_t socketId) {
  Logger::printlnf("WebSocket client #%u ping received", socketId);
}
void handleWebSocketClientPong(std::uint8_t socketId) {
  Logger::printlnf("WebSocket client #%u pong received", socketId);
}
void handleWebSocketClientError(std::uint8_t socketId, uint16_t code, const String& message) {
  Logger::printlnf("WebSocket client #%u error %u: %s", socketId, code, message.c_str());
}

// Blinks forever in a error pattern
[[noreturn]] void blinkHalt(int i) {
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

void InitializeLED() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}
void InitializeSDCard() {
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
}
void InitializeLogger() {
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
}
void InitializeWiFi() {
  Logger::println("Configuring WiFi");
  WiFi.persistent(false);
  if (!WiFi.setAutoConnect(false)) {
    Logger::println("Failed to configure WiFi auto-connect");
  }
  if (!WiFi.setAutoReconnect(false)) {
    Logger::println("Failed to configure WiFi auto-reconnect");
  }
  if (!WiFi.hostname("zapme")) {
    Logger::println("Failed to set WiFi hostname");
  }
}
void InitializeMDNS() {
  Logger::println("Starting mDNS");
  if (!MDNS.begin("zapme")) {
    Logger::println("Failed to configure DNS Multicast");
    // mDNS is not critical, so we can continue
  }
}
void InitializeWiFiAP() {
  Logger::println("Configuring WiFi AP");
  if (!WiFi_AP::Initialize(IPAddress(10,0,0,1), IPAddress(255,255,255,0))) {
    Logger::println("Failed to configure access point");
    blinkHalt(7);
  }
}
void InitializeWebServices() {
  if (webServices != nullptr) {
    return;
  }

  Logger::println("Initializing web services");
  webServices = std::make_shared<WebServices>(WebServices::WebSocketCallbacks {
    .onConnect = handleWebSocketClientConnected,
    .onDisconnect = handleWebSocketClientDisconnected,
    .onMessage = handleWebSocketClientMessage,
    .onPing = handleWebSocketClientPing,
    .onPong = handleWebSocketClientPong,
    .onError = handleWebSocketClientError
  });
}
void InitializeNTP() {
  Logger::println("Initializing NTP client");
  ntpClient = NtpClient();
}

void setup(){
  InitializeLED();
  InitializeSDCard();
  InitializeLogger();
  Logger::println("ZapMe starting up");
  InitializeWiFi();
  InitializeMDNS();
  InitializeWiFiAP();
  InitializeWebServices();
  InitializeNTP();
}

void handleScanResult(std::int8_t networksFound) {
  if (networksFound < 0) {
    return;
  }

  if (networksFound == 0) {
    Logger::println("[WiFi] Scan complete, no networks found");
  } else {
    Logger::printlnf("[WiFi] Scan complete, found %d networks:", networksFound);

    for (std::int8_t i = 0; i < networksFound; i++) {
      Logger::printlnf("[WiFi]     %s", WiFi.SSID(i).c_str());
    }
  }

  if (networksFound == 0) {
    // TODO: enableAP();
    return;
  }

  // Connect to the first network from config that we find in the scan results
  /*
  std::uint32_t offset = 0;
  std::uint8_t configNetworkCount;
  offset += SecureConfig::Read(0, configNetworkCount);
  for (std::uint8_t i = 0; i < configNetworkCount; i++) {
    String configNetworkSSID;
    offset += SecureConfig::Read(offset, configNetworkSSID);

    String configNetworkPassword;
    offset += SecureConfig::Read(offset, configNetworkPassword);

    for (std::int8_t j = 0; j < networksFound; j++) {
      if (WiFi.SSID(j) == configNetworkSSID) {
        Logger::println("[WiFi] Connecting to network from config");
        WiFi.begin(configNetworkSSID.c_str(), nullptr, i);
        return;
      }
    }
  }
  */
}
bool startWiFiScan() {
  switch (WiFi.status())
  {
  case WL_CONNECTED:
    Logger::println("[WiFi] Disconnecting from WiFi");
    WiFi.disconnect(false, true);
    break;
  case WL_DISCONNECTED:
    Logger::println("[WiFi] Starting WiFi");
    WiFi.begin();
    break;
  case WL_IDLE_STATUS:
    break;
  default:
    {
      Logger::printlnf("[WiFi] Cannot start scan, WiFi is busy (status %u)", WiFi.status());
    }
    return false;
  }

  WiFi.scanDelete();

  std::int8_t scanResult = WiFi.scanNetworks(true);
  if (scanResult == WIFI_SCAN_RUNNING) {
    Logger::println("[WiFi] Scanning for networks");
    return true;
  }

  if (scanResult == WIFI_SCAN_FAILED) {
    Logger::println("[WiFi] Failed to start scan");

    // TODO: Handle error, update web portal

    return false;
  }

  handleScanResult(scanResult);

  return true;
}
void processWiFiScanning() {
  std::int8_t scanResult = WiFi.scanComplete();
  if (scanResult < 0) {
    return;
  }

  handleScanResult(scanResult);
}

bool highPerformanceMode = false;
void SetHighPerformanceMode(bool enabled) {
  if (enabled == highPerformanceMode) {
    return;
  }

  if (enabled) {
    Logger::println("Entering high performance mode");
    highPerformanceMode = true;
    webServices = nullptr;
    WiFi_AP::Stop();
    MDNS.close();
  } else {
    Logger::println("Exiting high performance mode");
    InitializeMDNS();
    //WiFi_AP::Start();
    InitializeWebServices();
    highPerformanceMode = false;
  }
}

void loop() {
  // Run update functions
  // TODO: Add a way to toggle high performance mode

  // During high performance mode, we only run update functions that handle critical tasks
  // This is to ensure that the device is as responsive as possible
  // If any errors are encountered, we will exit high performance mode and run all update functions
  // AccessPoint will be disabled during high performance mode
  if (!highPerformanceMode) {
    if (MDNS.isRunning()) {
      MDNS.update();
    }
    WiFi_AP::Update();
    if (webServices != nullptr) {
      webServices->update();
    }
  }
}
