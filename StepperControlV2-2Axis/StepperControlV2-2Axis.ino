const float beltPitch = 2.0;  // GT2 belt

// X-Stepper configuration
const int stepPinX = 24;
const int dirPinX = 22;
const int motorStepsX = 400;
const int microstepsX = 4;
const int pulleyTeethX = 20;

// Y-Stepper configuration
const int stepPinY = 28;
const int dirPinY = 26;
const int motorStepsY = 200;
const int microstepsY = 4;
const int pulleyTeethY = 20;

const float stepsPerMMX = (motorStepsX * microstepsX) / (pulleyTeethX * beltPitch);
const float stepsPerMMY = (motorStepsY * microstepsY) / (pulleyTeethY * beltPitch);

struct Stepper {
  const char* name;
  int stepPin;
  int dirPin;
  float stepsPerMM;
  int currentPositionSteps;
};

Stepper stepperX = {"X", stepPinX, dirPinX, stepsPerMMX, 0};
Stepper stepperY = {"Y", stepPinY, dirPinY, stepsPerMMY, 0};

void setup() {
  Serial.begin(9600);
  pinMode(stepperX.stepPin, OUTPUT);
  pinMode(stepperX.dirPin, OUTPUT);
  pinMode(stepperY.stepPin, OUTPUT);
  pinMode(stepperY.dirPin, OUTPUT);

  Serial.println("Enter command in format: X10 Y20");
}

void loop() {
  static String input = "";

  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      input.trim();
      if (input.length() > 0) {
        float xVal = 0, yVal = 0;
        bool xFound = false, yFound = false;

        int xIndex = input.indexOf('X');
        if (xIndex != -1) {
          int endIdx = input.indexOf(' ', xIndex + 1);
          xVal = input.substring(xIndex + 1, endIdx == -1 ? input.length() : endIdx).toFloat();
          xFound = true;
        }

        int yIndex = input.indexOf('Y');
        if (yIndex != -1) {
          int endIdx = input.indexOf(' ', yIndex + 1);
          yVal = input.substring(yIndex + 1, endIdx == -1 ? input.length() : endIdx).toFloat();
          yFound = true;
        }

        if (xFound || yFound)
          moveXY(xFound ? xVal : 0, yFound ? yVal : 0, 500);
        else
          Serial.println("Invalid input. Use X10 Y20");

        input = "";
      }
    } else {
      input += c;
    }
  }
}


void moveXY(float xMM, float yMM, int speedMicros) {
  int xSteps = xMM * stepperX.stepsPerMM;
  int ySteps = yMM * stepperY.stepsPerMM;

  bool xDir = xSteps < 0;
  bool yDir = ySteps < 0;

  xSteps = abs(xSteps);
  ySteps = abs(ySteps);

  digitalWrite(stepperX.dirPin, xDir ? HIGH : LOW);
  digitalWrite(stepperY.dirPin, yDir ? HIGH : LOW);

  int dx = xSteps;
  int dy = ySteps;

  int sx = (dx > dy);
  int sy = !sx;

  int err = dx - dy;
  int e2;

  int x = 0, y = 0;
  int maxSteps = max(dx, dy);

  for (int i = 0; i < maxSteps; i++) {
    if (x < dx) digitalWrite(stepperX.stepPin, HIGH);
    if (y < dy) digitalWrite(stepperY.stepPin, HIGH);
    delayMicroseconds(speedMicros);
    if (x < dx) digitalWrite(stepperX.stepPin, LOW);
    if (y < dy) digitalWrite(stepperY.stepPin, LOW);
    delayMicroseconds(speedMicros);

    e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x++; }
    if (e2 < dx)  { err += dx; y++; }
  }

  stepperX.currentPositionSteps += (xDir ? -xSteps : xSteps);
  stepperY.currentPositionSteps += (yDir ? -ySteps : ySteps);

  Serial.print("X: ");
  Serial.print(stepperX.currentPositionSteps / stepperX.stepsPerMM, 2);
  Serial.print(" mm | Y: ");
  Serial.print(stepperY.currentPositionSteps / stepperY.stepsPerMM, 2);
  Serial.println(" mm");
}
