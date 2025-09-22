#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

//SD configuration
const int chipSelect = 53;
bool fileNameIdentical = false;
String baseGCO;  // keep selected base filename

// General configuration:
const float beltPitch = 2.0;
const int pulleyTeeth = 20;
const int microSteps = 4;       //check M0, M1, M2
const int motorRevSteps = 200;  //number of steps for 1 revolution
int speedMicros;

// X-Stepper motor configuration:
const int stepPinX = 28;
const int dirPinX = 26;
const int enablePinX = 5;
const int limitSwitchX = 40;
const float stepsPerMMX = (motorRevSteps * microSteps) / (pulleyTeeth * beltPitch);

// Y-Stepper motor configuration:
const int stepPinY = 24;
const int dirPinY = 22;
const int enablePinY = 8;
const int limitSwitchY = 40;
const float stepsPerMMY = (motorRevSteps * microSteps) / (pulleyTeeth * beltPitch);

// Z-Stepper motor configuration:
const int stepPinZ = 32;
const int dirPinZ = 30;
const int enablePinZ = 9;
const int limitSwitchZ = 40;
const float stepsPerMMZ = (motorRevSteps * microSteps) / (pulleyTeeth * beltPitch);
float zOffset = 0,0;

// E-Stepper motor configuration:
const int stepPinE = 50;
const int dirPinE = 50;
const int enablePinE = 50;
const int limitSwitchE = 50;
const float stepsPerMME = (motorRevSteps * microSteps) / (pulleyTeeth * beltPitch);

#endif