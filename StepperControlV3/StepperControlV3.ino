#include "StepperControl.h"

// GT2 belt pitch
const float beltPitch = 2.0;

// X-Stepper configuration
const int stepPinX = 24;
const int dirPinX = 22;
const int limitSwitchX = 6;
const int motorStepsX = 200;
const int microstepsX = 4;
const int pulleyTeethX = 20;

const float stepsPerMMX = (motorStepsX * microstepsX) / (pulleyTeethX * beltPitch);

Stepper stepperX = {"X", stepPinX,dirPinX, limitSwitchX, stepsPerMMX, 0, 65.0};

void setup() {
  Serial.begin(9600);
  pinMode(stepperX.stepPin, OUTPUT);
  pinMode(stepperX.dirPin, OUTPUT);
  pinMode(stepperX.limitSwitchPin, INPUT_PULLUP);

  Serial.println("Ready. Use: F 10, B 5, or H");
}

void loop() {
  if (Serial.available()) {
    char command = Serial.read();
    float value = Serial.parseFloat();

    if (command == 'F') {
      moveStepperMM(stepperX, false, value, 500);  // false = forward
    } 
    else if (command == 'B') {
      moveStepperMM(stepperX, true, value, 500);   // true = backward
    } 
    else if (command == 'H') {
      homeStepper(stepperX);
    } 
    else {
      Serial.println("Invalid input. Use F 10, B 5, or H");
    }

    while (Serial.available()) Serial.read(); // Clear remaining buffer
  }
}
