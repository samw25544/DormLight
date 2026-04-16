#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
  const int buttonPin1 = 25;
  const int buttonPin2 = 33;
  const int buttonPin3 = 32;
  const int buttonPin4 = 26;

  int TruebuttonState1 = 0;
  int TruebuttonState2 = 0;
  int TruebuttonState3 = 0;
  int TrueONOFFState = 0;

  int lastONOFFState = 1;
  int lastButtonState1 = 1;
  int lastButtonState2 = 1;
  int lastButtonState3 = 1;

  int press_cnt = 0;
  int temperature = 1000; 

  unsigned long lastDebounceTime = 0;  // Holds the timestamp of the last state change
  unsigned long debounceDelay = 50;    // How long to wait for the bouncing to stop (50ms)

  uint8_t receiverMac[6] = {0xF4, 0x2D, 0xC9, 0x6A, 0x6E, 0xBC};

  typedef struct {
    int counter;
    int Button1State;
    int Button2State;
    int Button3State;
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
    pinMode(buttonPin3, INPUT_PULLUP);
    pinMode(buttonPin4, INPUT_PULLUP);
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
    int CurrONOFFState = digitalRead(buttonPin1);
    int CurrButtonState1 = digitalRead(buttonPin2);
    int CurrButtonState2 = digitalRead(buttonPin3);
    int CurrButtonState3 = digitalRead(buttonPin4);

    //Serial.printf("button state 1=%d button state 2=%d\n", CurrONOFFState, CurrButtonState1);
    //want to send button state. 
    msg.counter = press_cnt;
    msg.Button1State = CurrButtonState1;
    msg.Button2State = CurrButtonState2;
    msg.Button3State = CurrButtonState3;
    msg.OnOff = CurrONOFFState;


    //Has the button state changed?
    if((CurrButtonState1 != lastButtonState1) || (CurrONOFFState != lastONOFFState) || (CurrButtonState2 != lastButtonState2) || (CurrButtonState3 != lastButtonState3)) {
      lastDebounceTime = millis();
    }
    //Have we given enough time for the bouncing to stop?
    if((millis() - lastDebounceTime) > debounceDelay){
      //Is the button state different from the last stable state?
      if ((CurrButtonState1 != TruebuttonState1) || (CurrONOFFState != TrueONOFFState) || (CurrButtonState2 != TruebuttonState2) || (CurrButtonState3 != TruebuttonState3)) {
        TruebuttonState1 = CurrButtonState1;
        TrueONOFFState = CurrONOFFState;
        TruebuttonState2 = CurrButtonState2;
        TruebuttonState3 = CurrButtonState3;
          esp_err_t res = esp_now_send(receiverMac, (const uint8_t*)&msg, sizeof(msg));
          Serial.println("button pressed");
          Serial.printf("Sent counter=%d res=%d\n", msg.counter, res);
          press_cnt++;
        
      }
    }
    lastONOFFState = CurrONOFFState;
    lastButtonState1 = CurrButtonState1;
    lastButtonState2 = CurrButtonState2;
    lastButtonState3 = CurrButtonState3;
  }