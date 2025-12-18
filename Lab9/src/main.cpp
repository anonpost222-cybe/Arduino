#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* apiKey = "5LP7Y9QQ5XZGX1QY";
const char* server = "http://api.thingspeak.com";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define POT_PIN 34
#define TRIG_PIN 5
#define ECHO_PIN 18
#define LED_ECO 2

int speedKmh = 0;
long distanceCm = 0;
int fuelEff = 100; 
String aiTip = "Ready";

void setup_wifi();
void sendToThingSpeak();
void updateDisplay();
void runAiEcoLogic();
long readDistance();

void setup() {
  Serial.begin(115200);
  setup_wifi();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_ECO, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.println("On-Board AI Init...");
  display.display();
  delay(1000);
}

void loop() {
  int val = analogRead(POT_PIN);
  speedKmh = map(val, 0, 4095, 0, 160);

  distanceCm = readDistance();

  runAiEcoLogic();

  updateDisplay();

  static unsigned long timer = 0;
  if (millis() - timer > 15000) {
    timer = millis();
    sendToThingSpeak();
  }
  
  delay(200);
}

void runAiEcoLogic() {

  if (distanceCm < 50 && speedKmh > 5) {
    aiTip = "!!! BRAKE !!!";
    fuelEff = 0;
    digitalWrite(LED_ECO, LOW);
    return;
  }

  if (speedKmh == 0) {
    aiTip = "Idling... (Stop?)";
    fuelEff = 10;
    digitalWrite(LED_ECO, LOW);
  } 
  else if (speedKmh > 0 && speedKmh <= 60) {
    aiTip = "City Mode: OK";
    fuelEff = 85;
    digitalWrite(LED_ECO, HIGH);
  }
  else if (speedKmh > 60 && speedKmh <= 90) {
    aiTip = "Cruising: Perfect";
    fuelEff = 100; 
    digitalWrite(LED_ECO, HIGH);
  }
  else if (speedKmh > 90 && speedKmh <= 120) {
    aiTip = "Drag high. Slow down";
    fuelEff = 70;
    digitalWrite(LED_ECO, LOW);
  }
  else {
    aiTip = "Fuel Waste! >120";
    fuelEff = 40;
    digitalWrite(LED_ECO, LOW);
  }
}

long readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  long cm = duration * 0.034 / 2;
  if (cm > 400) cm = 400; 
  return cm;
}

void updateDisplay() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("WiFi: OK | "); display.print(distanceCm); display.println("cm");

  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 20);
  display.print(speedKmh); display.println(" km/h");

  display.setTextSize(1);
  display.setCursor(0, 45);
  display.print("AI: "); display.println(aiTip);

  display.drawRect(0, 58, 128, 6, SSD1306_WHITE);
  display.fillRect(0, 58, (128 * fuelEff) / 100, 6, SSD1306_WHITE);

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
                 "&field1=" + String(speedKmh) + 
                 "&field2=" + String(distanceCm) +
                 "&field3=" + String(fuelEff);
    http.begin(url);
    http.GET();
    http.end();
  }
}