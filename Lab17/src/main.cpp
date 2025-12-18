#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* apiKey = "ISIRPZHOAVGD11FF";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

Adafruit_SSD1306 display(128, 64, &Wire, -1);
#define POT_W 34
#define POT_T 35
#define BUZZ 15

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  pinMode(BUZZ, OUTPUT);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);
}

void loop() {
  int w = map(analogRead(POT_W), 0, 4095, 0, 1000); 
  int t = map(analogRead(POT_T), 0, 4095, 20, 100); 

  String status = "Raw";
  int donenessCode = 0;
  
  if (t < 50) { status = "Cooking..."; donenessCode=0; }
  else if (t >= 50 && t < 60) { status = "Rare"; donenessCode=1; }
  else if (t >= 60 && t < 70) { status = "Medium"; donenessCode=2; }
  else { status = "Well Done"; donenessCode=3; tone(BUZZ, 2000, 100); }

  display.clearDisplay();
  display.setCursor(0,0);
  display.printf("Weight: %d g\n", w);
  display.printf("Temp:   %d C\n", t);
  display.println("--------------");
  display.printf("AI: %s", status.c_str());
  display.display();

  static unsigned long last = 0;
  if (millis() - last > 15000) {
    last = millis();
    if(WiFi.status() == WL_CONNECTED){
       HTTPClient http;
       String url = "http://api.thingspeak.com/update?api_key=" + String(apiKey) + 
                    "&field1=" + String(w) + "&field2=" + String(t) + "&field3=" + String(donenessCode);
       http.begin(url); http.GET(); http.end();
    }
  }
  delay(200);
}