#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <ESP32Servo.h>

const char* apiKey = "OQA4QZKCQ92DUUJU";
const char* server = "http://api.thingspeak.com";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHTPIN 15
#define DHTTYPE DHT22
#define POT_PIN 34
#define RELAY_PIN 2
#define SERVO_PIN 13
#define LED_ERR 4

DHT dht(DHTPIN, DHTTYPE);
Servo valveServo;

float temp = 25.0;
int soilMoisture = 0;
int adaptiveThreshold = 30;
bool pumpState = false;
String systemMsg = "System OK";
int errorCode = 0;

void setup_wifi();
void sendToThingSpeak();
void updateDisplay();
void runAdaptiveLogic();

void setup() {
  Serial.begin(115200);
  setup_wifi();

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_ERR, OUTPUT);
  dht.begin();
  valveServo.attach(SERVO_PIN);
  valveServo.write(0); 

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.println("Agro Control Init...");
  display.display();
  delay(1000);
}

void loop() {
  float t = dht.readTemperature();
  if (!isnan(t)) temp = t;
  
  int val = analogRead(POT_PIN);
  soilMoisture = map(val, 0, 4095, 0, 100);

  runAdaptiveLogic();

  if (pumpState) {
    digitalWrite(RELAY_PIN, HIGH);
    valveServo.write(90); 
  } else {
    digitalWrite(RELAY_PIN, LOW);
    valveServo.write(0);  
  }

  digitalWrite(LED_ERR, (errorCode > 0) ? HIGH : LOW);

  updateDisplay();

  static unsigned long timer = 0;
  if (millis() - timer > 15000) {
    timer = millis();
    sendToThingSpeak();
  }
  
  delay(500);
}

void runAdaptiveLogic() {

  if (temp > 30.0) {
    adaptiveThreshold = 60; 
  } else if (temp < 15.0) {
    adaptiveThreshold = 15; 
  } else {
    adaptiveThreshold = 30 + (int)(temp - 20); 
  }
  
  if (soilMoisture < adaptiveThreshold) {
    pumpState = true;
    systemMsg = "Watering...";
    errorCode = 0;
  } 
  else if (soilMoisture > (adaptiveThreshold + 10)) {
    pumpState = false;
    systemMsg = "Soil Optimal";
    errorCode = 0;
  }


  if (pumpState && soilMoisture == 0) {
    systemMsg = "ERR: Check Water Supply!";
    errorCode = 1;
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Temp: "); display.print(temp, 1); display.println(" C");
  
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setCursor(0, 20);
  display.print("Soil: "); display.print(soilMoisture); display.println("%");
  
  display.setCursor(0, 30);
  display.print("Threshold: <"); display.print(adaptiveThreshold); display.println("%");

  display.setTextSize(1);
  display.setCursor(0, 45);
  if (errorCode > 0) display.setTextColor(SSD1306_INVERSE);
  display.println(systemMsg);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(80, 55);
  display.print(pumpState ? "[PUMP ON]" : "[IDLE]");

  display.display();
}

void setup_wifi() {
  Serial.print("WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" OK");
}

void sendToThingSpeak() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "/update?api_key=" + String(apiKey) + 
                 "&field1=" + String(soilMoisture) + 
                 "&field2=" + String(adaptiveThreshold) +
                 "&field3=" + String(pumpState) +
                 "&field4=" + String(errorCode);
    http.begin(url);
    http.GET();
    http.end();
  }
}