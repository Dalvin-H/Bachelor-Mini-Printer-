#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <Arduino.h>
#include "StepperController.h"

class Executor {
private:
  MotionPlanner& motionPlanner;
  bool M84Active;

public:
  Executor(MotionPlanner& mp) : motionPlanner(mp), M84Active(false) {}
  
  void excecuteTargetFile(File& target) {
    while (target.available()) {
      String line = target.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) continue;

      Serial.print("Executing line: ");
      Serial.println(line);

      if (line.startsWith("M84")) {
        motionPlanner.enableAllAxes();
        M84Active = true;
      } else if (line.startsWith("M")) {
        int stepsX = 0, stepsY = 0, stepsZ = 0, stepsE = 0;

        int idx = line.indexOf('X');
        if (idx != -1)
          stepsX = line.substring(idx + 1, line.indexOf('Y', idx)).toInt();

        idx = line.indexOf('Y');
        if (idx != -1)
          stepsY = line.substring(idx + 1, line.indexOf('Z', idx)).toInt();

        idx = line.indexOf('Z');
        if (idx != -1)
          stepsZ = line.substring(idx + 1, line.indexOf('E', idx)).toInt();

        idx = line.indexOf('E');
        if (idx != -1)
          stepsE = line.substring(idx + 1).toInt();

        idx = line.indexOf('S');
        if (idx != -1)
          speedMicros = line.substring(idx + 1).toInt();

        motionPlanner.moveXYZE(stepsX, stepsY, stepsZ, stepsE, speedMicros);
      } else if (line.startsWith("G28")) {
        Serial.println("Homing all axes...");
        motionPlanner.homeAllAxes();
      } else {
        Serial.print("Unknown command in target: ");
        Serial.println(line);
      }
    }

    Serial.println("Print finished!");
    target.close();
  }
};

#endif