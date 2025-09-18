#ifndef STEPPERCONTROLLER_H
#define STEPPERCONTROLLER_H

#include <Arduino.h>
#include "Config.h"

class StepperController {
private:
  const char* name;
  int stepPin;
  int dirPin;
  int enablePin;
  int limitPin;
  float stepsPerMM;
  float currentPos;
  bool enabled;

public:
  StepperController(int step, int dir, int enable, int limit, float stepsMM, float current)
    : stepPin(step), dirPin(dir), enablePin(enable), limitPin(limit), stepsPerMM(stepsMM), currentPos(current), enabled(false) {
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    pinMode(enablePin, OUTPUT);
    pinMode(limitPin, OUTPUT);
    digitalWrite(enablePin, LOW);
  }

  void enable() {
    digitalWrite(enablePin, LOW);
    enabled = true;
  }

  void disable() {
    digitalWrite(enablePin, HIGH);
    enabled = false;
  }

  void pulseStepper(int speedMicros) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(speedMicros - 10);
  }

  void moveSingleMM(float targetPos, int speedMicros) {
    float distance = targetPos - currentPos;
    if (distance < 0) {
      digitalWrite(dirPin, HIGH);
    } else {
      digitalWrite(dirPin, LOW);
    }
    distance = abs(distance);
    int steps = round(distance * stepsPerMM);

    for (int i = 0; i <= steps; i++) {
      pulseStepper(speedMicros);
    }

    currentPos = targetPos;
  }

  int getDirPin() const {
    return dirPin;
  }

  int getStepsPerMM() const {
    return stepsPerMM;
  }

  float getCurrentPos() const {
    return currentPos;
  }

  void setCurrentPos(float input) {
    currentPos = input;
  }

  void home() {
    Serial.print("Homing started for ");
    Serial.println(name);
    digitalWrite(dirPin, LOW);
    while (digitalRead(limitPin) != 0) {
      pulseStepper(400);
    }
    delay(200);
    digitalWrite(dirPin, HIGH);
    for (int i = 0; i < 10; i++) {
      pulseStepper(400);
    }
    delay(500);
    currentPos = 0;
  }
};

#endif