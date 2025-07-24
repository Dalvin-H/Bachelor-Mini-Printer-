// StepperControl.h
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

extern Stepper stepperX;
extern Stepper stepperY;

void setupSteppers();
void homeStepper(Stepper& motor);
void moveStepper(Stepper& motor, bool direction, int steps, int speedMicros, bool ignoreLimits = false);
void moveStepperMM(Stepper& motor, bool direction, float distanceMM, int speedMicros);

#endif
