#include <max6675.h>

// ==== Pin map ====
const int PIN_SSR      = 5;   // Heater SSR
const int PIN_PSU_EN   = 8;   // ATX power-on (green wire)
const int PIN_MOTOR    = 10;  // Motor PWM
const int PIN_FAN      = 9;   // Fan PWM (can also double as speaker)
const int PIN_SPEAKER  = 7;   // Speaker (shares pin with FAN if you only need beep)

// MAX6675 pins
const int PIN_SCK = A2;
const int PIN_SO  = A1;
const int PIN_CS  = 4;

MAX6675 thermocouple(PIN_SCK, PIN_CS, PIN_SO);

bool heaterOn = false;
bool psuOn    = false;
int motorPwm  = 0;
int fanPwm    = 0;

// Line buffer
String rxLine;

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  pinMode(PIN_SSR, OUTPUT);
  pinMode(PIN_PSU_EN, OUTPUT);
  pinMode(PIN_MOTOR, OUTPUT);
  pinMode(PIN_FAN, OUTPUT);

  digitalWrite(PIN_SSR, LOW);
  digitalWrite(PIN_PSU_EN, HIGH); // ATX "PWR-ON" is active low; HIGH keeps PSU off initially

  analogWrite(PIN_MOTOR, 0);
  analogWrite(PIN_FAN, 0);

  Serial.println("READY");
}

void setHeater(bool on) {
  heaterOn = on;
  digitalWrite(PIN_SSR, on ? HIGH : LOW);
}

void setPSU(bool on) {
  psuOn = on;
  digitalWrite(PIN_PSU_EN, on ? LOW : HIGH); // Active low
}

void setMotor(int pwm) {
  motorPwm = constrain(pwm, 0, 255);
  analogWrite(PIN_MOTOR, motorPwm);
}

void setFan(int pwm) {
  fanPwm = constrain(pwm, 0, 255);
  analogWrite(PIN_FAN, fanPwm);
}

void beep(int ms, int freq = 2000) {
  tone(PIN_SPEAKER, freq);
  delay(ms);
  noTone(PIN_SPEAKER);
}

// Read line from Serial
bool readLine(String &out) {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      out = rxLine;
      rxLine = "";
      return true;
    }
    rxLine += c;
  }
  return false;
}

void handleCommand(const String &cmd) {
  if (cmd == "HEATER_ON")      { setHeater(true);  Serial.println("ACK HEATER=ON"); }
  else if (cmd == "HEATER_OFF"){ setHeater(false); Serial.println("ACK HEATER=OFF"); }
  else if (cmd == "PSU_ON")    { setPSU(true);     Serial.println("ACK PSU=ON"); }
  else if (cmd == "PSU_OFF")   { setPSU(false);    Serial.println("ACK PSU=OFF"); }
  else if (cmd.startsWith("MOTOR:")) {
    setMotor(cmd.substring(6).toInt());
    Serial.print("ACK MOTOR="); Serial.println(motorPwm);
  }
  else if (cmd.startsWith("FAN:")) {
    setFan(cmd.substring(4).toInt());
    Serial.print("ACK FAN="); Serial.println(fanPwm);
  }
  else if (cmd.startsWith("BEEP:")) {
    beep(cmd.substring(5).toInt());
    Serial.println("ACK BEEP");
  }
  else if (cmd == "PING") {
    Serial.println("PONG");
  }
}

unsigned long lastTelem = 0;

void loop() {
  String line;
  if (readLine(line)) {
    line.trim();
    if (line.length()) handleCommand(line);
  }

  unsigned long now = millis();
  if (now - lastTelem >= 500) {  // every 0.5s
    lastTelem = now;

    double tempC = thermocouple.readCelsius();

    Serial.print("STATE ");
    Serial.print("HEATER:"); Serial.print(heaterOn ? 1 : 0); Serial.print(" ");
    Serial.print("PSU:");    Serial.print(psuOn ? 1 : 0);    Serial.print(" ");
    Serial.print("MOTOR:");  Serial.print(motorPwm);         Serial.print(" ");
    Serial.print("FAN:");    Serial.print(fanPwm);           Serial.print(" ");
    Serial.print("TEMP:");   Serial.println(tempC, 2);
  }
}
