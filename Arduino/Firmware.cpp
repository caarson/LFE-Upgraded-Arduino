#include <max6675.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pin Definitions
const int thermoSO = 3;
const int thermoCS = 4;
const int thermoSCK = 2;

const int ssrPin = 5;
const int psuPin = 8;
const int hallPin = 6;
const int buttonPin = 7;

MAX6675 thermocouple(thermoSCK, thermoCS, thermoSO);
LiquidCrystal_I2C lcd(0x27, 20, 4); // Adjust I2C address if needed

void setup() {
  pinMode(ssrPin, OUTPUT);
  pinMode(psuPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(hallPin, INPUT);

  digitalWrite(psuPin, HIGH); // Enable PSU
  digitalWrite(ssrPin, LOW);  // SSR off

  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
}

void loop() {
  float tempC = thermocouple.readCelsius();
  int buttonState = digitalRead(buttonPin);
  int hallState = digitalRead(hallPin);

  // Display info
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(tempC);
  lcd.print(" C  ");

  lcd.setCursor(0, 1);
  lcd.print("Flow: ");
  lcd.print(hallState ? "ON " : "OFF");

  lcd.setCursor(0, 2);
  lcd.print("Btn: ");
  lcd.print(buttonState ? "Released" : "Pressed");

  Serial.print("TEMP(C): ");
  Serial.println(tempC);

  // Simple control logic
  if (!buttonState && tempC < 150) {
    digitalWrite(ssrPin, HIGH); // turn heater ON
  } else {
    digitalWrite(ssrPin, LOW); // turn heater OFF
  }

  delay(500);
}
