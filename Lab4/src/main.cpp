#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <ESP32Servo.h>

const char* apiKey = "5LP7Y9QQ5XZGX1QY";
const char* server = "http://api.thingspeak.com";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

#define DHTPIN 15
#define DHTTYPE DHT22
#define LDRPIN 34
#define SERVOPIN 13
#define PUMPPIN 2

DHT dht(DHTPIN, DHTTYPE);
Servo windowServo;

float temperature = 0;
float humidity = 0;
int lightLevel = 0;
int aiDecision = 0; 
String aiStatus = "Normal";

void setup_wifi();
void sendToThingSpeak();
void runAiLogic();
void actuateDevices();

void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  dht.begin();
  windowServo.attach(SERVOPIN);
  pinMode(PUMPPIN, OUTPUT);
  
  Serial.println("Smart Greenhouse AI Initialized...");
}

void loop() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  lightLevel = analogRead(LDRPIN);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  runAiLogic();

  actuateDevices();

  Serial.print("T: "); Serial.print(temperature);
  Serial.print(" H: "); Serial.print(humidity);
  Serial.print(" L: "); Serial.print(lightLevel);
  Serial.print(" | AI: "); Serial.println(aiStatus);

  static unsigned long lastTimer = 0;
  if (millis() - lastTimer > 15000) {
    lastTimer = millis();
    sendToThingSpeak();
  }
  
  delay(2000);
}

void runAiLogic() {
  if (temperature > 30.0) {
    aiDecision = 2; 
    aiStatus = "Critical Heat! Ventilating...";
  }
  else if (humidity < 40.0 && temperature > 20.0) {
    aiDecision = 1;
    aiStatus = "Dry Soil! Watering...";
  }
  else {
    aiDecision = 0;
    aiStatus = "Optimal Environment";
  }
}

void actuateDevices() {
  if (aiDecision == 2) {
    windowServo.write(90); 
    digitalWrite(PUMPPIN, LOW);
  } else if (aiDecision == 1) {
    windowServo.write(0);  
    digitalWrite(PUMPPIN, HIGH);
  } else {
    windowServo.write(0);
    digitalWrite(PUMPPIN, LOW);
  }
}

void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" Connected!");
}

void sendToThingSpeak() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "/update?api_key=" + String(apiKey) + 
                 "&field1=" + String(temperature) + 
                 "&field2=" + String(humidity) +
                 "&field3=" + String(lightLevel) +
                 "&field4=" + String(aiDecision);
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) Serial.println("ThingSpeak Sent: OK");
    http.end();
  }
}