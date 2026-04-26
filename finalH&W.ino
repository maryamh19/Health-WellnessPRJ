#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define TEMP_PIN   7
#define BUZZER_PIN 8
#define RGB_RED    11
#define RGB_GREEN  10
#define RGB_BLUE   9
#define TEMP_MAX   38.0
#define TEMP_MIN   35.0
#define TEMP_DANGER 39.5

LiquidCrystal_I2C lcd(0x27, 16, 2);
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

unsigned long lastSensor = 0;
unsigned long lastBuzz   = 0;
unsigned long lastBlink  = 0;
float currentTemp = 0.0;
bool ledState = false;

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RGB_RED,    OUTPUT);
  pinMode(RGB_GREEN,  OUTPUT);
  pinMode(RGB_BLUE,   OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.print("Temp Monitor");
  delay(1500);
  lcd.clear();

  sensors.begin();
  sensors.setWaitForConversion(true);  // wait for real reading
}

void loop() {
  if (millis() - lastSensor > 1000) {
    lastSensor = millis();

    sensors.requestTemperatures();
    float t = sensors.getTempCByIndex(0);
    currentTemp = (t < 10 || t > 45) ? 0 : t;

    bool isHigh   = (currentTemp > 0 && currentTemp >= TEMP_MAX);
    bool isLow    = (currentTemp > 0 && currentTemp <= TEMP_MIN);
    bool isDanger = (currentTemp > 0 && currentTemp >= TEMP_DANGER);

    handleBuzzer(isHigh, isLow, isDanger);
    handleRGB(isHigh, isLow, isDanger);
    updateLCD(isHigh, isLow, isDanger);

    // Serial JSON for website
    String status = isDanger ? "DANGER" : isHigh ? "HIGH" : isLow ? "LOW" : "NORMAL";
    Serial.print("{\"temp\":"); Serial.print(currentTemp, 1);
    Serial.print(",\"status\":\""); Serial.print(status);
    Serial.println("\"}");
  }
}

void handleBuzzer(bool isHigh, bool isLow, bool isDanger) {
  unsigned long now = millis();
  if (isDanger) {
    tone(BUZZER_PIN, 2000);
  } else if (isHigh) {
    if (now - lastBuzz > 300) { lastBuzz = now; tone(BUZZER_PIN, 1200, 150); }
  } else if (isLow) {
    if (now - lastBuzz > 1500) { lastBuzz = now; tone(BUZZER_PIN, 600, 300); }
  } else {
    noTone(BUZZER_PIN);
  }
}

void handleRGB(bool isHigh, bool isLow, bool isDanger) {
  unsigned long now = millis();
  if (isDanger) {
    if (now - lastBlink > 150) { lastBlink = now; ledState = !ledState; setRGB(ledState, LOW, LOW); }
  } else if (isHigh) {
    if (now - lastBlink > 400) { lastBlink = now; ledState = !ledState; setRGB(ledState, LOW, LOW); }
  } else if (isLow) {
    if (now - lastBlink > 1000) { lastBlink = now; ledState = !ledState; setRGB(LOW, LOW, ledState); }
  } else {
    setRGB(LOW, HIGH, LOW);
  }
}

void updateLCD(bool isHigh, bool isLow, bool isDanger) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp:");
  lcd.setCursor(0, 1);
  if (currentTemp > 0) {
    lcd.print(currentTemp, 1);
    lcd.print(" C ");
    if      (isDanger) lcd.print("DANGER");
    else if (isHigh)   lcd.print("HIGH");
    else if (isLow)    lcd.print("LOW");
    else               lcd.print("OK");
  } else {
    lcd.print("Out of scope");
  }
}

void setRGB(bool r, bool g, bool b) {
  digitalWrite(RGB_RED,   r);
  digitalWrite(RGB_GREEN, g);
  digitalWrite(RGB_BLUE,  b);
}