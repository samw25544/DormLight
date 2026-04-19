#include <Arduino.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <esp_now.h>

//PCA9685 GPIO Expander
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// I2C pins for PCA9685
#define I2C_SDA 21
#define I2C_SCL 22

// Create the PCA9685 objects at their hardware addresses
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver pwm_board2 = Adafruit_PWMServoDriver(0x41); 

volatile bool newData = false;
/*every int in the light value will follow the pattern: {cool white brightness panel 1,  
warm white brightness panel 1, cool white brightness panel 2,  warm white brightness panel 2.....}*/
//
//int preset1 [14] = {200, 1500, 200, 1500, 200, 1500, 200, 1500, 200, 1500, 200, 1500, 200, 1500};
float preset1 [14] = {1, 0.9, 1, 0.9, 1, 0.9, 1, 0.9, 1, 0.9, 1, 0.9, 1, 0.9};
float preset2 [14] = {1, 0.7, 1, 0.7, 1, 0.7, 1, 0.7, 1, 0.7, 1, 0.7, 1, 0.7};
float preset3 [14] = {1, 0.2, 1, 0.2, 1, 0.2, 1, 0.2, 1, 0.2, 1, 0.2, 1, 0.2};
float lastPreset[14] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0, 0};

// Web server and WebSocket setup
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char* SSID     = "DormLight";
const char* PASSWORD = "";   // min 8 chars, or "" for open

/********************/
/*START SENDING CODE*/
/********************/
uint8_t senderMac[6] = {0xF4, 0x2D, 0xC9, 0x6B, 0xFC, 0xAC}; 

  typedef struct {
    int counter;
    int Button1State;
    int Button2State;
    int Button3State;
    boolean OnOff;
  } struct_message;

struct_message inMsg;

void OnDataRecv(const uint8_t * mac, const uint8_t * incomingData, int len) {
  if (len != sizeof(struct_message)) return;
  memcpy(&inMsg, incomingData, sizeof(inMsg));
  Serial.printf("Received counter=%d, Button1State=%d, Button2State=%d, Button3State=%d, OnOff=%d\n", inMsg.counter, inMsg.Button1State, inMsg.Button2State, inMsg.Button3State, inMsg.OnOff);
  
  newData = true; 
}
/******************/
/*END SENDING CODE*/
/******************/

/********************/
/* SMART ROUTER     */
/********************/
// This function automatically routes the signal to the correct board
void setLightPWM(uint8_t index, int brightness) {

    // Indices 0-15 go to the first board
    
}

void setup() {
  Serial.begin(115200);

  if (SPIFFS.begin(true)) {
    Serial.println("Files loaded");
  }
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(SSID, PASSWORD);
  Serial.println(WiFi.softAPIP().toString().c_str());
  
  server.serveStatic("/main.css", SPIFFS, "/main.css");
  server.on("/string", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Thingy");
  });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });

  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    (void)len;
    if(type == WS_EVT_DATA) {
      data[len] = '\0'; 
      Serial.println((char *)data);
    }
  });
  server.addHandler(&ws);
  server.begin();
  Serial.println("Started website");

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (1);
  }

  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, senderMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  Serial.println("--- RECEIVER READY ---");
  Serial.println("Waiting for messages...");

/********************/
/*START LIGHT CODE*/
/********************/
  Serial.println("Starting Light Node Initialization...");

  Wire.begin(I2C_SDA, I2C_SCL);

  //Initialize BOTH PCA9685 boards
  pwm.begin();
  pwm.setPWMFreq(1000); 
  
  pwm_board2.begin();
  pwm_board2.setPWMFreq(1000); 

  // Ensure ALL lights (up to 32 ports across 2 boards) are OFF on startup
  for (uint8_t i=0; i<32; i++) {
    setLightPWM(i, 0); 
  }

  Serial.println("Both PCA9685 Boards Initialized Successfully!");
}


void loop() {
  if (newData) {
    newData = false; 

    if (inMsg.OnOff) {
      
      if (!inMsg.Button1State) {
        for (uint8_t i=0; i<(sizeof(preset1) / sizeof(preset1[0])); i+=2) {
          float b = preset1[i]; // Get the base brightness for this channel
          float t = preset1[i+1]; // Get the temp factor for this channel
          int coolPWM, warmPWM;
            if(t <= 0.5) {
              coolPWM = (int)(b * 4095.0); 
              warmPWM = (int)(t * 2.0 * b * 4095.0);
            }
            else {
              // Right half of the slider: Warm is pegged at Max, Cool ramps down from Max to 0
              coolPWM = (int)((1.0 - t) * 2.0 * b * 4095.0); 
              warmPWM = (int)(b * 4095.0);                   
              }
          

          lastPreset[i] = coolPWM; 
          lastPreset[i+1] = warmPWM;
        }
      } 
      else if (!inMsg.Button2State) {
         for (uint8_t i=0; i<(sizeof(preset2) / sizeof(preset2[0])); i+=2) {
          float b = preset1[i]; // Get the base brightness for this channel
          float t = preset1[i+1]; // Get the temp factor for this channel
          int coolPWM, warmPWM;
            if(t <= 0.5) {
              coolPWM = (int)(b * 4095.0); 
              warmPWM = (int)(t * 2.0 * b * 4095.0);
            }
            else {
              // Right half of the slider: Warm is pegged at Max, Cool ramps down from Max to 0
              coolPWM = (int)((1.0 - t) * 2.0 * b * 4095.0); 
              warmPWM = (int)(b * 4095.0);                   
              }
          

          lastPreset[i] = coolPWM; 
          lastPreset[i+1] = warmPWM;
        }
      } 
      else if (!inMsg.Button3State) {
          // FIXED: Now dynamically calculates array size instead of hardcoding '3'
          for (uint8_t i=0; i<(sizeof(preset3) / sizeof(preset3[0])); i+=2) {
            float b = preset3[i]; // Get the base brightness for this channel
            float t = preset3[i+1]; // Get the temp factor for this channel
            int coolPWM, warmPWM;
              if(t <= 0.5) {
                coolPWM = (int)(b * 4095.0); 
                warmPWM = (int)(t * 2.0 * b * 4095.0);
              }
              else {
                // Right half of the slider: Warm is pegged at Max, Cool ramps down from Max to 0
                coolPWM = (int)((1.0 - t) * 2.0 * b * 4095.0); 
                warmPWM = (int)(b * 4095.0);                   
                }
            

            lastPreset[i] = coolPWM; 
            lastPreset[i+1] = warmPWM;
          }
      }

      // Apply the chosen (or remembered) preset using the Smart Router
        for (uint8_t i=0; i<(sizeof(lastPreset) / sizeof(lastPreset[0])); i++) {
           pwm.setPWM(i, 0, lastPreset[i]);
        } 
      }
    else if(!inMsg.OnOff) {
      // System is Off - Apply 0 using the Smart Router
        for (uint8_t i=0; i<(sizeof(lastPreset) / sizeof(lastPreset[0])); i++) {
           pwm.setPWM(i, 0, 0);
      }
    }
  }
}
