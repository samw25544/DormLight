#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
  const int buttonPin1 = 25;
  const int buttonPin2 = 33;

  int TruebuttonState = 0;
  int lastButtonState = 1;

  int press_cnt = 0;
  int temperature = 1000; 

  unsigned long lastDebounceTime = 0;  // Holds the timestamp of the last state change
  unsigned long debounceDelay = 50;    // How long to wait for the bouncing to stop (50ms)

  uint8_t receiverMac[6] = {0xF4, 0x2D, 0xC9, 0x6A, 0x6E, 0xBC};

  typedef struct {
    int counter;
    int temperature;
    int brightness;
    boolean OnOff;
  } struct_message;

  struct_message msg;

  void OnDataSent(const uint8_t * mac_addr, esp_now_send_status_t status) {
    Serial.print("Send status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
  }

  void setup() {
    Serial.begin(115200);
    pinMode(buttonPin1, INPUT_PULLUP);
    pinMode(buttonPin2, INPUT_PULLUP);
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
      Serial.println("ESP-NOW init failed");
      while (1);
    }

    esp_now_register_send_cb(OnDataSent);

    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, receiverMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Peer add failed");
      while (1);
    }

    Serial.println("--- SENDER READY ---");
  }

  void loop() {
    int CurrButtonState1 = digitalRead(buttonPin1);
    int CurrButtonState2 = digitalRead(buttonPin2);

    Serial.printf("button state 1=%d button state 2=%d\n", CurrButtonState1, CurrButtonState2);
    //want to send button state. 
    msg.counter = press_cnt;
    msg.temperature = temperature;
    msg.brightness = 85;
    msg.OnOff = 1;


    //Has the button state changed?
    if(CurrButtonState1 != lastButtonState){
      lastDebounceTime = millis();
    }
    //Have we given enough time for the bouncing to stop?
    if((millis() - lastDebounceTime) > debounceDelay){
      //Is the button state different from the last stable state?
      if (CurrButtonState1 != TruebuttonState) {
        TruebuttonState = CurrButtonState1;
        if (TruebuttonState == 0) {
          esp_err_t res = esp_now_send(receiverMac, (const uint8_t*)&msg, sizeof(msg));
          Serial.println("button pressed");
          Serial.printf("Sent counter=%d res=%d\n", msg.counter, res);
          press_cnt++;
        }
      }
    }
    lastButtonState = CurrButtonState1;
  }