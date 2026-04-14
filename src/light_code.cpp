#include <Arduino.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <esp_now.h>



// put function declarations here:

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char* SSID     = "DormLight";
const char* PASSWORD = "";   // min 8 chars, or "" for open

/********************/
/*SENDING CODE START*/
/********************/
// get the adress of the mac asdress of sending esp
uint8_t senderMac[6] = {0xF4, 0x2D, 0xC9, 0x6B, 0xFC, 0xAC}; 

//define the strcutre of sending message
typedef struct {
  int counter;
  int temperature;
  int brightness;
  boolean OnOff;
} struct_message;

// create a varable of our new type sending message
struct_message inMsg;

//define a function of what to do once a message is recived
void OnDataRecv(const uint8_t * mac, const uint8_t * incomingData, int len) {
  if (len != sizeof(struct_message)) return;
  memcpy(&inMsg, incomingData, sizeof(inMsg));
  Serial.printf("Received counter=%d, temperature=%d, brightness=%d, OnOff=%d\n", inMsg.counter, inMsg.temperature,inMsg.brightness, inMsg.OnOff);
}

/******************/
/*SENDING CODE END*/
/******************/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // Serial.begin(115200); Jack: I was running code at 115200 this could be a issue but I cant check rn.
  


  //Serial.println("Started");
  if (SPIFFS.begin(true)) {
    Serial.println("Files loaded");
  }
  // before we had WiFi.mode(WIFI_AP) my code needs STA. google says we can do both. I cant test rn. 
  WiFi.mode(WIFI_AP_STA);
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

/********************/
/*SENDING CODE START*/
/********************/
//make sure espnow is working
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (1);
  }

//register the recive callback function
  esp_now_register_recv_cb(OnDataRecv);

  //add the sender as a peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, senderMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  Serial.println("--- RECEIVER READY ---");
  Serial.println("Waiting for messages...");

}



  

void loop() {
  // put your main code here, to run repeatedly:
}

