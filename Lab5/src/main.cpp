#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <ESP32Servo.h>

const char* apiKey = "OQA4QZKCQ92DUUJU";
const char* server = "http://api.thingspeak.com";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

#define BUZZER_PIN 15
#define SERVO_PIN 13
#define BUTTON_PIN 4

Adafruit_MPU6050 mpu;
Servo safeLock;

float baselineGravity = 9.8;
float vibrationThreshold = 2.5; 
bool alarmActive = false;
float maxImpact = 0;

void setup_wifi();
void sendToThingSpeak(float impact, int alarm);

void setup() {
  Serial.begin(115200);
  setup_wifi();

  Wire.begin(21, 22);
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1) delay(10);
  }
  
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  safeLock.attach(SERVO_PIN);
  safeLock.write(0);
  
  Serial.println("System ARMED. Don't touch the safe!");
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float totalAccel = sqrt(a.acceleration.x*a.acceleration.x + 
                          a.acceleration.y*a.acceleration.y + 
                          a.acceleration.z*a.acceleration.z);
  
  float vibration = abs(totalAccel - baselineGravity);

  if (vibration > maxImpact) maxImpact = vibration;

  if (vibration > vibrationThreshold && !alarmActive) {
    alarmActive = true;
    Serial.print(">>> IMPACT DETECTED! Level: "); Serial.println(vibration);
    sendToThingSpeak(vibration, 1);
  }


  if (alarmActive) {
    digitalWrite(BUZZER_PIN, HIGH);
    tone(BUZZER_PIN, 1000); 
    safeLock.write(90); 
  } else {
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
    if (alarmActive) {
      Serial.println("Alarm Reset by User.");
      sendToThingSpeak(0, 0);
    }
    alarmActive = false;
    maxImpact = 0;
    delay(500);
  }

  static unsigned long timer = 0;
  if (millis() - timer > 15000 && !alarmActive) {
    timer = millis();
    sendToThingSpeak(maxImpact, 0);
    maxImpact = 0;
  }
  
  delay(100);
}

void setup_wifi() {
  Serial.print("WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" OK");
}

void sendToThingSpeak(float impact, int alarm) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "/update?api_key=" + String(apiKey) + 
                 "&field1=" + String(impact) + 
                 "&field2=" + String(alarm);
    http.begin(url);
    http.GET();
    http.end();
    Serial.println("Data sent to Cloud.");
  }
}