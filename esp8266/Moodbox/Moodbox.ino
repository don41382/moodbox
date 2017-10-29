#include <ESP8266WiFi.h> 
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#define TRIGGER 5
#define ECHO    4

#define BUTTON A0
#define LED_1 14
#define LED_2 12
#define LED_3 13
#define LED_4 15
const int leds[] = {LED_1,LED_2,LED_3,LED_4};

const int MAX_VOTE_WAIT (10 * 1000);
const int COOL_DOWN_WAIT (10 * 1000);

enum state {
  waitForPerson,
  vote,
  coolDown
};
 
void setup() {
  Serial.begin (115200);

  // distance sensor
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);  
  
  // button config
  pinMode(BUTTON, INPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(LED_4, OUTPUT);

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
  } else {
    return -1;
  }
}

int button;

void loop() {
  switch (currentState) {
    case waitForPerson:
      allLights(false);
      if (checkDistance(30)) {
        readyToVoteLights();
        currentState = vote;
        timeout = millis() + MAX_VOTE_WAIT;
      }
      delay(100);
      break;
    case vote:
      if (millis() >= timeout) {
        currentState = waitForPerson;
      }
      button = readButton();
      if (button >= 0) {
        allLights(false);
        achknoledgeButton(button);
        currentState = coolDown;
        timeout = millis() + COOL_DOWN_WAIT;
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
