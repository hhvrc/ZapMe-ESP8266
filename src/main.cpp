#include "crypto-io.hpp"
#include "crypto-utils.hpp"
#include "logger.hpp"
#include "ntp-client.hpp"
#include "sdcard.hpp"
#include "serializers/caixianlin-serialize.hpp"
#include "webservices.hpp"
#include "wifi-ap.hpp"

#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFiMulti.h>

NtpClient ntpClient;
std::shared_ptr<WebServices> webServices = nullptr;

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
  switch (sd->error()) {
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
  switch (loggerError) {
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
  WiFi.disconnect(true);
  if (!WiFi.mode(WIFI_OFF)) {
    Logger::println("Failed to disable WiFi");
  }

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

  WiFi.mode(WIFI_STA);
}
void InitializeMDNS() {
  Logger::println("Starting mDNS");
  if (!MDNS.begin("zapme")) {
    Logger::println("Failed to configure DNS Multicast");
    // mDNS is not critical, so we can continue
  }
}
void InitializeNTP() {
  Logger::println("Initializing NTP client");
  ntpClient = NtpClient();
}

void enableAP();

void setup() {
  InitializeLED();
  InitializeSDCard();
  InitializeLogger();
  Logger::println("ZapMe starting up");
  InitializeWiFi();
  InitializeMDNS();
  InitializeNTP();
  Logger::println("ZapMe startup complete");

  enableAP();
}

void enableAP() {
  Logger::println("Enabling access point");
  StaticJsonDocument<256> doc;

  auto readFile = CryptoFileReader("/config/wifi-ap.conf.bin");
  if (!readFile) {
    Logger::println("Failed to open config file for reading, creating default config");
    auto writeFile = CryptoFileWriter("/config/wifi-ap.conf.bin");
    if (!writeFile) {
      Logger::println("Failed to open config file for writing");
      return;
    }

    doc["ssid"] = "ZapMe";
    doc["psk"]  = "ZapMe12345";

    if (serializeMsgPack(doc, writeFile) == 0) {
      Logger::println("Failed to serialize config file");
      return;
    }

    doc.clear();
    writeFile.close();

    readFile = CryptoFileReader("/config/wifi-ap.conf.bin");
  }

  auto err = deserializeMsgPack(doc, readFile);
  if (err) {
    Logger::printlnf("Failed to deserialize config file: %s", err.c_str());
    return;
  }

  const char* ssid = doc["ssid"];
  const char* psk  = doc["psk"];

  if (ssid == nullptr || psk == nullptr) {
    Logger::println("Config file is missing ssid and/or psk");
    return;
  }

  Logger::printlnf("Starting access point with SSID %s", ssid);

  if (!WiFi_AP::Start(ssid, psk)) {
    Logger::println("Failed to start access point");
    return;
  }

  Logger::println("Access point started, starting web services");

  WebServices::Start();

  Logger::println("Web services started");
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
  switch (WiFi.status()) {
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
    webServices         = nullptr;
    WiFi_AP::Stop();
    MDNS.close();
  } else {
    Logger::println("Exiting high performance mode");
    InitializeMDNS();
    enableAP();
    WebServices::Start();
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
    WebServices::Update();
  }
}
