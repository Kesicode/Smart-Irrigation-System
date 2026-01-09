#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* ================= OLED ================= */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/* ================= PINS ================= */
#define SOIL_SENSOR_ANALOG A0
#define SOIL_SENSOR_DIGITAL 2
#define RELAY_PUMP 7
#define LED_GREEN 8
#define LED_RED 9

/* ============== SETTINGS =============== */
#define DRY_LEVEL 600           // ABOVE this value = DRY
#define CHECK_INTERVAL 30000UL  // 30 seconds
#define PUMP_RUN_TIME 30000UL   // 30 seconds

/* ============== VARIABLES ============== */
int soilMoisture = 0;
bool digitalDry = false;

bool pumpRunning = false;
unsigned long lastCheckTime = 0;
unsigned long pumpStartTime = 0;

/* ====================================== */
void setup() {
  Serial.begin(9600);

  pinMode(SOIL_SENSOR_ANALOG, INPUT);
  pinMode(SOIL_SENSOR_DIGITAL, INPUT);
  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  digitalWrite(RELAY_PUMP, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println("SMART");
  display.setCursor(5, 40);
  display.println("IRRIGATION SYSTEM");
  display.display();
  delay(2000);

  lastCheckTime = millis();
}

/* ====================================== */
void loop() {
  unsigned long currentTime = millis();

  soilMoisture = analogRead(SOIL_SENSOR_ANALOG);
  digitalDry = (digitalRead(SOIL_SENSOR_DIGITAL) == HIGH);

  /* ===== PUMP CONTROL ===== */
  if (pumpRunning) {
    if (currentTime - pumpStartTime >= PUMP_RUN_TIME) {
      digitalWrite(RELAY_PUMP, LOW);
      pumpRunning = false;
      lastCheckTime = currentTime;
      Serial.println("Pump stopped");
    }
  }
  else {
    if (currentTime - lastCheckTime >= CHECK_INTERVAL) {
      Serial.println("Checking soil...");
      Serial.print("Analog: ");
      Serial.print(soilMoisture);
      Serial.print(" | Digital: ");
      Serial.println(digitalDry ? "DRY" : "WET");

      bool analogDry = (soilMoisture > DRY_LEVEL);

      if (analogDry && digitalDry) {
        digitalWrite(RELAY_PUMP, HIGH);
        pumpRunning = true;
        pumpStartTime = currentTime;
        Serial.println("Soil dry → Pump ON");
      } else {
        Serial.println("Soil OK → No watering");
        lastCheckTime = currentTime;
      }
    }
  }

  /* ===== LED STATUS ===== */
  if (pumpRunning) {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, HIGH);
  }
  else if (soilMoisture > DRY_LEVEL || digitalDry) {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
  }
  else {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
  }

  updateDisplay();
  delay(300);
}

/* ====================================== */
void updateDisplay() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SMART IRRIGATION");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setCursor(0, 15);
  display.print("Analog: ");
  display.print(soilMoisture);
  display.print("  D0: ");
  display.println(digitalDry ? "DRY" : "WET");

  // Moisture bar (FULL = wet)
  int bar = map(soilMoisture, 1023, 0, 0, 120);
  bar = constrain(bar, 0, 120);
  display.drawRect(2, 27, 124, 8, SSD1306_WHITE);
  display.fillRect(3, 28, bar, 6, SSD1306_WHITE);

  display.setCursor(0, 40);

  if (pumpRunning) {
    int remaining = max(0, (int)((PUMP_RUN_TIME - (millis() - pumpStartTime)) / 1000));
    display.println("Status: WATERING");
    display.setCursor(0, 50);
    display.print("Time left: ");
    display.print(remaining);
    display.println("s");
  } else {
    int nextCheck = max(0, (int)((CHECK_INTERVAL - (millis() - lastCheckTime)) / 1000));
    display.println((soilMoisture > DRY_LEVEL || digitalDry) ? "Status: DRY" : "Status: GOOD");
    display.setCursor(0, 50);
    display.print("Next check: ");
    display.print(nextCheck);
    display.println("s");
  }

  display.display();
}
