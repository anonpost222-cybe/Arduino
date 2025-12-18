#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* apiKey = "5LP7Y9QQ5XZGX1QY";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define PIN_GPS_SIM 34
#define PIN_SOS 13

float baseLat = 50.4501; 
float currentLat = 50.4501;
bool sosActive = false;
bool safeZone = true;

void setup_wifi() {
  Serial.print("WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(" OK");
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  pinMode(PIN_SOS, INPUT_PULLUP);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.println("GPS Tracker Init...");
  display.display();
  delay(1000);
}

void loop() {
  if (digitalRead(PIN_SOS) == LOW) {
    sosActive = true;
    Serial.println("SOS ACTIVATED!");
  }

  int val = analogRead(PIN_GPS_SIM);
  float offset = map(val, 0, 4095, 0, 500) / 10000.0;
  currentLat = baseLat + offset;

  if (offset > 0.0300) safeZone = false;
  else safeZone = true;

  display.clearDisplay();
  
  display.setCursor(0,0);
  display.print("Lat: "); display.println(currentLat, 4);
  
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setCursor(0,20);
  display.setTextSize(2);
  
  if (sosActive) {
    display.print("!!! SOS !!!");
  } else if (!safeZone) {
    display.print("WARNING!");
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.print("Left Safe Zone");
  } else {
    display.print("SAFE ZONE");
  }
  
  display.display();

  static unsigned long t = 0;
  if (millis() - t > 15000) {
    t = millis();
    if(WiFi.status() == WL_CONNECTED){
       HTTPClient http;
       String url = "http://api.thingspeak.com/update?api_key=" + String(apiKey) + 
                    "&field1=" + String(currentLat, 6) + 
                    "&field2=" + String(sosActive) + 
                    "&field3=" + String(safeZone);
       http.begin(url); 
       http.GET(); 
       http.end();
       Serial.println("Data sent to Cloud");
    }
  }
  delay(100);
}