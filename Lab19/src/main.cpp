#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

const char* apiKey = "OQA4QZKCQ92DUUJU"; 
const char* server = "http://api.thingspeak.com";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define LDR_PIN 34
#define RELAY_PIN 2

float temp = 0;
int lightLevel = 0; 
int optimizationMode = 0; 
String aiStatus = "Init...";

void setup_wifi() {
  Serial.print("WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" OK");
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  dht.begin();
  pinMode(RELAY_PIN, OUTPUT);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.println("Agro-Optima AI v1.0");
  display.display();
  delay(1000);
}

void loop() {
  float t = dht.readTemperature();
  if (!isnan(t)) temp = t;
  
  int rawLdr = analogRead(LDR_PIN);
  lightLevel = map(rawLdr, 0, 4095, 100, 0); 


  if (lightLevel < 30) {
    if (temp < 30.0) {
      digitalWrite(RELAY_PIN, HIGH); 
      optimizationMode = 2; 
      aiStatus = "Boosting Growth";
    } else {
      digitalWrite(RELAY_PIN, LOW); 
      optimizationMode = 1; 
      aiStatus = "Heat Protection!";
    }
  } else {
    digitalWrite(RELAY_PIN, LOW);
    optimizationMode = 0;
    aiStatus = "Natural Light";
  }

  display.clearDisplay();
  display.setCursor(0,0);
  display.printf("Temp: %.1f C\n", temp);
  display.printf("Light: %d%%\n", lightLevel);
  
  display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
  
  display.setCursor(0, 25);
  display.println("AI Decision:");
  display.println(aiStatus);

  display.setCursor(0, 55);
  if (digitalRead(RELAY_PIN)) display.print("[LAMP ON]");
  else display.print("[LAMP OFF]");
  
  display.display();

  static unsigned long lastSend = 0;
  if (millis() - lastSend > 15000) {
    lastSend = millis();
    if(WiFi.status() == WL_CONNECTED){
       HTTPClient http;
       String url = String(server) + "/update?api_key=" + String(apiKey) + 
                    "&field1=" + String(lightLevel) + 
                    "&field2=" + String(optimizationMode);
       http.begin(url); 
       http.GET(); 
       http.end();
       Serial.println("Data sent to Cloud");
    }
  }
  
  delay(500);
}