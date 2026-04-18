#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// put function declarations here:


AsyncWebSocket ws("/ws");
const char* SSID     = "DormLight";
const char* PASSWORD = "";   // min 8 chars, or "" for open

JsonDocument load_settings() {
  File settings_file = SPIFFS.open("/settings.json", "r");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, settings_file);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
  }
  Serial.println((int)doc["current_preset"]);
  return doc;
}

void save_settings(JsonDocument doc) {
  File settings_file = SPIFFS.open("/settings.json", FILE_WRITE);
  if (serializeJson(doc, settings_file) == 0) {
        Serial.print("write error");
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

void setup() {
  if (SPIFFS.begin(true)) {
    Serial.println("Files loaded");
  }

  JsonDocument doc = load_settings();
  //Serial.println((int)doc["presets"][0]["panels"][0]["bright"]);
  //doc["presets"][0]["panels"][0]["bright"] = (int)((int)doc["presets"][0]["panels"][0]["bright"] + (int)1);
  //save_settings(doc);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID, PASSWORD);
  Serial.println(WiFi.softAPIP().toString().c_str());
  
  // // Close the file
  // file.close();
  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  ws.onEvent([doc](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    (void)len;
    if(type == WS_EVT_DATA) {
      //Serial.println("data");
      Serial.println("data");
      data[len] = '\0'; 
      Serial.println((char *)data);
      //Serial.println("something else");
      JsonDocument newDoc;
      deserializeJson(newDoc, (char *)data);
      save_settings(newDoc);
      JsonDocument doc = load_settings();
    } else if (type == WS_EVT_CONNECT) {
      Serial.println("Connected");
      char output[1024];
      serializeJson(doc, output);
      ws.printfAll(output);
    }
  });

  server.addHandler(&ws);
  server.begin();
  Serial.println("Started website");
}

void loop() {
  // put your main code here, to run repeatedly:
}
