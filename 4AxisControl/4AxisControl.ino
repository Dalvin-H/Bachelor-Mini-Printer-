#include <SPI.h>
#include <SD.h>

bool M84Active = false;

const int chipSelect = 53;
const char* sourceFile = "file.GCO";  // File to read
const char* targetFile = "copy.txt";  // File to create and write


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
const int limitSwitchY = 40;
const float stepsPerMMY = (motorRevSteps * microSteps) / (pulleyTeeth * beltPitch);

// Z-Stepper motor configuration:
const int stepPinZ = 32;
const int dirPinZ = 30;
const int enablePinZ = 9;
const int limitSwitchZ = 40;
const float stepsPerMMZ = (motorRevSteps * microSteps) / (pulleyTeeth * beltPitch);

// E-Stepper motor configuration:
const int stepPinE = 50;
const int dirPinE = 50;
const int enablePinE = 50;
const int limitSwitchE = 50;
const float stepsPerMME = (motorRevSteps * microSteps) / (pulleyTeeth * beltPitch);

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

// Creating stepper objects using struct:
// {Name, stepPin, dirPin, enablePin, limitPin, stepsPerMM, currentPosition}
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

  processGCODE(sourceFile, targetFile);
}

void loop() {
  if (digitalRead(38) == 0) {
    delay(50);
    File target = SD.open(targetFile, FILE_READ);
    if (target) {
      if (M84Active)
      {
        ableAllSteppers(0);
        M84Active = false;
      }
      excecuteTargetFile(target);
    } else {
      Serial.println("Failed to open target file for execution.");
    }
  }
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

void moveXYZE(int stepsX, int stepsY, int stepsZ, int stepsE, int baseSpeedMicros) {
  Axis axes[] = {
    { stepperX, abs(stepsX), 0 },
    { stepperY, abs(stepsY), 0 },
    { stepperZ, abs(stepsZ), 0 },
    { stepperE, abs(stepsE), 0 }
  };

  int axisSteps[] = { stepsX, stepsY, stepsZ, stepsE };
  for (int i = 0; i < 4; i++) {
    digitalWrite(axes[i].motor.dirPin, axisSteps[i] > 0);
  }

  int maxSteps = 0;
  for (int i = 0; i < 4; i++) {
    if (axes[i].steps > maxSteps) maxSteps = axes[i].steps;
    axes[i].err = maxSteps / 2;
  }

  // Acceleration profile
  int accelSteps = max(5, maxSteps / 10); // accelerate and decelerate over 25% each
  int minSpeedMicros = baseSpeedMicros; // fastest (smallest delay)
  int maxSpeedMicros = baseSpeedMicros * 1.5; // slowest (start/end)

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
        pulseStepper(axes[j].motor, currentSpeedMicros);
        axes[j].err += maxSteps;
      }
    }
  }
}


/*void moveXYZE(int stepsX, int stepsY, int stepsZ, int stepsE, int speedMicros) {
  Axis axes[] = {
    { stepperX, abs(stepsX), 0 },
    { stepperY, abs(stepsY), 0 },
    { stepperZ, abs(stepsZ), 0 },
    { stepperE, abs(stepsE), 0 }
  };

  // set directional pins
  int axisSteps[] = { stepsX, stepsY, stepsZ, stepsE };
  for (int i = 0; i < 4; i++) {
    digitalWrite(axes[i].motor.dirPin, axisSteps[i] > 0);
  }

  // Find max steps
  int maxSteps = 0;
  for (int i = 0; i < 4; i++) {
    if (axes[i].steps > maxSteps) maxSteps = axes[i].steps;
    axes[i].err = maxSteps / 2;
  }

  // Pulse loop
  for (int i = 0; i < maxSteps; i++) {
    for (int j = 0; j < 4; j++) {
      axes[j].err -= axes[j].steps;
      if (axes[j].err < 0) {
        pulseStepper(axes[j].motor, speedMicros);
        axes[j].err += maxSteps;
      }
    }
  }
} */

