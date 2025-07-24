// main.ino
#include "StepperControl.h"

void setup() {
  Serial.begin(9600);
  setupSteppers();
}

void loop() {
  if (Serial.available()) {
    char direction = Serial.read(); // F, B, or H
    float distanceMM = Serial.parseFloat();

    if (direction == 'F') {
      moveStepperMM(stepperX, false, distanceMM, 500);
    } else if (direction == 'B') {
      moveStepperMM(stepperX, true, distanceMM, 500);
    } else if (direction == 'H') {
      homeStepper(stepperX);
    } else {
      Serial.println("Invalid input. Use F 20, B 10, or H.");
    }

    while (Serial.available()) Serial.read(); // flush
  }
}
