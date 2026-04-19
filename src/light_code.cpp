#include <Arduino.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <esp_now.h>
#include <ArduinoJson.h>


//PCA9685 GPIO Expander
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// I2C pins for PCA9685
#define I2C_SDA 21
#define I2C_SCL 22

// Create the PCA9685 object at the default hardware address(0x40)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver pwm_board2 = Adafruit_PWMServoDriver(0x41); 
//Adafruit_PWMServoDriver pwm_board2 = Adafruit_PWMServoDriver(0x41); //for the second pca
volatile bool newData = false;
/*every int in the light value will follow the pattern: {cool white brightness panel 1,  
warm white brightness panel 1, cool white brightness panel 2,  warm white brightness panel 2.....}*/
//
//int preset1 [14] = {200, 1500, 200, 1500, 200, 1500, 200, 1500, 200, 1500, 200, 1500, 200, 1500};
float preset1 [14] = {1, 0.9, 1, 0.9, 1, 0.9, 1, 0.9, 1, 0.9, 1, 0.9, 1, 0.9};
float preset2 [14] = {1, 0.7, 1, 0.7, 1, 0.7, 1, 0.7, 1, 0.7, 1, 0.7, 1, 0.7};
float preset3 [14] = {1, 0.2, 1, 0.2, 1, 0.2, 1, 0.2, 1, 0.2, 1, 0.2, 1, 0.2};
float lastPreset[14] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0, 0};


// put function declarations here:

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char* SSID     = "DormLight";
const char* PASSWORD = "";   // min 8 chars, or "" for open

/********************/
/*START SENDING CODE*/
/********************/
// get the adress of the mac asdress of sending esp
uint8_t senderMac[6] = {0xF4, 0x2D, 0xC9, 0x6B, 0xFC, 0xAC}; 

//define the strcutre of sending message
  typedef struct {
    int counter;
    int Button1State;
    int Button2State;
    int Button3State;
    boolean OnOff;
  } struct_message;

// create a varable of our new type sending message
struct_message inMsg;
//SENDING CODE 
//define a function of what to do once a message is recived
void OnDataRecv(const uint8_t * mac, const uint8_t * incomingData, int len) {
  if (len != sizeof(struct_message)) return;
  memcpy(&inMsg, incomingData, sizeof(inMsg));
  Serial.printf("Received counter=%d, Button1State=%d, Button2State=%d, Button3State=%d, OnOff=%d\n", inMsg.counter, inMsg.Button1State, inMsg.Button2State, inMsg.Button3State, inMsg.OnOff);
  
  // Tell the loop we have a new message!
  newData = true; 
}

/******************/
/*END SENDING CODE*/
/******************/

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
  settings_file.close();
}

void save_settings(JsonDocument doc) {
  File settings_file = SPIFFS.open("/settings.json", FILE_WRITE);
  if (serializeJson(doc, settings_file) == 0) {
        Serial.print("write error");
  }
  settings_file.close();
}

void updatePresets() {
  JsonDocument doc = load_settings();
  int len = sizeof(preset1) / sizeof(preset1[0]); // = 14
  for (uint8_t i = 0; i < len; i += 2) {
    preset1[i] = (float)(doc["presets"][0]["panels"][i/2]["bright"])/100;
    Serial.println(preset1[i]);
    preset1[i+1] = (float)(doc["presets"][0]["panels"][i/2]["temp"])/100;
    preset2[i] = (float)(doc["presets"][1]["panels"][i/2]["bright"])/100;
    Serial.println(preset2[i]);
    preset2[i+1] = (float)(doc["presets"][1]["panels"][i/2]["temp"])/100;
    preset3[i] = (float)(doc["presets"][2]["panels"][i/2]["bright"])/100;
    Serial.println(preset3[i]);
    preset3[i+1] = (float)(doc["presets"][2]["panels"][i/2]["temp"])/100;

  }
  Serial.println("updated");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // Serial.begin(115200); Jack: I was running code at 115200 this could be a issue but I cant check rn.

  //Serial.println("Started");
  if (SPIFFS.begin(true)) {
    Serial.println("Files loaded");
  }
  JsonDocument doc = load_settings();
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

  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    (void)len;
    if(type == WS_EVT_DATA) {
      Serial.println("data");
      data[len] = '\0'; 
      Serial.println((char *)data);
      JsonDocument newDoc;
      deserializeJson(newDoc, (char *)data);
      save_settings(newDoc);
      updatePresets();
    } else if (type == WS_EVT_CONNECT) {
      Serial.println("Connected");
      JsonDocument freshDoc = load_settings();
      char output[1024];
      serializeJson(freshDoc, output);
      client->text(output); 
      updatePresets();
    } else if (type == WS_EVT_DISCONNECT) {
      Serial.println("disconnected");
    }
  });

  server.addHandler(&ws);
  server.begin();
  Serial.println("Started website");

/********************/
/*START SENDING CODE*/
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

/******************/
/*END SENDING CODE */
/******************/

/********************/
/*START LIGHT CODE*/
/********************/
  Serial.println("Starting Light Node Initialization...");

 // Start the I2C bus using your specific pins
  Wire.begin(I2C_SDA, I2C_SCL);

  //Initialize the PCA9685
  pwm.begin();
  pwm.setPWMFreq(1000); 
  
  pwm_board2.begin();
  pwm_board2.setPWMFreq(1000); 

  // Ensure ALL lights (up to 32 ports across 2 boards) are OFF on startup
  // for (uint8_t i=0; i<32; i++) {
  //   setLightPWM(i, 0); 
  // }

  Serial.println("Both PCA9685 Boards Initialized Successfully!");
  updatePresets();
}

unsigned long previousMillis = 0;
const long interval = 100;


void loop() {
  if (true) {
    newData = false; 

    if  (inMsg.OnOff) {
      
      if  (!inMsg.Button1State) {
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
    


