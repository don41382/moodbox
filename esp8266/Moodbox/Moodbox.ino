#include <EEPROM.h> 
#include <ESP8266WiFi.h> 
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>

#define USE_SERIAL Serial


#define TRIGGER 5
#define ECHO    4

#define IR      16

#define BUTTON A0
#define LED_1 14
#define LED_2 12
#define LED_3 13
#define LED_4 15
const int leds[] = {LED_1,LED_2,LED_3,LED_4};

const int SETUP_BUTTON = 100;

const int MAX_VOTE_WAIT (10 * 1000);
const int COOL_DOWN_WAIT (1 * 1000);

bool shouldSaveConfig = false;

enum state {
  waitForPerson,
  vote,
  coolDown
};

char api_key[40] = "YOUR API KEY";
 
void setup() {
  Serial.begin (115200);

  loadConfiguration();
  
  // distance sensor
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);  

  // IR sensor
  pinMode(IR, INPUT);
  
  // button config
  pinMode(BUTTON, INPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(LED_4, OUTPUT);
}

void saveConfigCallback () {
  Serial.println("Saving!");
  shouldSaveConfig = true;
}

void saveConfiguration(const char value[40]) {
  EEPROM.begin(512);
  byte address = 0;

  for (byte i = 0; i < 40; i++) {
    EEPROM.write(i, value[i]);
  }
  EEPROM.commit();
  EEPROM.end();
}


void loadConfiguration() {
  EEPROM.begin(512);
  byte address = 0;

  for (byte i = 0; i < 40; i++) {
    api_key[i] = EEPROM.read(i);
  }
  EEPROM.end();  
}

void readyToVoteLights() {
  onOff(LED_1);
  onOff(LED_2);
  onOff(LED_3);
  onOff(LED_4);
  onOff(LED_3);
  onOff(LED_2);
  allLights(true);
  delay(200);
  allLights(false);
  delay(200);
  allLights(true);
  delay(200);
  allLights(false);
  delay(500);
  allLights(true);
}

void errorBlink() {
  allLights(true);
  delay(500);
  allLights(false);
  delay(500);
  allLights(true);
  delay(500);
  allLights(false);
  delay(500);
}

void allLights(bool on) {
  for (int i = 0; i < 5; i++) {
    digitalWrite(leds[i], on ? HIGH : LOW);
  }
}

void onOff(int led) {
 digitalWrite(led, HIGH);
 delay(50);
 digitalWrite(led, LOW);
 delay(50);
}

void achknoledgeButton(int button) {
 for (int i = 0; i < 5; i++) {
   digitalWrite(leds[button], HIGH);
   delay(100);
   digitalWrite(leds[button], LOW);
   delay(100);  
 }
}

bool checkDistance(int minDistance) {

  long duration, distance;
  digitalWrite(TRIGGER, LOW);  
  delayMicroseconds(2); 
  
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10); 
  
  digitalWrite(TRIGGER, LOW);
  duration = pulseIn(ECHO, HIGH);
  distance = (duration/2) / 29.1;

  return distance < minDistance;
}

bool checkMotion() {
  return digitalRead(IR) == HIGH;
}

state currentState = waitForPerson;
long timeout = 0;

bool isNear(int v, int level) {
  return (v > level - 10 && v < level + 10);
}

int readButton() {
  int value = analogRead(BUTTON);
  if (isNear(value, 677)) {
    return 3;
  } else if (isNear(value, 818)) {
    return 2;
  } else if (isNear(value, 909)) {
    return 1;
  } else if (isNear(value, 958)) {
    return 0;
  } else if (isNear(value, 928)) {
    return 100;
  } else {
    return -2;
  }
}

bool postVote(int button) {
  HTTPClient http;
  http.begin("https://io.adafruit.com/api/v2/felix41382/feeds/votes/data","AD4B64B36740B5FC0E519BBD25E97F88B62AA35B");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-AIO-Key", api_key);
  int httpCode = http.POST("{ \"value\": \" " + String(button) + " \"} ");
  http.writeToStream(&Serial);
  http.end();
  return httpCode == 200;
}

bool postEvent(String type) {
  HTTPClient http;
  http.begin("https://io.adafruit.com/api/v2/felix41382/feeds/events/data","AD4B64B36740B5FC0E519BBD25E97F88B62AA35B");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-AIO-Key", api_key);
  int httpCode = http.POST("{ \"value\": \" " + type + " \"} ");
  http.end();
  return httpCode == 200;
}

int button;

void loop() {
  switch (currentState) {
    case waitForPerson:
      allLights(false);
      if (checkMotion()) {
        readyToVoteLights();
        currentState = vote;
        timeout = millis() + MAX_VOTE_WAIT;
      }
      delay(100);
      break;
      
    case vote:
      if (millis() >= timeout) {
        currentState = waitForPerson;
//        postEvent("passing");
      }
      button = readButton();
      if (button >= 0 && button <= 4) {
        allLights(false);
        achknoledgeButton(button);
        if (!postVote(button)) {
          errorBlink();
        }
        if (!postEvent("voted")) {
          errorBlink();
        }
        currentState = coolDown;
        timeout = millis() + COOL_DOWN_WAIT;
      } else if (button == SETUP_BUTTON) {
        resetBox();
      }
      delay(100);
      break;
      
    case coolDown:
      if (millis() >= timeout) {
        currentState = waitForPerson;
      }
      break;
  }
}

void resetBox() {
  WiFiManager wifiManager;
  WiFiManagerParameter api_key_param("API_KEY", "api_key", "", 40);
  wifiManager.addParameter(&api_key_param); 
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  
  if (!wifiManager.startConfigPortal("Moodbox Configuration")) {
    delay(3000);
    ESP.reset();
  }

  if (shouldSaveConfig) {
    saveConfiguration(api_key_param.getValue());
  }

}

