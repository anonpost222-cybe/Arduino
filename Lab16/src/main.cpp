#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <ESP32Servo.h>

const char* apiKey = "LTT0IHEEOB82KSP0";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

Adafruit_SSD1306 display(128, 64, &Wire, -1);
DHT dht(15, DHT22);
Servo windowServo;

#define POT_SMOKE 34
#define RELAY_PIN 2

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  dht.begin();
  pinMode(RELAY_PIN, OUTPUT);
  windowServo.attach(13);
  windowServo.write(0); 
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);
}

void loop() {
  float temp = dht.readTemperature();
  int smokeRaw = analogRead(POT_SMOKE);

  bool simpleAlert = (temp > 50 || smokeRaw > 2000);

  int riskScore = 0;
  if (temp > 35) riskScore += (temp - 35) * 2; 
  if (smokeRaw > 500) riskScore += map(smokeRaw, 500, 4095, 0, 80); 
  
  if (riskScore > 60) {
    digitalWrite(RELAY_PIN, HIGH);
    windowServo.write(90); 
  } else {
    digitalWrite(RELAY_PIN, LOW);
    windowServo.write(0);
  }

  display.clearDisplay();
  display.setCursor(0,0);
  display.printf("T:%.1f S:%d\n", temp, smokeRaw);
  display.setCursor(0,20);
  display.printf("Simple: %s\n", simpleAlert ? "ALARM" : "OK");
  display.printf("Smart:  %d%%\n", riskScore);
  display.display();


  static unsigned long t = 0;
  if (millis() - t > 15000) {
    t = millis();
    if(WiFi.status() == WL_CONNECTED){
       HTTPClient http;
       String url = "http://api.thingspeak.com/update?api_key=" + String(apiKey) + 
                    "&field1=" + String(simpleAlert) + 
                    "&field2=" + String(riskScore) +
                    "&field3=" + String(riskScore - (simpleAlert ? 100 : 0));
       http.begin(url); http.GET(); http.end();
    }
  }
  delay(500);
}