// ================================================
// ACS712 CURRENT SENSOR (CALIBRATED VERSION)
// ESP32 - Accurate Reading with Noise Filtering
// ================================================

#define SENSOR_PIN 34

int offsetADC = 0;        // Will be auto-calibrated
float sensitivity = 0.185; // For 5A module (V/A)

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Calibrating... Keep NO LOAD connected!");

  // 🔹 Auto Calibration (take 100 readings)
  long sum = 0;
  for (int i = 0; i < 100; i++) {
    sum += analogRead(SENSOR_PIN);
    delay(5);
  }
  offsetADC = sum / 100;

  Serial.print("Calibration Done. Offset ADC = ");
  Serial.println(offsetADC);
  Serial.println("----------------------------------");
}

void loop() {

  // 🔹 Read multiple samples for stability
  long sum = 0;
  int samples = 50;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(SENSOR_PIN);
  }

  int adcValue = sum / samples;

  // 🔹 Convert ADC difference to voltage
  float voltage = (adcValue - offsetADC) * (3.3 / 4095.0);

  // 🔹 Convert voltage to current
  float current = voltage / sensitivity;

  // 🔹 Convert to mA
  float current_mA = current * 1000;

  // 🔹 Print values
  Serial.print("ADC: ");
  Serial.print(adcValue);

  Serial.print(" | Current: ");
  Serial.print(current, 3);
  Serial.print(" A (");
  Serial.print(current_mA, 1);
  Serial.println(" mA)");

  delay(500);
}
