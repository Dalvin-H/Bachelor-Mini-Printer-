#ifndef GCODEPARSER_H
#define GCODEPARSER_H

#include <Arduino.h>
#include <SD.h>
#include "StepperController.h"

class GcodeParser {
private:
  MotionPlanner& planner;
  StepperController& stepperX;
  StepperController& stepperY;
  StepperController& stepperZ;
  StepperController& stepperE;

public:
  GcodeParser(MotionPlanner& p, StepperController& x, StepperController& y, StepperController& z, StepperController& e)
    : planner(p), stepperX(x), stepperY(y), stepperZ(z), stepperE(e) {}

  void parseGcodeLine(File& source, File& target) {
    float x = NAN;
    float y = NAN;
    float z = NAN;
    float e = NAN;
    float s = NAN;

    String line = source.readStringUntil('\n');
    line.trim();
    int spaceIndex = line.indexOf(' ');
    String CMD = (spaceIndex > 0) ? line.substring(0, spaceIndex) : line;

    if (CMD == "G0" || CMD == "G1") {
      Serial.print(CMD);
      Serial.print(" line detected including: ");

      int index;
      index = line.indexOf('X');
      if (index != -1) {
        x = line.substring(index + 1).toFloat();
        Serial.print("X ");
      }

      index = line.indexOf('Y');
      if (index != -1) {
        y = line.substring(index + 1).toFloat();
        Serial.print("Y ");
      }

      index = line.indexOf('Z');
      if (index != -1) {
        z = line.substring(index + 1).toFloat();
        Serial.print("Z ");
      }
      index = line.indexOf('E');
      if (index != -1) {
        e = line.substring(index + 1).toFloat();
        Serial.print("E ");
      }

      index = line.indexOf('F');
      if (index != -1) {
        s = line.substring(index + 1).toFloat();
        Serial.print("F ");
      }

      Serial.println("");

      translateG(x, y, z, e, s, target);

    } else if (CMD == "G28") { //home all axes
      target.println(CMD);
      Serial.println("G28 line detected");
    } else if (CMD == "M84") { //enable all steppers
      target.println(CMD);
      Serial.println("M84 line detected");
    } else {
      Serial.println("no matching start code... ignoring line");
    }
  }

  void translateG(float parsedX, float parsedY, float parsedZ, float parsedE, float parsedS, File& target)
  //Method for translating the G command line in gcode
  //by parsing the values, calculating steps and writing to a translated file
  {
    int stepsX = 0;
    int stepsY = 0;
    int stepsZ = 0;
    int stepsE = 0;

    bool moved = false;
    char line[64];
    strcpy(line, "M");  // Start line with 'M'

    // Only move axes that were actually provided (not 0 by default)
    if (!isnan(parsedX)) {
      stepsX = planner.calculateSteps(stepperX, stepperX.getCurrentPos(), parsedX);
      stepperX.setCurrentPos(parsedX);
      if (stepsX != 0) {
        char temp[12];
        sprintf(temp, "X%d", stepsX);
        strcat(line, temp);
        moved = true;
      }
    }

    if (!isnan(parsedY)) {
      stepsY = planner.calculateSteps(stepperY, stepperY.getCurrentPos(), parsedY);
      stepperY.setCurrentPos(parsedY);
      if (stepsY != 0) {
        char temp[12];
        sprintf(temp, "Y%d", stepsY);
        strcat(line, temp);
        moved = true;
      }
    }

    if (!isnan(parsedZ)) {
      float compensatedZ = parsedZ + zOffset;
      stepsZ = planner.calculateSteps(stepperZ, stepperZ.getCurrentPos(), compensatedZ);
      stepperZ.setCurrentPos(compensatedZ);
      if (stepsZ != 0) {
        char temp[12];
        sprintf(temp, "Z%d", stepsZ);
        strcat(line, temp);
        moved = true;
      }
    }

    if (!isnan(parsedE)) {
      stepsE = planner.calculateSteps(stepperE, stepperE.getCurrentPos(), parsedE);
      stepperE.setCurrentPos(parsedE);
      if (stepsE != 0) {
        char temp[12];
        sprintf(temp, "E%d", stepsE);
        strcat(line, temp);
        moved = true;
      }
    }

    if (!isnan(parsedS)) {
      speedMicros = (1000000 / ((parsedS / 60) * stepperX.getStepsPerMM()));
      char temp[12];
      sprintf(temp, "S%d", speedMicros);
      strcat(line, temp);
    }

    if (moved) {
      target.println(line);
    }
  }

  void processGCODE(char* sourceFile, char* targetFile) {
    // Step 1: Open the source file
    File source = SD.open(sourceFile, FILE_READ);
    if (!source) {
      Serial.print("Could not open source file: ");
      Serial.println(sourceFile);
      return;
    }

    File target = SD.open(targetFile, FILE_WRITE);
    if (!target) {
      Serial.print("Could not open target file: ");
      Serial.println(targetFile);
      source.close();
      return;
    }

    while (source.available()) {
      parseGcodeLine(source, target);
    }

    Serial.println("Translating done!");

    source.close();
    target.close();
  }
};

#endif