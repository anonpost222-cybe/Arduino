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
#define PIR_PIN 13
#define RELAY_PIN 2

float simHour = 6.0; 
int startHourSchedule = 9; 
int detectedStartHour = 9; 

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);
}

void loop() {
  simHour += 0.1; 
  if (simHour >= 24.0) simHour = 0.0;

  bool motion = digitalRead(PIR_PIN);

  bool systemOn = false;
  if (simHour >= startHourSchedule && simHour < 18.0) {
    systemOn = true; 
  } else {
    if (motion) systemOn = true; 
  }

  
  if (motion && simHour < startHourSchedule && simHour > 6.0) {
     detectedStartHour = (int)simHour; 
  }

  digitalWrite(RELAY_PIN, systemOn ? HIGH : LOW);

  display.clearDisplay();
  display.setCursor(0,0);
  display.printf("Time: %02d:%02d\n", (int)simHour, (int)((simHour-(int)simHour)*60));
  display.printf("Motion: %s\n", motion ? "YES" : "NO");
  display.printf("Relay:  %s\n", systemOn ? "ON" : "OFF");
  display.drawLine(0, 35, 128, 35, SSD1306_WHITE);
  display.setCursor(0, 40);
  if (detectedStartHour < startHourSchedule) {
    display.printf("AI: Suggest Start\n @ %02d:00", detectedStartHour);
  } else {
    display.printf("AI: Schedule OK");
  }
  display.display();

  static unsigned long last = 0;
  if (millis() - last > 15000) {
    last = millis();
    if(WiFi.status() == WL_CONNECTED){
       HTTPClient http;
       String url = "http://api.thingspeak.com/update?api_key=" + String(apiKey) + 
                    "&field1=" + String(motion) + "&field2=" + String(systemOn) + "&field3=" + String(detectedStartHour);
       http.begin(url); http.GET(); http.end();
    }
  }
  delay(500); 
}