int calculateSteps(float currentPos, float targetPos, Stepper& motor) {
  float disMM = abs(currentPos - targetPos);
  int steps = disMM * motor.stepsPerMM;
  if (targetPos < currentPos) steps = -steps;
  return steps;
}
void translateG(float parsedX, float parsedY, float parsedZ, float parsedE, File& target)
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
      strcat(line, temp);
      moved = true;
    }
  }

  if (!isnan(parsedE)) {
    stepsE = calculateSteps(stepperE.currentPos, parsedE, stepperE);
    stepperE.currentPos = parsedE;
    if (stepsE != 0) {
      char temp[12];
      sprintf(temp, "E%d", stepsE);
      strcat(line, temp);
      moved = true;
    }
  }

  if (moved) {
    target.println(line);
  }
}

void parseGcodeLine(File& source, File& target) {
  float x = NAN;
  float y = NAN;
  float z = NAN;
  float e = NAN;

  String line = source.readStringUntil('\n');
  line.trim();
  int spaceIndex = line.indexOf(' ');
  String CMD = (spaceIndex > 0) ? line.substring(0, spaceIndex) : line;

  if (CMD == "G0" || CMD == "G1") {
    Serial.print(CMD);
    Serial.println(" line detected");

    int index;
    index = line.indexOf('X');
    if (index != -1) x = line.substring(index + 1).toFloat();

    index = line.indexOf('Y');
    if (index != -1) y = line.substring(index + 1).toFloat();

    index = line.indexOf('Z');
    if (index != -1) z = line.substring(index + 1).toFloat();

    index = line.indexOf('E');
    if (index != -1) e = line.substring(index + 1).toFloat();

    translateG(x, y, z, e, target);

  } else if (CMD == "G28") {
    target.println(CMD);
    Serial.println("G28 line detected");
  } else if (CMD == "M84") {
    target.println(CMD);
    Serial.println("M84 line detected");
  } else {
    Serial.println("no matching start code... ignoring line");
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

  // Step 2: Create or overwrite the target file
  if (SD.exists(targetFile)) {
    SD.remove(targetFile);  // Delete the existing file
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

void homeAxes(Stepper& motor) {
  Serial.print("Homing started for ");
  Serial.println(motor.name);
  digitalWrite(motor.dirPin, LOW);
  while (digitalRead(motor.limitPin) != 0) {
    pulseStepper(motor, 400);
  }
  delay(200);
  digitalWrite(motor.dirPin, HIGH);
  for (int i = 0; i < 10; i++) {
    pulseStepper(motor, 400);
  }
  delay(500);
  motor.currentPos = 0;
}

void excecuteTargetFile(File& target) {
  while (target.available()) {
    String line = target.readStringUntil('\n');
    line.trim();

    if (line.length() == 0) continue;
    Serial.print("Excecuteing line: ");
    Serial.println(line);

    if (line.startsWith("M84")) {
      ableAllSteppers(1);
      M84Active = true;

    } else if (line.startsWith("M")) {
      int stepsX = 0, stepsY = 0, stepsZ = 0, stepsE = 0;

      int idx = line.indexOf('X');
      if (idx != -1)
        stepsX = line.substring(idx + 1, line.indexOf('y', idx)).toInt();

      idx = line.indexOf('Y');
      if (idx != -1)
        stepsY = line.substring(idx + 1, line.indexOf('z', idx)).toInt();

      idx = line.indexOf('Z');
      if (idx != -1)
        stepsZ = line.substring(idx + 1, line.indexOf('e', idx)).toInt();

      idx = line.indexOf('E');
      if (idx != -1)
        stepsE = line.substring(idx + 1).toInt();

      idx = line.indexOf('S');
      if (idx != -1)
        speedMicros = line.substring(idx + 1).toInt();
      
      moveXYZE(stepsX, stepsY, stepsZ, stepsE, speedMicros);
    }

    else if (line.startsWith("G28")) {
      Serial.println("Homing all axes...");
      homeAllAxes();
    }

    else {
      Serial.print("Unknown command in target: ");
      Serial.println(line);
    }
  }

  Serial.println("Print finnished!");
  target.close();
}

void homeAllAxes() {
  homeAxes(stepperX);
  homeAxes(stepperY);
  //homeAxes(stepperZ);
}

void ableAllSteppers(int state) {
  digitalWrite(stepperX.enablePin, state);
  digitalWrite(stepperY.enablePin, state);
  digitalWrite(stepperZ.enablePin, state);
  digitalWrite(stepperE.enablePin, state);
}
