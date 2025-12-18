#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* apiKey = "ISIRPZHOAVGD11FF";
const char* server = "http://api.thingspeak.com";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define POT_PIN 34
#define LED_PIN 2
#define BUZZER_PIN 13

float filteredBPM = 60.0;
float alpha = 0.1;

int rawBPM = 0;
int spo2 = 98;
int status = 0; 

void setup_wifi();
void sendToThingSpeak();
void updateDisplay();
void checkAnomalies();

void setup() {
  Serial.begin(115200);
  setup_wifi();

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.println("BioMonitor AI Init...");
  display.display();
  delay(1000);
}

void loop() {
  int sensorVal = analogRead(POT_PIN);
  int baseBPM = map(sensorVal, 0, 4095, 40, 180);
  int noise = random(-5, 6); 
  rawBPM = baseBPM + noise;


  filteredBPM = (alpha * rawBPM) + ((1.0 - alpha) * filteredBPM);

  if (filteredBPM > 140) spo2 = random(88, 94);
  else spo2 = random(96, 100);

  checkAnomalies();

  updateDisplay();

  static unsigned long timer = 0;
  if (millis() - timer > 15000) {
    timer = millis();
    sendToThingSpeak();
  }
  
  delay(200);
}

void checkAnomalies() {
  bool anomaly = false;

  if (filteredBPM > 120 || filteredBPM < 50) anomaly = true;

  if (spo2 < 90) anomaly = true;

  if (anomaly) {
    status = 2; 
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 1000, 100); 
  } else {
    status = 0; 
    digitalWrite(LED_PIN, LOW);
  }
}

void updateDisplay() {
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("WiFi: OK | "); 
  if (status == 2) display.print("ALERT!");
  else display.print("Normal");

  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 20);
  display.print("BPM: "); display.println((int)filteredBPM);

  display.setTextSize(1);
  display.setCursor(0, 45);
  display.print("SpO2: "); display.print(spo2); display.println("%");

  display.setCursor(0, 55);
  display.print("Raw Input: "); display.print(rawBPM);

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
                 "&field1=" + String((int)filteredBPM) + 
                 "&field2=" + String(spo2) +
                 "&field3=" + String(status);
    http.begin(url);
    http.GET();
    http.end();
  }
}