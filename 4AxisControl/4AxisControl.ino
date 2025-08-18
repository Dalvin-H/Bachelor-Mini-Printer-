#include <SPI.h>
#include <SD.h>

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
String baseGCO;

String readFirstLine(File &file) { // pass by reference
  String line = "";
  file.seek(0); // make sure we start at beginning
  while (file.available()) {
    char c = file.read();
    if (c == '\n') break;
    line += c;
  }
  return line;
}

const float beltPitch = 2.0;
const int pulleyTeeth = 20;
const int microSteps = 4;
const int motorRevSteps = 200;
int speedMicros = 500;

// Stepper pins
Stepper stepperX = { "X", 28, 26, 5, 40, (motorRevSteps*microSteps)/(pulleyTeeth*beltPitch), 0 };
Stepper stepperY = { "Y", 24, 22, 8, 41, (motorRevSteps*microSteps)/(pulleyTeeth*beltPitch), 0 };
Stepper stepperZ = { "Z", 32, 30, 9, 42, (motorRevSteps*microSteps)/(pulleyTeeth*beltPitch), 0 };
Stepper stepperE = { "E", 50, 51, 52, 43, (motorRevSteps*microSteps)/(pulleyTeeth*beltPitch), 0 };

void setup() {
  Serial.begin(9600);
  pinMode(38, INPUT_PULLUP);

  initializeStepper(stepperX);
  initializeStepper(stepperY);
  initializeStepper(stepperZ);
  initializeStepper(stepperE);

  Serial.println("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("SD init failed!");
    while (true);
  }
  Serial.println("SD card ready.");
  delay(1000);

  File root = SD.open("/");
  File entry;

  int index = 1;
  Serial.println("GCODE files on SD card:");
  while ((entry = root.openNextFile())) {
    String nameGCO = entry.name();
    if (nameGCO.endsWith(".gco") || nameGCO.endsWith(".GCO")) {
      Serial.print(index); Serial.print(": "); Serial.println(nameGCO);
      gcodeFiles[fileCount++] = nameGCO;
      index++;
    }
    entry.close();
  }
  Serial.println("END LIST");

  Serial.println("Type the number of the file you want to select:");
  while (Serial.available() == 0) {}
  int selectedIndex = Serial.readStringUntil('\n').toInt() - 1;

  if (selectedIndex >= 0 && selectedIndex < fileCount) {
    baseGCO = gcodeFiles[selectedIndex];
    baseGCO.remove(baseGCO.lastIndexOf('.'));
  } else {
    Serial.println("Invalid selection.");
    while (true);
  }

  fileNameIdentical = false;
  root = SD.open("/");
  while ((entry = root.openNextFile())) {
    String nameTXT = entry.name();
    if (nameTXT.endsWith(".TXT") || nameTXT.endsWith(".txt")) {
      String baseTXT = nameTXT;
      baseTXT.remove(baseTXT.lastIndexOf('.'));
      if (baseTXT == baseGCO) fileNameIdentical = true;
    }
    entry.close();
  }

  String txtFileName = baseGCO + ".TXT";

  if (fileNameIdentical) {
    Serial.println("Identical TXT file found, checking IDs...");
    File source = SD.open(gcodeFiles[selectedIndex]);
    File target = SD.open(txtFileName);
    if (source && target) {
      String sourceID = readFirstLine(source);
      String targetID = readFirstLine(target);
      source.close(); target.close();

      if (sourceID == targetID) {
        Serial.println("IDs match -> Executing TXT file...");
        File reTarget = SD.open(txtFileName);
        excecuteTargetFile(reTarget);
        reTarget.close();
      } else {
        Serial.println("IDs do not match -> Overwriting TXT file...");
        SD.remove(txtFileName);
        File newTarget = SD.open(txtFileName, FILE_WRITE);
        File source2 = SD.open(gcodeFiles[selectedIndex]);
        if (newTarget && source2) {
          newTarget.println(readFirstLine(source2));
          processGCODE(source2, newTarget);
          newTarget.close(); source2.close();
        }
      }
    }
  } else {
    Serial.println("No identical TXT file found -> Creating new file...");
    File newTarget = SD.open(txtFileName, FILE_WRITE);
    File source = SD.open(gcodeFiles[selectedIndex]);
    if (newTarget && source) {
      newTarget.println(readFirstLine(source));
      processGCODE(source, newTarget);
      newTarget.close(); source.close();
    }
  }
}
void loop() {
  if (digitalRead(38) == 0) {
    delay(50);
    File targetFile = SD.open(baseGCO + ".TXT");
    if (targetFile) {
      if (M84Active) {
        ableAllSteppers(0);
        M84Active = false;
      }
      excecuteTargetFile(targetFile);
      targetFile.close();
    } else Serial.println("Failed to open target file for execution.");
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



void processGCODE(File& sourceFile, File& targetFile) {
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
