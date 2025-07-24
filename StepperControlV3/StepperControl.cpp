// StepperControl.cpp
#include "StepperControl.h"
#include <Arduino.h>

const float beltPitch = 2.0;

const int motorSteps = 200;
const int microstepsX = 4;
const int microstepsY = 16;
const int pulleyTeeth = 20;

const float stepsPerMMX = (motorSteps * microstepsX) / (pulleyTeeth * beltPitch);
const float stepsPerMMY = (motorSteps * microstepsY) / (pulleyTeeth * beltPitch);

Stepper stepperX = {"X", 24, 22, 6, stepsPerMMX, 0, 65.0};
Stepper stepperY = {"Y", 5, 6, 7, stepsPerMMY, 0, 65.0};

void setupSteppers() {
  pinMode(stepperX.stepPin, OUTPUT);
  pinMode(stepperX.dirPin, OUTPUT);
  pinMode(stepperX.limitSwitchPin, INPUT_PULLUP);

  pinMode(stepperY.stepPin, OUTPUT);
  pinMode(stepperY.dirPin, OUTPUT);
  pinMode(stepperY.limitSwitchPin, INPUT_PULLUP);
}

void moveStepper(Stepper& motor, bool direction, int steps, int speedMicros, bool ignoreLimits) {
  int newPosition = direction 
    ? motor.currentPositionSteps + steps 
    : motor.currentPositionSteps - steps;

  int maxSteps = motor.maxPositionMM * motor.stepsPerMM;

  if (!ignoreLimits && (newPosition < 0 || newPosition > maxSteps)) {
    Serial.print("Error: ");
    Serial.print(motor.name);
    Serial.println(" move exceeds travel limits!");
    return;
  }

  digitalWrite(motor.dirPin, direction);
  for (int i = 0; i < steps; i++) {
    digitalWrite(motor.stepPin, HIGH);
    delayMicroseconds(speedMicros);
    digitalWrite(motor.stepPin, LOW);
    delayMicroseconds(speedMicros);
  }

  if (!ignoreLimits)
    motor.currentPositionSteps = newPosition;

  Serial.print(motor.name);
  Serial.print(" new position = ");
  Serial.print(motor.currentPositionSteps / motor.stepsPerMM);
  Serial.println(" mm");
}

void moveStepperMM(Stepper& motor, bool direction, float distanceMM, int speedMicros) {
  int stepsToMove = distanceMM * motor.stepsPerMM;
  moveStepper(motor, direction, stepsToMove, speedMicros);
}

void homeStepper(Stepper& motor) {
  Serial.print("Homing ");
  Serial.println(motor.name);

  while (digitalRead(motor.limitSwitchPin) != 0)
    moveStepper(motor, true, 1, 1000, true);

  delay(50);
  moveStepper(motor, false, 50, 500, true);

  Serial.print("Homing complete, ");
  Serial.print(motor.name);
  Serial.println(" = 50 steps");

  motor.currentPositionSteps = 50;
}
