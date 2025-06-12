const int pwmMotorPin = 10;

void setup() {
  pinMode(pwmMotorPin, OUTPUT);
}

void loop() {
  for (int pwm = 0; pwm <= 255; pwm++) {
    analogWrite(pwmMotorPin, pwm);
    delay(20);
  }

  delay(1000);

  for (int pwm = 255; pwm >= 0; pwm--) {
    analogWrite(pwmMotorPin, pwm);
    delay(20);
  }

  delay(1000);
}
