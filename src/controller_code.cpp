#include <Arduino.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

// put function declarations here:

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char* SSID     = "DormLight";
const char* PASSWORD = "";   // min 8 chars, or "" for open

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //Serial.println("Started");
  if (SPIFFS.begin(true)) {
    Serial.println("Files loaded");
  }
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID, PASSWORD);
  Serial.println(WiFi.softAPIP().toString().c_str());
  //server.serveStatic("/", SPIFFS, "/SPIFFS/index.html");
  server.serveStatic("/main.css", SPIFFS, "/main.css");
  server.on("/string", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Thingy");
  });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });

  // File file = SPIFFS.open("/index.html");
  // if (!file) {
  //   Serial.println("Failed to open file for reading");
  // } else {
  //   Serial.println("Opened file for reading");
  // }
  // while (file.available()) {
  //   Serial.println(file.read());
  //   Serial.println("file");
  // }
  
  // // Close the file
  // file.close();
  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    (void)len;
    if(type == WS_EVT_DATA) {
      //Serial.println("data");
      data[len] = '\0'; 
      Serial.println((char *)data);
    } else {
      //Serial.println("something else");
    }
  });
  server.addHandler(&ws);
  server.begin();
  Serial.println("Started website");
}

void loop() {
  // put your main code here, to run repeatedly:
}

