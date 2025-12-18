#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

const char* apiKey = "5LP7Y9QQ5XZGX1QY"; 
const char* server = "http://api.thingspeak.com";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

Adafruit_SSD1306 display(128, 64, &Wire, -1);

Adafruit_MPU6050 mpu;
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define LED_ALARM 2

void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" OK");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_ALARM, OUTPUT);


  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.println("System Boot...");
  display.display();

  setup_wifi();
  dht.begin();
  
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found");
    while (1) delay(10);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void loop() {
  float temp = dht.readTemperature();
  if (isnan(temp)) temp = 0.0;

  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  // Розрахунок вібрації (магнітуда по X та Y)
  float vib = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2));

  // 2. AI Multi-factor Diagnosis (Expert System)
  // 0=OK, 1=Overheat, 2=Unbalance, 3=Bearing Fault
  int faultCode = 0;
  String diagnosis = "System Healthy";
  bool alarm = false;

  // Правило 1: Найкритичніше (Вібрація + Температура) -> Знос підшипника
  if (vib > 15.0 && temp > 45.0) {
    faultCode = 3;
    diagnosis = "CRIT: Bearing Fault!";
    alarm = true;
  }
  // Правило 2: Тільки вібрація -> Дисбаланс ротора
  else if (vib > 12.0) {
    faultCode = 2;
    diagnosis = "Warn: Unbalance";
    alarm = true;
  }
  // Правило 3: Тільки температура -> Перегрів
  else if (temp > 50.0) {
    faultCode = 1;
    diagnosis = "Warn: Overheat";
    alarm = true;
  }

  // LED Indication
  digitalWrite(LED_ALARM, alarm ? HIGH : LOW);

  // 3. Візуалізація
  display.clearDisplay();
  display.setCursor(0,0);
  display.printf("Temp: %.1f C\n", temp);
  display.printf("Vib:  %.1f m/s2\n", vib);
  
  display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
  
  display.setCursor(0, 25);
  display.println("AI Diagnosis:");
  if (faultCode == 3) display.setTextColor(SSD1306_INVERSE);
  display.println(diagnosis);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 55);
  display.printf("Code: %d", faultCode);
  display.display();

  // 4. Відправка на ThingSpeak (кожні 15 сек)
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 15000) {
    lastSend = millis();
    if(WiFi.status() == WL_CONNECTED){
       HTTPClient http;
       String url = String(server) + "/update?api_key=" + String(apiKey) + 
                    "&field1=" + String(vib) + 
                    "&field2=" + String(temp) + 
                    "&field3=" + String(faultCode);
       
       int httpCode = http.GET();
       if (httpCode > 0) Serial.printf("Cloud Update: %d\n", httpCode);
       else Serial.println("Cloud Error");
       
       http.end();
    }
  }

  delay(200);
}