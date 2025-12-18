#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

const char* apiKey = "OQA4QZKCQ92DUUJU";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

DHT dht(15, DHT22);
#define POT_SMOKE 34
#define RELAY_VENT 2

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  dht.begin();
  pinMode(RELAY_VENT, OUTPUT);
}

void loop() {
  float temp = dht.readTemperature();
  int smokeRaw = analogRead(POT_SMOKE);

  int riskScore = 0;
  if (temp > 40) riskScore += 50;
  if (smokeRaw > 1000) riskScore += 50;


  if (riskScore >= 50) {
    digitalWrite(RELAY_VENT, HIGH); 
    Serial.println("RISK HIGH! Vent ON");
  } else {
    digitalWrite(RELAY_VENT, LOW);
  }

  static unsigned long t = 0;
  if (millis() - t > 15000) {
    t = millis();
    if(WiFi.status() == WL_CONNECTED){
       HTTPClient http;
       String url = "http://api.thingspeak.com/update?api_key=" + String(apiKey) + 
                    "&field1=" + String(temp) + "&field2=" + String(riskScore);
       http.begin(url); http.GET(); http.end();
    }
  }
  delay(1000);
}