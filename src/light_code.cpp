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

// Create the PCA9685 object at the default hardware address(0x40)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
//Adafruit_PWMServoDriver pwm_board2 = Adafruit_PWMServoDriver(0x41); //for the second pca
int preset1 = 200; // Example brightness value for preset 1
int preset2 = 400; // Example brightness value for preset 2
int preset3 = 800; // Example brightness value for preset 3
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
}

/******************/
/*END SENDING CODE*/
/******************/

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
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

  // Set the PWM Frequency for the PT4115
  pwm.setPWMFreq(1000); 

  // Ensure all lights are OFF on startup
  for (uint8_t i=0; i<16; i++) {
    pwm .setPWM(i, 0, 0); 
  }

  Serial.println("PCA9685 Initialized Successfully!");

}


  

void loop() {
  // put your main code here, to run repeatedly:

  if(inMsg.OnOff && !inMsg.Button1State) {
    pwm.setPWM(0, 0, 200);
    pwm.setPWM(1, 0, 200);

  } 
  if(inMsg.OnOff && !inMsg.Button2State) {
    pwm.setPWM(0, 0, 1500);
    pwm.setPWM(1, 0, 200);

  }
  if(inMsg.OnOff && !inMsg.Button3State) {
    pwm.setPWM(0, 0, 4000);
    pwm.setPWM(1, 0, 4000);
  }
  if(!inMsg.OnOff) {
    pwm.setPWM(0, 0, 0);
    pwm.setPWM(1, 0, 0);
  }


  /*
  this is the code for the serial monitor control of the light. I have it commented out for now because I want to test the espnow.
  // 1. Check if you have typed anything into the Serial Monitor
  if (Serial.available() > 0) {
    
    // 2. Read the incoming text and convert it to an integer
    int newBrightness = Serial.parseInt();

    // 3. Clear any leftover invisible newline characters (\n) from the buffer
    Serial.readStringUntil('\n');

    // 4. Safety Check: Constrain the value to the strict 12-bit range (0 to 4095)
    // This prevents the chip from crashing if you accidentally type "5000"
    newBrightness = constrain(newBrightness, 0, 4095);

    // 5. Instantly apply the new brightness to Channel 0
    pwm.setPWM(0, 0, newBrightness);

    // 6. Print a confirmation back to your screen so you know it worked
    Serial.print("Real-time brightness updated to: ");
    Serial.println(newBrightness);
    
  }
    */
}

    


