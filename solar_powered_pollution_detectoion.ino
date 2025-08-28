#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// === LCD ===
LiquidCrystal_I2C lcd(0x27, 16, 2); // Address 0x27, 16x2 LCD

// === Pins ===
#define MQ135_PIN A0     // Air Quality Sensor
#define TDS_PIN   A1     // Water TDS Sensor
#define BUZZER_PIN 13    // Buzzer

// === TDS Constants ===
#define VREF 5.0         // Analog reference voltage
#define SAMPLES 30       // Number of readings for median filter
float temperature = 25;  // Fixed temperature for compensation

int tdsSamples[SAMPLES]; // Buffer for TDS readings
int sampleIndex = 0;

// === Pollution Thresholds ===
#define TDS_THRESHOLD 700
#define AIR_THRESHOLD 400

void setup() {
  Serial.begin(9600);
  pinMode(BUZZER_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
}

void loop() {
  // === 1. Read Sensors ===
  int airValue = analogRead(MQ135_PIN); // Air Quality
  int tdsRaw = analogRead(TDS_PIN);     // One TDS reading

  // Store TDS sample
  tdsSamples[sampleIndex++] = tdsRaw;
  if (sampleIndex >= SAMPLES) sampleIndex = 0;

  // Wait for 800ms to process data
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 800) {
    lastUpdate = millis();

    // === 2. Process TDS ===
    float voltage = getMedian(tdsSamples, SAMPLES) * VREF / 1024.0;
    float compensation = 1.0 + 0.02 * (temperature - 25.0);
    float compensatedVoltage = voltage / compensation;
    float tdsValue = (133.42 * pow(compensatedVoltage, 3)
                    - 255.86 * pow(compensatedVoltage, 2)
                    + 857.39 * compensatedVoltage) * 0.5;

    // === 3. Check Pollution Status ===
    bool waterPollution = tdsValue > TDS_THRESHOLD;
    bool airPollution = airValue > AIR_THRESHOLD;
    bool alert = waterPollution || airPollution;

    // === 4. Display on LCD ===
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("TDS:");
    lcd.print((int)tdsValue);
    lcd.print(" AQ:");
    lcd.print(airValue);

    lcd.setCursor(0, 1);
    if (waterPollution && airPollution) {
      lcd.print("Water & Air Poll!");
    } else if (waterPollution) {
      lcd.print("Water Pollution! ");
    } else if (airPollution) {
      lcd.print("Air Pollution!   ");
    } else {
      lcd.print("Status: Clean    ");
    }

    // === 5. Buzzer ===
    digitalWrite(BUZZER_PIN, alert ? HIGH : LOW);

    // === 6. Serial Debugging ===
    Serial.print("TDS: ");
    Serial.print(tdsValue);
    Serial.print(" ppm | Air: ");
    Serial.println(airValue);
  }

  delay(100); // Short delay to stabilize readings
}

// === Median Filter ===
int getMedian(int arr[], int len) {
  int sorted[len];
  for (int i = 0; i < len; i++) sorted[i] = arr[i];

  for (int i = 0; i < len - 1; i++) {
    for (int j = 0; j < len - i - 1; j++) {
      if (sorted[j] > sorted[j + 1]) {
        int temp = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = temp;
      }
    }
  }

  // Return median
  if (len % 2 == 0)
    return (sorted[len / 2 - 1] + sorted[len / 2]) / 2;
  else
    return sorted[len / 2];
}
