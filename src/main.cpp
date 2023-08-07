#include <ESP8266WiFi.h>

#include "config.hpp"
#include "wifi-ap-server.hpp"
#include "serializers/caixianlin-serialize.hpp"

bool ledState = 0;
const int ledPin = LED_BUILTIN;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ZapMe - Device Setup</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
  html {
    font-family: Arial, Helvetica, sans-serif;
    text-align: center;
  }
  h1 {
    font-size: 1.8rem;
    color: white;
  }
  h2{
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
  }
  .topnav {
    overflow: hidden;
    background-color: #143642;
  }
  body {
    margin: 0;
  }
  .content {
    padding: 30px;
    max-width: 600px;
    margin: 0 auto;
  }
  .card {
    background-color: #F8F7F9;;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
  }
  .button {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #0f8b8d;
    border: none;
    border-radius: 5px;
    -webkit-touch-callout: none;
    -webkit-user-select: none;
    -khtml-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;
    user-select: none;
    -webkit-tap-highlight-color: rgba(0,0,0,0);
   }
   /*.button:hover {background-color: #0f8b8d}*/
   .button:active {
     background-color: #0f8b8d;
     box-shadow: 2 2px #CDCDCD;
     transform: translateY(2px);
   }
   .state {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
   }
  </style>
</head>
<body>
  <div class="topnav">
    <h1>ZapMe - Device Setup</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>WiFi Credentials</h2>
      <form action="/wifi" method="post">
        <p>SSID: <input type="text" name="ssid" placeholder="SSID"></p>
        <p>Password: <input type="password" name="password" placeholder="Password"></p>
        <p><input type="submit" value="Submit"></p>
      </form>
    </div>
    <div class="card">
      <h2>Output - GPIO 2</h2>
      <p class="state">state: <span id="state">%STATE%</span></p>
      <p><button id="button" class="button">Toggle</button></p>
    </div>
  </div>
<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    var state;
    if (event.data == "1"){
      state = "OFF";
    }
    else{
      state = "ON";
    }
    document.getElementById('state').innerHTML = state;
  }
  function onLoad(event) {
    initWebSocket();
    initButton();
  }
  function initButton() {
    document.getElementById('button').addEventListener('click', toggle);
  }
  function toggle(){
    websocket.send('toggle');
  }
</script>
</body>
</html>
)rawliteral";

String processor(const String& var){
  Serial.println(var);
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

void handleWebServerGetRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", index_html, processor);
}
void handleWebServerGetWifi(AsyncWebServerRequest *request) {
  auto config = Config::LoadReadOnly();

  String str = "";
  for (std::size_t i = 0; i < 4; i++) {
    const char* ssid = config->wifi.networks[i].ssid;
    if (ssid[0] == '\0') {
      continue;
    }
    str += String(ssid) + "\n";
  }

  request->send(200, "text/plain", str);
}
void handleWebServerPostWifi(AsyncWebServerRequest *request) {
  String ssid;
  String password;
  if (request->hasParam("ssid", true)) {
    ssid = request->getParam("ssid", true)->value();
  }
  if (request->hasParam("password", true)) {
    password = request->getParam("password", true)->value();
  }

  auto config = Config::LoadReadWrite();

  config->wifi.networks[0].flags = 0;
  strcpy(config->wifi.networks[0].ssid, ssid.c_str());
  strcpy(config->wifi.networks[0].password, password.c_str());

  request->send_P(200, "text/html", index_html, processor);
}

void handleWebSocketClientConnected(AsyncWebSocketClient* client) {
  Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
}
void handleWebSocketClientDisconnected(AsyncWebSocketClient* client) {
  Serial.printf("WebSocket client #%u disconnected\n", client->id());
}
void handleWebSocketClientMessage(AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) {
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
      AccessPoint::WebSocketBroadcast(String(ledState));
    }
  }
}
void handleWebSocketClientPong(AsyncWebSocketClient* client) {
  Serial.printf("WebSocket client #%u pong received\n", client->id());
}
void handleWebSocketClientError(AsyncWebSocketClient* client, uint16_t code, const String& message) {
  Serial.printf("WebSocket client #%u error %u: %s\n", client->id(), code, message.c_str());
}

void setup(){
  Serial.begin(115200);

  Config::Migrate();
  auto config = Config::LoadReadOnly();

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  AccessPoint::SetWebSocketCallbacks({
    handleWebSocketClientConnected,
    handleWebSocketClientDisconnected,
    handleWebSocketClientMessage,
    handleWebSocketClientPong,
    handleWebSocketClientError
  });

  AsyncCallbackWebHandler rootHandler = AsyncCallbackWebHandler();
  rootHandler.setUri("/");
  rootHandler.setMethod(HTTP_GET);
  rootHandler.onRequest(handleWebServerGetRoot);

  AsyncCallbackWebHandler wifiGetHandler = AsyncCallbackWebHandler();
  wifiGetHandler.setUri("/wifi");
  wifiGetHandler.setMethod(HTTP_GET);
  wifiGetHandler.onRequest(handleWebServerGetWifi);

  AsyncCallbackWebHandler wifiPostHandler = AsyncCallbackWebHandler();
  wifiPostHandler.setUri("/wifi");
  wifiPostHandler.setMethod(HTTP_POST);
  wifiPostHandler.onRequest(handleWebServerPostWifi);

  AccessPoint::Start(
    config->wifi.apName,
    config->wifi.apPassword,
    {
      rootHandler,
      wifiGetHandler,
      wifiPostHandler
    }
  );
}

void loop() {
  if (AccessPoint::IsRunning()) {
    AccessPoint::RunChores();
  }
}
