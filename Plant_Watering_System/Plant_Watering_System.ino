#include <Wire.h>
#include <RTClib.h>
#include <Servo.h>

RTC_DS1307 rtc;
Servo ventil;

// Pins
const int servoPin  = 9;
const int buttonPin = 7;

// Servo-Positionen
const int SERVO_ZU  = 180;
const int SERVO_AUF = 0;

// Zeiten in Sekunden
const uint32_t OPEN_TIME  = 10;
const uint32_t CLOSE_TIME = 7UL * 24UL * 60UL * 60UL;

// Zustände
enum State {
  WAIT_OPEN,
  WAIT_CLOSED
};

State currentState;
uint32_t nextActionTime = 0;

bool loopActive = false;
bool lastButtonState = HIGH;

void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(buttonPin, INPUT_PULLUP);

  ventil.attach(servoPin);
  ventil.write(SERVO_ZU); // Start: geschlossen

  if (!rtc.begin()) {
    Serial.println("RTC nicht gefunden!");
    while (1);
  }

  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("System bereit – Button startet Dauerschleife");
}

void loop() {
  DateTime now = rtc.now();
  uint32_t nowUnix = now.unixtime();

  // ---------- Button ----------
  bool buttonState = digitalRead(buttonPin);

  if (lastButtonState == HIGH && buttonState == LOW) {
    if (!loopActive) {
      Serial.println("Button gedrückt → Dauerschleife START");
      loopActive = true;

      ventil.write(SERVO_AUF);
      currentState = WAIT_OPEN;
      nextActionTime = nowUnix + OPEN_TIME;
    }
  }
  lastButtonState = buttonState;

  // ---------- Dauerschleife ----------
  if (!loopActive) return;

  switch (currentState) {

    case WAIT_OPEN:
      if (nowUnix >= nextActionTime) {
        Serial.println("10s offen → schließe");
        ventil.write(SERVO_ZU);
        currentState = WAIT_CLOSED;
        nextActionTime = nowUnix + CLOSE_TIME;
      }
      break;

    case WAIT_CLOSED:
      if (nowUnix >= nextActionTime) {
        Serial.println("7 Tage zu → öffne erneut");
        ventil.write(SERVO_AUF);
        currentState = WAIT_OPEN;
        nextActionTime = nowUnix + OPEN_TIME;
      }
      break;
  }
}
