#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

const char* apiKey = "ISIRPZHOAVGD11FF"; 
const char* server = "http://api.thingspeak.com";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

Servo arm;
#define TRIG_PIN 5
#define ECHO_PIN 18
#define POT_CAM 34
#define BUZZER 15

int objectType = 0; 
long distance = 0;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  arm.attach(13);
  arm.write(0); 
}

void loop() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  distance = pulseIn(ECHO_PIN, HIGH) * 0.034 / 2;

  int camVal = analogRead(POT_CAM); 
  
  if (distance < 20) { 
    if (camVal > 2000) {
      objectType = 2; 
      Serial.println("DANGER: Landmine Detected!");
      tone(BUZZER, 1000); 
      arm.write(90); 
    } else {
      objectType = 1; 
      Serial.println("Object: Safe debris");
      noTone(BUZZER);
      arm.write(45); 
    }
  } else {
    objectType = 0;
    noTone(BUZZER);
    arm.write(0);
  }

  static unsigned long t = 0;
  if (millis() - t > 15000) {
    t = millis();
    if(WiFi.status() == WL_CONNECTED){
       HTTPClient http;
       String url = String(server) + "/update?api_key=" + String(apiKey) + 
                    "&field1=" + String(distance) + "&field2=" + String(objectType);
       http.begin(url); http.GET(); http.end();
    }
  }
  delay(200);
}