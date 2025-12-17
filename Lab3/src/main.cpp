#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* apiKey = "ISIRPZHOAVGD11FF"; 
const char* server = "http://api.thingspeak.com";

const char* ssid = "Wokwi-GUEST";
const char* password = "";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Adafruit_MPU6050 mpu;

String activityState = "Idle";
int activityCode = 0;
int heartRate = 70;


void updateDisplay(float ax, float ay, float az);
void classifyActivity(float magnitude);
void simulateHeartRate(String state);
void sendToThingSpeak();
void setup_wifi();

void setup() {
  Serial.begin(115200);
  
  setup_wifi();
  Wire.begin(21, 22);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.println("Connecting Sensor...");
  display.display();
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip! Check wiring.");
    while (1) delay(10);
  }

  Serial.println("MPU6050 Found!");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float magnitude = sqrt(a.acceleration.x * a.acceleration.x + 
                         a.acceleration.y * a.acceleration.y + 
                         a.acceleration.z * a.acceleration.z);

  classifyActivity(magnitude);
  simulateHeartRate(activityState);
  updateDisplay(a.acceleration.x, a.acceleration.y, a.acceleration.z);

  static unsigned long lastTimer = 0;
  if (millis() - lastTimer > 15000) {
    lastTimer = millis();
    sendToThingSpeak();
  }
  
  delay(100);
}

void classifyActivity(float magnitude) {
  if (magnitude < 11.0 && magnitude > 9.0) {
    activityState = "Resting";
    activityCode = 0;
  } else if (magnitude >= 11.0 && magnitude < 18.0) {
    activityState = "Walking";
    activityCode = 1;
  } else if (magnitude >= 18.0) {
    activityState = "Running";
    activityCode = 2;
  }
}

void simulateHeartRate(String state) {
  if (state == "Resting") {
    if (heartRate > 70) heartRate--;
  } else if (state == "Walking") {
    if (heartRate < 100) heartRate++;
    else if (heartRate > 100) heartRate--;
  } else if (state == "Running") {
    if (heartRate < 160) heartRate += 2;
  }
}

void updateDisplay(float ax, float ay, float az) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("WiFi: Connected"); 
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 15);
  display.println(activityState);
  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print("HR: "); display.print(heartRate); display.println(" bpm");
  display.setCursor(0, 50);
  display.print("Mag: "); display.print(sqrt(ax*ax + ay*ay + az*az));
  display.display();
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" WiFi Connected!");
}

void sendToThingSpeak() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "/update?api_key=" + String(apiKey) + 
                 "&field1=" + String(activityCode) + 
                 "&field2=" + String(heartRate);
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) Serial.println("ThingSpeak Sent: OK");
    else Serial.println("ThingSpeak Error");
    http.end();
  }
}