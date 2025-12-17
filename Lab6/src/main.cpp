#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* apiKey = "OQA4QZKCQ92DUUJU";
const char* server = "http://api.thingspeak.com";
const char* ssid = "Wokwi-GUEST";
const char* password = "";

#define PIR_PIN 13
#define DOOR_PIN 15
#define LED_PIN 2
#define BUZZER_PIN 4
#define BTN_PIN 12

bool systemArmed = true;
int threatLevel = 0; 
int alarmStatus = 0; 

void setup_wifi();
void sendToThingSpeak();
void runSecurityAI();

void setup() {
  Serial.begin(115200);
  setup_wifi();

  pinMode(PIR_PIN, INPUT);
  pinMode(DOOR_PIN, INPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println("AI Security System Initialized.");
}

void loop() {

  if (digitalRead(BTN_PIN) == LOW) {
    delay(200);
    systemArmed = !systemArmed;
    Serial.print("System Armed: "); Serial.println(systemArmed);
    if (!systemArmed) {
      digitalWrite(LED_PIN, LOW);
      noTone(BUZZER_PIN);
      threatLevel = 0;
    }
  }

  if (systemArmed) {
    runSecurityAI();
  }

  static unsigned long lastLog = 0;
  if (millis() - lastLog > 1000) {
    lastLog = millis();
    Serial.print("Armed: "); Serial.print(systemArmed);
    Serial.print(" | Door: "); Serial.print(digitalRead(DOOR_PIN));
    Serial.print(" | PIR: "); Serial.print(digitalRead(PIR_PIN));
    Serial.print(" | Threat: "); Serial.print(threatLevel);
    Serial.println("%");
  }


  static unsigned long lastSend = 0;
  if (millis() - lastSend > 15000) {
    lastSend = millis();
    sendToThingSpeak();
  }
  
  delay(100);
}

void runSecurityAI() {
  bool motion = digitalRead(PIR_PIN);
  bool doorOpen = (digitalRead(DOOR_PIN) == HIGH); 


  if (doorOpen) {
    threatLevel = 50; 

    if (motion) {
      threatLevel = 100;
      alarmStatus = 2;
 
      digitalWrite(LED_PIN, HIGH);
      tone(BUZZER_PIN, 2000);
      Serial.println(">>> ALARM! BURGLAR DETECTED!");
    }
  } 
  else if (motion) {
    threatLevel = 30;
    alarmStatus = 1;
    Serial.println(">>> Warning: Motion detected inside (Pet?)");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN)); 
    delay(100);
    noTone(BUZZER_PIN);
  } 
  else {
    if (alarmStatus != 2) {
       threatLevel = 0;
       alarmStatus = 0;
       digitalWrite(LED_PIN, LOW);
       noTone(BUZZER_PIN);
    }
  }
}

void setup_wifi() {
  Serial.print("WiFi...");
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
                 "&field1=" + String(threatLevel) + 
                 "&field2=" + String(alarmStatus);
    http.begin(url);
    http.GET();
    http.end();
  }
}