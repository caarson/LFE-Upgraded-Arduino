#include <max6675.h>

// ==== Pin map ====
const int PIN_SSR      = 5;   // Heater SSR
const int PIN_PSU_EN   = 8;   // Drives NPN base (via ~10k) for ATX PS_ON# (green)
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

String rxLine;

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  pinMode(PIN_SSR, OUTPUT);
  pinMode(PIN_MOTOR, OUTPUT);
  pinMode(PIN_FAN, OUTPUT);

  // IMPORTANT WIRING NOTES:
  // - Arduino GND must tie to PSU GND (black).
  // - Power Arduino from USB or +5VSB (purple), so it stays alive during PSU on/off.
  // - D8 -> 10k -> NPN base, NPN emitter -> GND, NPN collector -> PS_ON# (green).
  // Start with PSU OFF: no base drive (hi-Z).
  pinMode(PIN_PSU_EN, INPUT);

  digitalWrite(PIN_SSR, LOW);
  analogWrite(PIN_MOTOR, 0);
  analogWrite(PIN_FAN, 0);

  Serial.println("READY");
}

// =======================
// ==== Core Controls ====
// =======================

void setHeater(bool on) {
  heaterOn = on;
  digitalWrite(PIN_SSR, on ? HIGH : LOW);
}

// ATX PS_ON# via NPN:
//  - ON  -> drive base HIGH (OUTPUT, HIGH) to sink PS_ON# to GND.
//  - OFF -> stop base drive (INPUT or OUTPUT, LOW). We'll use INPUT (hi-Z) for safety.
void setPSU(bool on) {
  psuOn = on;
  if (on) {
    pinMode(PIN_PSU_EN, OUTPUT);
    digitalWrite(PIN_PSU_EN, HIGH);   // base drive -> transistor sinks PS_ON# -> PSU ON
  } else {
    pinMode(PIN_PSU_EN, INPUT);       // hi-Z -> no base drive -> PS_ON# floats HIGH -> PSU OFF
  }
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

// ===========================
// ==== Serial Line Reader ====
// ===========================

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

// ===========================
// ==== Command Handling =====
// ===========================

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

// ===========================
// ==== Telemetry Loop =======
// ===========================

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
