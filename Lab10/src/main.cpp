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
#define POT_SOIL 34
#define POT_PH 35
#define PUMP_PIN 2

DHT dht(DHTPIN, DHTTYPE);

float temp = 0;
float hum = 0;
int soilMoisture = 0;
float phLevel = 7.0;
int hoursToWater = 24;
String llmDigest = "Analyzing...";

void setup_wifi();
void sendToThingSpeak();
void updateDisplay();
void runAiAgroLogic();

void setup() {
  Serial.begin(115200);
  setup_wifi();

  pinMode(PUMP_PIN, OUTPUT);
  dht.begin();

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.println("Agro AI System v1.0");
  display.display();
  delay(1000);
}

void loop() {

  temp = dht.readTemperature();
  hum = dht.readHumidity();
  
  int rawSoil = analogRead(POT_SOIL);
  soilMoisture = map(rawSoil, 0, 4095, 0, 100);

  int rawPh = analogRead(POT_PH);
  phLevel = rawPh * (14.0 / 4095.0);

  runAiAgroLogic();

  if (soilMoisture < 30) digitalWrite(PUMP_PIN, HIGH);
  else digitalWrite(PUMP_PIN, LOW);

  updateDisplay();

  static unsigned long timer = 0;
  if (millis() - timer > 15000) {
    timer = millis();
    sendToThingSpeak();
  }
  
  delay(500);
}

void runAiAgroLogic() {
  
  float dryRate = 1.0;
  if (temp > 30) dryRate = 2.0; 
  
  int moistureGap = soilMoisture - 30; 
  if (moistureGap < 0) hoursToWater = 0;
  else hoursToWater = moistureGap / dryRate;

  
  String status = "";
  if (soilMoisture < 30) status = "Dry!";
  else if (soilMoisture > 80) status = "Wet.";
  else status = "OK.";

  String phStatus = "";
  if (phLevel < 6.0) phStatus = "Acidic";
  else if (phLevel > 7.5) phStatus = "Alkaline";
  else phStatus = "Neutral";


  if (hoursToWater == 0) {
    llmDigest = "CRITICAL: Water NOW! Soil is " + status;
  } else {
    llmDigest = "Stable. " + phStatus + " soil. Water in " + String(hoursToWater) + "h";
  }
}

void updateDisplay() {
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("T:"); display.print((int)temp);
  display.print("C H:"); display.print((int)hum); display.println("%");

  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setCursor(0, 20);
  display.print("Soil: "); display.print(soilMoisture); display.println("%");
  display.print("pH: "); display.println(phLevel, 1);

  display.drawRect(0, 38, 128, 26, SSD1306_WHITE);
  display.setCursor(4, 42);
  
  if (llmDigest.length() > 20) {
     display.print(llmDigest.substring(0, 20));
     display.setCursor(4, 52);
     display.print(llmDigest.substring(20));
  } else {
     display.print(llmDigest);
  }

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
                 "&field2=" + String(phLevel) +
                 "&field3=" + String(temp) +
                 "&field4=" + String(hoursToWater);
    http.begin(url);
    http.GET();
    http.end();
  }
}