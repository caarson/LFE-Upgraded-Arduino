#include <max6675.h>

// Thermocouple pins
const int thermoSO = 3;
const int thermoCS = 4;
const int thermoSCK = 2;

// SSR and speaker pins
const int ssrPin = 5;
const int speakerPin = 9;

// Stepper motor control
const int stepPin = 10;
const int dirPin = 7;

MAX6675 thermocouple(thermoSCK, thermoCS, thermoSO);

void setup() {
  Serial.begin(9600);

  pinMode(ssrPin, OUTPUT);
  pinMode(speakerPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);

  digitalWrite(ssrPin, LOW);
  digitalWrite(dirPin, HIGH); // Default direction
}

void loop() {
  handleSerial();

  float temp = thermocouple.readCelsius();
  Serial.print("TEMP(C): ");
  Serial.println(temp);

  delay(500);  // Don't spam serial too fast
}

void handleSerial() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');

    if (cmd == "HEATER_ON") {
      digitalWrite(ssrPin, HIGH);
    } else if (cmd == "HEATER_OFF") {
      digitalWrite(ssrPin, LOW);
    } else if (cmd.startsWith("MOTOR:")) {
      int pwm = cmd.substring(6).toInt();
      analogWrite(speakerPin, pwm); // For testing PWM on speaker
    } else if (cmd.startsWith("STEP:")) {
      int count = cmd.substring(5).toInt();
      stepMotor(count);
    }
  }
}

void stepMotor(int steps) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(500);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(500);
  }
}
