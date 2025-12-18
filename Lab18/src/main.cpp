#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>

const char* apiKey = "5LP7Y9QQ5XZGX1QY";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

Adafruit_SSD1306 display(128, 64, &Wire, -1);

const byte ROWS = 4; const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'}, {'4','5','6','B'}, {'7','8','9','C'}, {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27}; 
byte colPins[COLS] = {26, 25, 33, 32};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

#define LED_G 2
#define LED_R 4

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  pinMode(LED_G, OUTPUT); pinMode(LED_R, OUTPUT);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);
  display.println("Swipe Badge..."); display.display();
}

void loop() {
  char key = keypad.getKey();
  if (key) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.printf("ID Scanned: %c\n", key);
    
    bool access = false;
    bool anomaly = false;
    
    if (key == '1' || key == '5' || key == '9') {
      access = true;
      digitalWrite(LED_G, HIGH); delay(1000); digitalWrite(LED_G, LOW);
      display.println("ACCESS GRANTED");
    } else {
      access = false;
      anomaly = true; 
      digitalWrite(LED_R, HIGH); delay(1000); digitalWrite(LED_R, LOW);
      display.println("ACCESS DENIED");
      display.println("Anomaly Logged!");
    }
    display.display();

    if(WiFi.status() == WL_CONNECTED){
       HTTPClient http;
       String url = "http://api.thingspeak.com/update?api_key=" + String(apiKey) + 
                    "&field1=" + String(key) + "&field2=" + String(access) + "&field3=" + String(anomaly);
       http.begin(url); http.GET(); http.end();
    }
  }
}