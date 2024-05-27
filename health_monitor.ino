#define USE_ARDUINO_INTERRUPTS true
#include <Wire.h>
#include <PulseSensorPlayground.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "MAX30100_PulseOximeter.h"



// --- LCD setup ---
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD I2C address 0x27, 16x2 display

// --- Pulse Sensor setup ---
const int PULSE_SENSOR_PIN = A3; // Analog pin
const int LED_PIN = 13;          // On-board LED
const int THRESHOLD = 550;
PulseSensorPlayground pulseSensor;

// --- DS18B20 Temperature Sensor ---
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float bodyTempC = 0.0;
float bodyTempF = 0.0;

// --- MAX30100 SpO2 Sensor ---
PulseOximeter pox;
uint32_t tsLastReport = 0;
#define REPORTING_PERIOD_MS 1000
float spo2 = 0.0;

// Beat callback for MAX30100
void onBeatDetected() {
  Serial.println("Beat detected by MAX30100!");
}

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  // --- Setup Pulse Sensor ---
  pulseSensor.analogInput(PULSE_SENSOR_PIN);
  pulseSensor.blinkOnPulse(LED_PIN);
  pulseSensor.setThreshold(THRESHOLD);

  if (pulseSensor.begin()) {
    Serial.println("PulseSensor initialized.");
  } else {
    Serial.println("PulseSensor FAILED to initialize!");
  }

  // --- Setup DS18B20 ---
  sensors.begin();

  // --- Setup MAX30100 ---
  Serial.print("Initializing MAX30100...");
  if (!pox.begin()) {
    Serial.println("FAILED");
    while (1);
  } else {
    Serial.println("SUCCESS");
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);

  // --- Welcome Message ---
  lcd.setCursor(0, 0);
  lcd.print("Health Monitor");
  delay(2000);
  lcd.clear();
}

void loop() {
  pox.update();

  // --- Display Heart Rate (from Pulse Sensor) ONCE per beat ---
  int currentBPM = pulseSensor.getBeatsPerMinute();
  if (pulseSensor.sawStartOfBeat()) {
    if (currentBPM > 40 && currentBPM < 180) {
      Serial.println("♥ HeartBeat Detected!");
      Serial.print("BPM: ");
      Serial.println(currentBPM);
      lcd.setCursor(0, 0);
      lcd.print("Pulse: ");
      lcd.print(currentBPM);
      lcd.print(" BPM ");
  } else {
    Serial.print("Ignored BPM (out of range): ");
    Serial.println(currentBPM);
    }
  }

  // --- Temperature Reading ---
  sensors.requestTemperatures();
  bodyTempC = sensors.getTempCByIndex(0);
  bodyTempF = bodyTempC * 1.8 + 32;

  // --- SpO2 Reporting Every 1 Sec ---
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    float heartRateFromMAX = pox.getHeartRate(); // Not displayed
    spo2 = pox.getSpO2();

    Serial.print("Body Temp (C): ");
    Serial.print(bodyTempC);
    Serial.print(" | Body Temp (F): ");
    Serial.println(bodyTempF);

    Serial.print("SpO2: ");
    Serial.print(spo2);
    Serial.println("%");

    // Clear Line 2 before displaying
    lcd.setCursor(0, 1);
    lcd.print("                "); // 16 spaces to clear the line


    // Display Temp & SpO2
    lcd.setCursor(0, 1);
    lcd.print("T:");
    lcd.print(bodyTempF, 1); // 1 decimal place
    lcd.print("F ");
    lcd.print("O2:");
    lcd.print(spo2, 0); // Integer SpO2
    lcd.print("% ");

    tsLastReport = millis();
  }

  delay(20); // Small delay for stability
}