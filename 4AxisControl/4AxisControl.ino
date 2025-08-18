#include <SPI.h>
#include <SD.h>

// Struct setup for Stepper objects:
struct Stepper {
  const char* name;
  int stepPin;
  int dirPin;
  int enablePin;
  int limitPin;
  float stepsPerMM;
  float currentPos;
};

struct Axis {
  Stepper& motor;
  int steps;
  int err;
};

bool M84Active = false;

const int chipSelect = 53;

String gcodeFiles[20];
int fileCount = 0;
bool fileNameIdentical = false;
String baseGCO;  // keep selected base filename

// Function to read the first line of a file
String readFirstLine(File file) {
  String line = "";
  while (file.available()) {
    char c = file.read();
    if (c == '\n') break;  // stop at first newline
    line += c;
  }
  return line;
}

// General configuration:
const float beltPitch = 2.0;
const int pulleyTeeth = 20;
const int microSteps = 4;       //check M0, M1, M2
const int motorRevSteps = 200;  //number of steps for 1 revolution
int speedMicros = 500;

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
const int limitSwitchY = 41;  // FIX: unique pin
const float stepsPerMMY = (motorRevSteps * microSteps) / (pulleyTeeth * beltPitch);

// Z-Stepper motor configuration:
const int stepPinZ = 32;
const int dirPinZ = 30;
const int enablePinZ = 9;
const int limitSwitchZ = 42;  // FIX: unique pin
const float stepsPerMMZ = (motorRevSteps * microSteps) / (pulleyTeeth * beltPitch);

// E-Stepper motor configuration:
const int stepPinE = 50;
const int dirPinE = 51;
const int enablePinE = 52;
const int limitSwitchE = 43;  // FIX: unique pin
const float stepsPerMME = (motorRevSteps * microSteps) / (pulleyTeeth * beltPitch);

// Creating stepper objects using struct:
Stepper stepperX = { "X", stepPinX, dirPinX, enablePinX, limitSwitchX, stepsPerMMX, 0 };
Stepper stepperY = { "Y", stepPinY, dirPinY, enablePinY, limitSwitchY, stepsPerMMY, 0 };
Stepper stepperZ = { "Z", stepPinZ, dirPinZ, enablePinZ, limitSwitchZ, stepsPerMMZ, 0 };
Stepper stepperE = { "E", stepPinE, dirPinE, enablePinE, limitSwitchE, stepsPerMME, 0 };

