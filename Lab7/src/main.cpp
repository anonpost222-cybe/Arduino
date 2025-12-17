#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* apiKey = "LTT0IHEEOB82KSP0";
const char* server = "http://api.thingspeak.com";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define POT_PIN 34
#define BTN_IN 13
#define BTN_OUT 12
#define LED_PIN 2

const int MAX_CAPACITY = 20;
const int CO2_WARNING = 1000; 

int peopleCount = 0;
int co2Level = 400;
float fillRate = 0; 
int forecastMinutes = 99; 

void setup_wifi();
void sendToThingSpeak();
void updateDisplay();
void runAiAnalytics();

void setup() {
  Serial.begin(115200);
  setup_wifi();

  pinMode(BTN_IN, INPUT_PULLUP);
  pinMode(BTN_OUT, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.println("Bus AI System Init...");
  display.display();
  delay(1000);
}

void loop() {
  int potValue = analogRead(POT_PIN);
  co2Level = map(potValue, 0, 4095, 400, 2500);

  static unsigned long lastDebounce = 0;
  if (millis() - lastDebounce > 200) {
    if (digitalRead(BTN_IN) == LOW) {
      peopleCount++;
      if (peopleCount > MAX_CAPACITY) peopleCount = MAX_CAPACITY;
      lastDebounce = millis();
      runAiAnalytics();
    }
    if (digitalRead(BTN_OUT) == LOW) {
      if (peopleCount > 0) peopleCount--;
      lastDebounce = millis();
    }
  }

  if (co2Level > CO2_WARNING || peopleCount >= MAX_CAPACITY) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  updateDisplay();

  static unsigned long timer = 0;
  if (millis() - timer > 15000) {
    timer = millis();
    sendToThingSpeak();
  }
  
  delay(100);
}

void runAiAnalytics() {

  
  int seatsLeft = MAX_CAPACITY - peopleCount;
  if (seatsLeft <= 0) {
    forecastMinutes = 0;
  } else {
    fillRate = 2.0; 
    forecastMinutes = seatsLeft / fillRate;
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("WiFi: OK | Cap: "); display.print(MAX_CAPACITY);
  
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 15);
  display.print("Ppl: "); display.println(peopleCount);

  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print("CO2: "); display.print(co2Level); display.println(" ppm");

  display.setCursor(0, 50);
  display.print("Full in: "); 
  if (forecastMinutes == 0) display.print("NOW!");
  else { display.print(forecastMinutes); display.print(" min"); }

  display.display();
}

void setup_wifi() {
  Serial.print("Connecting");
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
                 "&field1=" + String(peopleCount) + 
                 "&field2=" + String(co2Level) +
                 "&field3=" + String(forecastMinutes);
    http.begin(url);
    http.GET();
    http.end();
  }
}