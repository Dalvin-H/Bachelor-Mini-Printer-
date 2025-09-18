#include "StepperController.h"
#include "Config.h"
#include "MotionPlanner.h"
#include "GcodeParser.h"
#include "FileManager.h"
#include "Executor.h"

StepperController stepperX(stepPinX, dirPinX, enablePinX, limitSwitchX, stepsPerMMX, 0);
StepperController stepperY(stepPinY, dirPinY, enablePinY, limitSwitchY, stepsPerMMY, 0);
StepperController stepperZ(stepPinZ, dirPinZ, enablePinZ, limitSwitchZ, stepsPerMMZ, 0);
StepperController stepperE(stepPinE, dirPinE, enablePinE, limitSwitchE, stepsPerMME, 0);

MotionPlanner motionPlanner(stepperX, stepperY, stepperZ, stepperE);
GcodeParser parser(motionPlanner, stepperX, stepperY, stepperZ, stepperE);
FileManager fileManager(chipSelect, &parser, &motionPlanner);

void setup() {
    Serial.begin(9600);
    fileManager.initializeSD();
    delay(1000);
    fileManager.listGcodeFiles();
    fileManager.selectFile();
}

void loop() {}