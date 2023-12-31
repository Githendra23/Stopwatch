#include <Arduino.h>

#define UNITS_BCD_FIRST_PIN  3
#define TENS_BCD_FIRST_PIN   8
#define UNITS_BLANK_PIN      7
#define TENS_BLANK_PIN       12

#define FIRST_INPUT_PIN UNITS_BCD_FIRST_PIN
#define LAST_INPUT_PIN  TENS_BLANK_PIN

#define START_STOP_BUTTON_PIN 2
#define SET_RESET_BUTTON_PIN  13

#define risingEdge(signal) (signal.levels == 0b01) // erreur corrigée

#define isHigh(signal) (signal.levels & 0b01 == 1) 

#define updateSignal(signal) {\
  signal.levels <<= 1;\
  signal.levels = ((digitalRead(signal.pin) | signal.levels) & 0b11);\
}

#define unitDigit(number) (number % 10)

#define tensDigit(number) ((number / 10) % 10)

#define modulo100increment(number) ((number + 1) % 100)

enum StartStopState {STOP, START};
bool startStopState = STOP;

enum Mode {SET, WORK};
bool mode = WORK;

typedef struct {
  uint8_t pin;  // pin number to be declared as INPUT or INPUT_PULLUP
  byte levels;  // current level on bit 0, previous level on bit 1
} LogicalSignal;

bool longPushMaintained(LogicalSignal signal);
bool longPushOccured(LogicalSignal signal); // prototype ajouté

LogicalSignal startStopSignal = {START_STOP_BUTTON_PIN, 0b00}; // positive logic
LogicalSignal setResetSignal  = {SET_RESET_BUTTON_PIN,  0b00}; // positive logic

// -----------------------------------------------------------------------------------------

void setup() {
  initPins();
  steadyDisplay();
  Serial.begin(9600);
}

void loop() {
  static uint8_t num = 20;

  updateSignal(startStopSignal);
  updateSignal(setResetSignal);

  if (mode == WORK) {
    steadyDisplay();

    if (risingEdge(startStopSignal)) {
      startStopState = !startStopState;
    }

    if (startStopState == START) {
      if (num != 0) {
        if (clockSecondTick() == 1) {
          num--;
        }
      }
      else { // num == 0
        startStopState = STOP;
      }
    }
    else { // startStopState == STOP
      if (longPushOccured(setResetSignal) == 1) {
        mode = SET;
      }
      else if (risingEdge(setResetSignal)) {
        num = 20;
      }
    }
  }
  else { // mode == SET
    blinkDisplay();

    if (longPushOccured(setResetSignal) == 1) {
      mode = WORK;
      startStopState = STOP;
    }

    if (longPushOccured(startStopSignal) == 1) {
      steadyDisplay();
      
      if (clockTenthTick() == 1) {
        num = modulo100increment(num);
      }
    }
    else if (risingEdge(startStopSignal)) {
      num = modulo100increment(num);
    }
  }
  displayNumber(num);
  Serial.println(longPushOccured(setResetSignal));
  delay(10);
}

// -----------------------------------------------------------------------------------------

void initPins() {
  for (int pin = FIRST_INPUT_PIN; pin <= LAST_INPUT_PIN; pin++) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  pinMode(START_STOP_BUTTON_PIN, INPUT);
  pinMode(SET_RESET_BUTTON_PIN, INPUT);
}

void steadyDisplay() {
  digitalWrite(UNITS_BLANK_PIN, HIGH);
  digitalWrite(TENS_BLANK_PIN, HIGH);
}

void displayDigit(uint8_t digit, uint8_t inputBDCFirstPin) { 
  for (uint8_t pin = inputBDCFirstPin; pin <= inputBDCFirstPin + 3; pin++) {
    digitalWrite(pin, bitRead(digit, pin - inputBDCFirstPin));
  }
}

void displayNumber(uint8_t positiveNumber) {
  displayDigit(tensDigit(positiveNumber), TENS_BCD_FIRST_PIN);
  displayDigit(unitDigit(positiveNumber), UNITS_BCD_FIRST_PIN);
}

bool clockSecondTick() {
  static uint32_t previousSecond = millis();

  if (millis() - previousSecond >= 1000) {
    previousSecond = millis();
    return 1;
  }
  return 0;
}

bool clockTenthTick() {
  static uint32_t previousTenth = millis();

  if (millis() - previousTenth >= 100) {
    previousTenth = millis();
    return 1;
  }
  else return 0;
}

void blinkDisplay() {
  static uint32_t millisBlink = 0;

  if (millis() - millisBlink >= 500) {
    millisBlink = millis();
    digitalWrite(UNITS_BLANK_PIN, !digitalRead(UNITS_BLANK_PIN));
    digitalWrite(TENS_BLANK_PIN, !digitalRead(TENS_BLANK_PIN));
  }
}

bool longPushOccured(LogicalSignal signal) {
  static uint32_t previousMillis = 0;
  static bool hasRisingEdgeOccured = false;

  if (risingEdge(signal)) {
    previousMillis = millis();
    hasRisingEdgeOccured = true;
  }

  if (hasRisingEdgeOccured == true) {
    if (isHigh(signal)) {
      if (millis() - previousMillis >= 1000) {
        hasRisingEdgeOccured = false;
        return 1;
      } 
    } 
    else { // isHigh(signal) == 0
      hasRisingEdgeOccured = false;
      return 0;
    }
  }
  else { // hasRisingEdgeOccured == false
    return 0;
  }
}

bool longPushMaintained(LogicalSignal signal) {
  static uint32_t millisWhenPushed = 0;
  if (risingEdge(signal)) {
    millisWhenPushed = millis();
  }

  if (isHigh(signal)) {
    if (millis() - millisWhenPushed >= 1000) {
      return 1;
    }
  } 
  return 0;
}