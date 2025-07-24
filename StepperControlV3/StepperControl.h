#ifndef STEPPER_CONTROL_H
#define STEPPER_CONTROL_H

struct Stepper {
  const char* name;
  int stepPin;
  int dirPin;
  int limitSwitchPin;
  float stepsPerMM;
  int currentPositionSteps;
  float maxPositionMM;
};

void moveStepper(Stepper& motor, bool directionBackward, int steps, int speedMicros, bool ignoreLimits = false);
void moveStepperMM(Stepper& motor, bool directionBackward, float distanceMM, int speedMicros);
void homeStepper(Stepper& motor);

#endif
