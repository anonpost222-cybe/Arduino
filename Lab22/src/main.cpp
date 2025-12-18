#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

const char* apiKey = "5LP7Y9QQ5XZGX1QY";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Adafruit_MPU6050 mpu;
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define LED_ALARM 2

void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" OK");
}

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_ALARM, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.println("Connecting WiFi...");
  display.display();

  setup_wifi();

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) delay(10);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  dht.begin();
}

void loop() {
  float temp = dht.readTemperature();
  if (isnan(temp)) temp = 0.0;

  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  float vibration = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2));

  bool alarm = false;
  String statusMsg = "System OK";

  if (vibration > 10.0) {
    alarm = true;
    statusMsg = "HIGH VIBRATION!";
  }
  if (temp > 50.0) {
    alarm = true;
    statusMsg = "OVERHEAT!";
  }

  digitalWrite(LED_ALARM, alarm ? HIGH : LOW);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("MOTOR GUARD (WiFi)");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setCursor(0, 20);
  display.printf("Temp: %.1f C\n", temp);
  display.printf("Vib:  %.2f m/s2\n", vibration);

  display.drawRect(0, 45, 128, 18, SSD1306_WHITE);
  display.setCursor(5, 50);
  if (alarm) display.setTextColor(SSD1306_INVERSE);
  display.println(statusMsg);
  display.setTextColor(SSD1306_WHITE);

  display.display();

  static unsigned long lastSend = 0;
  if (millis() - lastSend > 15000) {
    lastSend = millis();
    if(WiFi.status() == WL_CONNECTED){
       HTTPClient http;
       String url = "http://api.thingspeak.com/update?api_key=" + String(apiKey) + 
                    "&field1=" + String(vibration) + 
                    "&field2=" + String(temp) + 
                    "&field3=" + String(alarm);
       
       int httpCode = http.GET();
       if (httpCode > 0) {
         Serial.printf("ThingSpeak sent. Code: %d\n", httpCode);
       } else {
         Serial.println("Error sending to ThingSpeak");
       }
       http.end();
    }
  }

  delay(200);
}