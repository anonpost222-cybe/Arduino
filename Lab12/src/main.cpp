#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

const char* apiKey = "LTT0IHEEOB82KSP0"; 
const char* server = "http://api.thingspeak.com";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

Adafruit_MPU6050 mpu;

#define PIN_RED 15
#define PIN_GREEN 2
#define PIN_BLUE 4

int gestureID = 0; 
float confidence = 0.0;

void setup_wifi();
void sendToThingSpeak();
void classifyGesture(sensors_event_t a, sensors_event_t g);

void setup() {
  Serial.begin(115200);
  setup_wifi();

  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);

  Wire.begin(21, 22);
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found");
    while (1) delay(10);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  classifyGesture(a, g);

  digitalWrite(PIN_GREEN, (gestureID == 0));
  digitalWrite(PIN_RED,   (gestureID == 1));
  digitalWrite(PIN_BLUE,  (gestureID == 2));

  static unsigned long timer = 0;
  if (millis() - timer > 15000) {
    timer = millis();
    sendToThingSpeak();
  }
  delay(100);
}

void classifyGesture(sensors_event_t a, sensors_event_t g) {
  float accelMag = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));
  float gyroMag = sqrt(pow(g.gyro.x, 2) + pow(g.gyro.y, 2) + pow(g.gyro.z, 2));

  if (accelMag > 15.0) { 
    gestureID = 1; 
    confidence = (accelMag / 20.0) * 100;
    if (confidence > 100) confidence = 100;
    Serial.println("Detected: SHAKE!");
  } 
  else if (abs(a.acceleration.x) > 5.0 || abs(a.acceleration.y) > 5.0) {
    gestureID = 2; 
    confidence = 85.0;
    Serial.println("Detected: TILT");
  } 
  else {
    gestureID = 0; 
    confidence = 99.9;
  }
}

void setup_wifi() {
  Serial.print("WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(" OK");
}

void sendToThingSpeak() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "/update?api_key=" + String(apiKey) + 
                 "&field1=" + String(gestureID) + "&field2=" + String(confidence);
    http.begin(url); http.GET(); http.end();
  }
}