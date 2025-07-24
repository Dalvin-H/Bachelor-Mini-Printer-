const float beltPitch = 2.0;  // GT2 belt

// X-Stepper configuration
const int stepPinX = 24;
const int dirPinX = 22;
const int limitSwitchX = 6;

const int motorStepsX = 200;
const int microstepsX = 4;
const int pulleyTeethX = 20;

const float stepsPerMMX = (motorStepsX * microstepsX) / (pulleyTeethX * beltPitch);

struct Stepper {
  const char* name;
  int stepPin;
  int dirPin;
  int limitSwitchPin;
  float stepsPerMM;
  int currentPositionSteps;
  float maxPositionMM;
};

Stepper stepperX = {"X", stepPinX, dirPinX, limitSwitchX, stepsPerMMX, 0, 65.0};

void setup() {
  Serial.begin(9600);
  pinMode(stepperX.stepPin, OUTPUT);
  pinMode(stepperX.dirPin, OUTPUT);
  pinMode(stepperX.limitSwitchPin, INPUT_PULLUP);

  Serial.println("Ready. Use: F 10, B 5, or H");
}

void loop() {
  if (Serial.available()) {
    char command = Serial.read();
    float value = Serial.parseFloat();

    if (command == 'F') {
      moveStepperMM(stepperX, false, value, 500);  // false = forward
    } 
    else if (command == 'B') {
      moveStepperMM(stepperX, true, value, 500);   // true = backward
    } 
    else if (command == 'H') {
      homeStepper(stepperX);
    } 
    else {
      Serial.println("Invalid input. Use F 10, B 5, or H");
    }

    while (Serial.available()) Serial.read(); // Clear remaining buffer
  }
}

void moveStepper(Stepper& motor, bool directionBackward, int steps, int speedMicros, bool ignoreLimits = false) {
  int newPosition = directionBackward
    ? motor.currentPositionSteps - steps
    : motor.currentPositionSteps + steps;

  int maxSteps = motor.maxPositionMM * motor.stepsPerMM;

  if (!ignoreLimits && (newPosition < 0 || newPosition > maxSteps)) {
    Serial.print("Error: ");
    Serial.print(motor.name);
    Serial.println(" movement out of bounds.");
    return;
  }

  digitalWrite(motor.dirPin, directionBackward ? HIGH : LOW);

  for (int i = 0; i < steps; i++) {
    digitalWrite(motor.stepPin, HIGH);
    delayMicroseconds(speedMicros);
    digitalWrite(motor.stepPin, LOW);
    delayMicroseconds(speedMicros);
  }

  if (!ignoreLimits)
    motor.currentPositionSteps = newPosition;

  Serial.print(motor.name);
  Serial.print(" position: ");
  Serial.print(motor.currentPositionSteps);
  Serial.print(" steps (");
  Serial.print(motor.currentPositionSteps / motor.stepsPerMM, 2);
  Serial.println(" mm)");
}

void moveStepperMM(Stepper& motor, bool directionBackward, float distanceMM, int speedMicros) {
  int stepsToMove = distanceMM * motor.stepsPerMM;
  moveStepper(motor, directionBackward, stepsToMove, speedMicros);
}

void homeStepper(Stepper& motor) {
  Serial.print("Homing ");
  Serial.println(motor.name);

  // Move backwards until limit switch is hit
  while (digitalRead(motor.limitSwitchPin) != LOW) {
    digitalWrite(motor.dirPin, HIGH); // HIGH = backward
    digitalWrite(motor.stepPin, HIGH);
    delayMicroseconds(1000);
    digitalWrite(motor.stepPin, LOW);
    delayMicroseconds(1000);
  }

  delay(50); // Debounce
  delay(100);
  
  moveStepper(motor, false, 20, 500, true); // Move forward a bit to release

  motor.currentPositionSteps = 0;

  Serial.print("Homing complete. ");
  Serial.print(motor.name);
  Serial.println(" position set to 0 mm.");
}
