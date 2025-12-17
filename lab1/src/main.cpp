#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

const char* apiKey = "LTT0IHEEOB82KSP0";
const char* server = "http://api.thingspeak.com";

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7200;
const int daylightOffset_sec = 3600;

#define LED_PIN 15
#define LDR_PIN 34
#define BUTTON_PIN 4

enum State { STATE_OFF, STATE_ON, STATE_AUTO };
State currentState = STATE_AUTO;

int lightThresholdOn = 1500;
int lightThresholdOff = 2000;
int ldrValue = 0;
bool isLampOn = false;

float usageProbability[24] = {
  0.1, 0.0, 0.0, 0.0, 0.0, 0.1,
  0.4, 0.8, 0.2, 0.1, 0.0, 0.0,
  0.0, 0.0, 0.0, 0.1, 0.3, 0.8,
  0.9, 0.9, 0.8, 0.6, 0.3, 0.2
};

void setup_wifi();
void sendToThingSpeak(int light, int state);
int getCurrentHour();
void handleLogic();

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  setup_wifi();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  handleLogic();
  digitalWrite(LED_PIN, isLampOn ? HIGH : LOW);

  static unsigned long lastTimer = 0;
  if (millis() - lastTimer > 15000) {
    lastTimer = millis();
    sendToThingSpeak(ldrValue, isLampOn ? 1 : 0);
  }
  
  delay(10); 
}

void handleLogic() {
  ldrValue = analogRead(LDR_PIN);
  int currentHour = getCurrentHour();

  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(20); 
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (currentState == STATE_AUTO) currentState = STATE_ON;
      else if (currentState == STATE_ON) currentState = STATE_OFF;
      else currentState = STATE_AUTO;
      
      Serial.println("\n>>> BUTTON PRESSED! Mode Changed <<<");
      while(digitalRead(BUTTON_PIN) == LOW) delay(10);
    }
  }

  float aiFactor = usageProbability[currentHour];
  int adaptiveThreshold = lightThresholdOn + (aiFactor * 1000);

  switch (currentState) {
    case STATE_OFF:
      isLampOn = false;
      break;
    case STATE_ON:
      isLampOn = true;
      break;
    case STATE_AUTO:
      if (ldrValue < adaptiveThreshold) isLampOn = true;
      else if (ldrValue > lightThresholdOff) isLampOn = false;
      break;
  }

  static unsigned long logTimer = 0;
  if (millis() - logTimer > 500) {
    logTimer = millis();
    Serial.print("LDR: "); Serial.print(ldrValue);
    Serial.print(" | Thresh: "); Serial.print(adaptiveThreshold);
    Serial.print(" | Mode: "); Serial.print(currentState);
    Serial.print(" | Lamp: "); Serial.println(isLampOn);
  }
}

void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" Connected!");
}

void sendToThingSpeak(int light, int state) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "/update?api_key=" + String(apiKey) +
                 "&field1=" + String(light) + "&field2=" + String(state);
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) Serial.println("ThingSpeak Sent: OK");
    else Serial.println("ThingSpeak Error");
    http.end();
  }
}

int getCurrentHour() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return 12;
  }
  return timeinfo.tm_hour;
}