void setup() {
  Serial.begin(9600);
  pinMode(38, INPUT_PULLUP);

  // assign pinModes:
  initializeStepper(stepperX);
  initializeStepper(stepperY);
  initializeStepper(stepperZ);
  initializeStepper(stepperE);

  Serial.println("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("SD init failed!");
    return;
  }
  Serial.println("SD card ready.");

  delay(2000);

  File root = SD.open("/");
  File entry;

  int index = 1;
  Serial.println("GCODE files on SD card:");

  while (true) {
    entry = root.openNextFile();
    if (!entry) break;

    String nameGCO = entry.name();
    if (nameGCO.endsWith(".gco") || nameGCO.endsWith(".GCO")) {
      Serial.print(index);
      Serial.print(": ");
      Serial.println(nameGCO);
      gcodeFiles[fileCount] = nameGCO;
      fileCount++;
      index++;
    }

    entry.close();
  }
  Serial.println("END LIST");

  Serial.println("Type the number of the file you want to select:");

  while (Serial.available() == 0) {
    // just wait for user input
  }

  String input = Serial.readStringUntil('\n');
  int selectedIndex = input.toInt() - 1;

  if (selectedIndex >= 0 && selectedIndex < fileCount) {
    Serial.print("You selected file: ");
    Serial.println(gcodeFiles[selectedIndex]);
    baseGCO = gcodeFiles[selectedIndex];
    baseGCO.remove(baseGCO.lastIndexOf('.'));  // strip extension
  } else {
    Serial.println("Invalid selection.");
    return;
  }

  // Check for matching TXT file
  Serial.println("Searching for identical TXT file...");
  root = SD.open("/");
  while (true) {
    entry = root.openNextFile();
    if (!entry) break;

    String nameTXT = entry.name();
    if (nameTXT.endsWith(".TXT") || nameTXT.endsWith(".txt")) {
      String baseTXT = nameTXT;
      baseTXT.remove(baseTXT.lastIndexOf('.'));
      if (baseTXT == baseGCO) {
        fileNameIdentical = true;
      }
    }

    entry.close();
  }

  String txtFileName = baseGCO + ".TXT";

  if (fileNameIdentical) {
    Serial.println("Identical TXT file found, checking IDs...");

    // Open GCO source and TXT target
    File source = SD.open(gcodeFiles[selectedIndex]);
    File target = SD.open(txtFileName);

    if (source && target) {
      String sourceID = readFirstLine(source);
      String targetID = readFirstLine(target);
      source.close();
      target.close();

      Serial.print("Source ID: ");
      Serial.println(sourceID);
      Serial.print("Target ID: ");
      Serial.println(targetID);

      if (sourceID == targetID) {
        Serial.println("IDs match -> Executing TXT file...");
        File reTarget = SD.open(txtFileName);
        excecuteTargetFile(reTarget);
      } else {
        Serial.println("IDs do not match -> Overwriting TXT file...");
        SD.remove(txtFileName);  // delete old file
        File newTarget = SD.open(txtFileName, FILE_WRITE);
        File source2 = SD.open(gcodeFiles[selectedIndex]);
        if (newTarget && source2) {
          target.println(readFirstLine(source));
          processGCODE(source2, newTarget);
          newTarget.close();
          source2.close();
        }
      }
    } else {
      Serial.println("Error opening files for comparison.");
    }

  } else {
    Serial.println("No identical TXT file found -> Creating new file...");
    File newTarget = SD.open(txtFileName, FILE_WRITE);
    File source = SD.open(gcodeFiles[selectedIndex]);
    if (newTarget && source) {
      target.println(readFirstLine(source));
      processGCODE(source, newTarget);
      newTarget.close();
      source.close();
    }
  }
}

void loop() {
  // your runtime logic can go here
}

void pulseStepper(Stepper& motor, int speedMicros) {
  digitalWrite(motor.stepPin, HIGH);
  delayMicroseconds(speedMicros);
  digitalWrite(motor.stepPin, LOW);
  delayMicroseconds(speedMicros);
}

void initializeStepper(Stepper& motor) {
  pinMode(motor.stepPin, OUTPUT);
  pinMode(motor.dirPin, OUTPUT);
  pinMode(motor.enablePin, OUTPUT);
  pinMode(motor.limitPin, INPUT_PULLUP);
  digitalWrite(motor.enablePin, LOW);  // enable motor
}

int calculateSteps(float currentPos, float targetPos, Stepper& motor) {
  float disMM = abs(currentPos - targetPos);
  int steps = disMM * motor.stepsPerMM;
  if (targetPos < currentPos) steps = -steps;
  return steps;
}

void translateG(float parsedX, float parsedY, float parsedZ, float parsedE, File& target) {
  int stepsX = 0;
  int stepsY = 0;
  int stepsZ = 0;
  int stepsE = 0;

  bool moved = false;
  char line[64];
  strcpy(line, "M");  // Start line with 'M'

  if (!isnan(parsedX)) {
    stepsX = calculateSteps(stepperX.currentPos, parsedX, stepperX);
    stepperX.currentPos = parsedX;
    if (stepsX != 0) {
      char temp[12];
      sprintf(temp, "X%d", stepsX);
      strcat(line, temp);
      moved = true;
    }
  }

  if (!isnan(parsedY)) {
    stepsY = calculateSteps(stepperY.currentPos, parsedY, stepperY);
    stepperY.currentPos = parsedY;
    if (stepsY != 0) {
      char temp[12];
      sprintf(temp, "Y%d", stepsY);
      strcat(line, temp);
      moved = true;
    }
  }

  if (!isnan(parsedZ)) {
    stepsZ = calculateSteps(stepperZ.currentPos, parsedZ, stepperZ);
    stepperZ.currentPos = parsedZ;
    if (stepsZ != 0) {
      char temp[12];
      sprintf(temp, "Z%d", stepsZ);
      strcat
