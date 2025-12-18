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

Adafruit_SSD1306 display(128, 64, &Wire, -1);

#define BTN_NEW 13
#define BTN_DONE 12
#define POT_SPEED 34

int queueLength = 0;
int avgServiceTime = 2; 
int predictedWait = 0;
String managerDigest = "Store is Quiet";

void setup_wifi() {
  Serial.print("WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(" OK");
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  pinMode(BTN_NEW, INPUT_PULLUP);
  pinMode(BTN_DONE, INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);
}

void loop() {
  static unsigned long lastDebounce = 0;
  if (millis() - lastDebounce > 200) {
    if (digitalRead(BTN_NEW) == LOW) {
      queueLength++;
      lastDebounce = millis();
    }
    if (digitalRead(BTN_DONE) == LOW) {
      if (queueLength > 0) queueLength--;
      lastDebounce = millis();
    }
  }

  avgServiceTime = map(analogRead(POT_SPEED), 0, 4095, 1, 10);

  predictedWait = queueLength * avgServiceTime;

  if (predictedWait > 15) {
    managerDigest = "CRITICAL: Open Register #2!";
  } else if (predictedWait > 5) {
    managerDigest = "Busy. Monitor flow.";
  } else {
    managerDigest = "Optimal flow.";
  }

  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.printf("Staff Speed: %d m/p\n", avgServiceTime);
  
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 20);
  display.printf("Q: %d ppl\n", queueLength);
  
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.printf("Wait: ~%d min\n", predictedWait);
  
  display.setCursor(0, 54);
  display.print(managerDigest);
  
  display.display();

  static unsigned long lastSend = 0;
  if (millis() - lastSend > 15000) {
    lastSend = millis();
    if(WiFi.status() == WL_CONNECTED){
       HTTPClient http;
       String url = String(server) + "/update?api_key=" + String(apiKey) + 
                    "&field1=" + String(queueLength) + 
                    "&field2=" + String(predictedWait) +
                    "&field3=" + String(avgServiceTime);
       http.begin(url); http.GET(); http.end();
    }
  }
  delay(100);
}