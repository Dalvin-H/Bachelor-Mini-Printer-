#ifndef MOTIONPLANNER_H
#define MOTIONPLANNER_H

#include <Arduino.h>
#include "StepperController.h"

struct Axis {
  StepperController& motor;
  int steps;
  int err;
};

class MotionPlanner {
private:
  StepperController& stepperX;
  StepperController& stepperY;
  StepperController& stepperZ;
  StepperController& stepperE;

public:
  MotionPlanner(StepperController& x, StepperController& y, StepperController& z, StepperController& e)
    : stepperX(x), stepperY(y), stepperZ(z), stepperE(e) {}

  inline void moveXYZE(int stepsX, int stepsY, int stepsZ, int stepsE, int baseSpeedMicros) {
    Axis axes[] = {
      { stepperX, abs(stepsX), 0 },
      { stepperY, abs(stepsY), 0 },
      { stepperZ, abs(stepsZ), 0 },
      { stepperE, abs(stepsE), 0 }
    };

    int axisSteps[] = { stepsX, stepsY, stepsZ, stepsE };
    for (int i = 0; i < 4; i++) {
      digitalWrite(axes[i].motor.getDirPin(), axisSteps[i] > 0);
    }

    int maxSteps = 0;
    for (int i = 0; i < 4; i++) {
      if (axes[i].steps > maxSteps) maxSteps = axes[i].steps;
      axes[i].err = maxSteps / 2;
    }

    // Acceleration profile
    int accelSteps = max(5, maxSteps / 10);      // accelerate and decelerate over 25% each
    int minSpeedMicros = baseSpeedMicros;        // fastest (smallest delay)
    int maxSpeedMicros = baseSpeedMicros * 1.5;  // slowest (start/end)

    for (int i = 0; i < maxSteps; i++) {
      // Compute current delay based on phase of motion
      int currentSpeedMicros = minSpeedMicros;

      if (i < accelSteps) {
        // Accelerating
        float t = (float)i / accelSteps;
        currentSpeedMicros = maxSpeedMicros - (maxSpeedMicros - minSpeedMicros) * t;
      } else if (i > maxSteps - accelSteps) {
        // Decelerating
        float t = (float)(maxSteps - i) / accelSteps;
        currentSpeedMicros = maxSpeedMicros - (maxSpeedMicros - minSpeedMicros) * t;
      }

      // Step all axes using Bresenham
      for (int j = 0; j < 4; j++) {
        axes[j].err -= axes[j].steps;
        if (axes[j].err < 0) {
          axes[j].motor.pulseStepper(currentSpeedMicros);
          axes[j].err += maxSteps;
        }
      }
    }
  }

  inline int calculateSteps(StepperController& motor, float currentPos, float targetPos) {
    float disMM = abs(currentPos - targetPos);
    int steps = disMM * motor.getStepsPerMM();
    if (targetPos < currentPos) steps = -steps;
    return steps;
  }

  void homeAllAxes() {
    stepperX.home();
    stepperY.home();
    stepperZ.home();
  }

  void enableAllAxes() {
    stepperX.enable();
    stepperY.enable();
    stepperZ.enable();
    stepperE.enable();
  }
};

#endif