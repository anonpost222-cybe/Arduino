#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* apiKey = "OQA4QZKCQ92DUUJU";
const char* server = "http://api.thingspeak.com";

const char* ssid = "Wokwi-GUEST";
const char* password = "";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int BTN_COPY = 13;
const int BTN_PASTE = 12;
const int BTN_ACT1 = 14;
const int BTN_ACT2 = 27;

enum Mode { MODE_GENERAL = 1, MODE_VSCODE = 2, MODE_PHOTOSHOP = 3 };
Mode currentMode = MODE_GENERAL;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

void updateDisplay();
void handleButtonPress(int btnIndex);
void sendToThingSpeak(int field1_mode, int field2_btn);

void setup() {
  Serial.begin(115200);

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");

  pinMode(BTN_COPY, INPUT_PULLUP);
  pinMode(BTN_PASTE, INPUT_PULLUP);
  pinMode(BTN_ACT1, INPUT_PULLUP);
  pinMode(BTN_ACT2, INPUT_PULLUP);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 failed"));
    for(;;);
  }

  updateDisplay();
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command == "code") currentMode = MODE_VSCODE;
    else if (command == "photo") currentMode = MODE_PHOTOSHOP;
    else if (command == "gen") currentMode = MODE_GENERAL;
    
    updateDisplay();
    sendToThingSpeak(currentMode, 0);
  }

  if (millis() - lastDebounceTime > debounceDelay) {
    if (digitalRead(BTN_COPY) == LOW)  handleButtonPress(1);
    if (digitalRead(BTN_PASTE) == LOW) handleButtonPress(2);
    if (digitalRead(BTN_ACT1) == LOW)  handleButtonPress(3);
    if (digitalRead(BTN_ACT2) == LOW)  handleButtonPress(4);
  }
}

void handleButtonPress(int btnIndex) {
  lastDebounceTime = millis();
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 20);
  display.print("BTN: "); display.print(btnIndex);
  display.display();
  
  Serial.println("Button Pressed: " + String(btnIndex));

  sendToThingSpeak(currentMode, btnIndex);

  delay(500); 
  updateDisplay();
}

void sendToThingSpeak(int field1_mode, int field2_btn) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "/update?api_key=" + String(apiKey) + 
                 "&field1=" + String(field1_mode) + "&field2=" + String(field2_btn);
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      Serial.println("ThingSpeak Update: OK");
    } else {
      Serial.println("ThingSpeak Error");
    }
    http.end();
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print("Mode: ");
  if(currentMode == 1) display.print("General");
  if(currentMode == 2) display.print("VS Code");
  if(currentMode == 3) display.print("Photoshop");
  
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  
  display.setCursor(0, 20);
  display.print("WiFi: Connected");
  
  display.setCursor(0, 35);
  switch (currentMode) {
    case MODE_GENERAL:
      display.println("1:Copy 2:Paste");
      break;
    case MODE_VSCODE:
      display.println("1:Fmt  2:Cmt");
      break;
    case MODE_PHOTOSHOP:
      display.println("1:Brsh 2:Ers");
      break;
  }
  
  display.display();
}