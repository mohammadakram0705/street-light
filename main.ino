// ================================================
// ACS712 STABLE CURRENT + FAULT FLAG (NEGATIVE)
// ================================================

#include <WiFi.h>
#include <HTTPClient.h>

// -------- WiFi Config --------
const char* ssid     = "Airtel_moha_4524";
const char* password = "Air@09640";

// -------- Server Config --------
const char* serverURL = "http://192.168.1.20:5000/api/data";

// -------- Sensor Config --------
#define SENSOR_PIN 34
float sensitivity = 0.185;
int samples = 200;

// -------- Calibration --------
int offsetADC = 0;

// -------- Filtering --------
float filteredCurrent = 0;
float alpha = 0.2;

// -------- State --------
float current_mA = 0;
float current_A = 0;
int currentADC = 0;

// -------- Fault --------
bool fault = false;

// ================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(SENSOR_PIN, ADC_11db);

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  Serial.println("Calibrating (NO LOAD!)...");
  long sum = 0;
  for (int i = 0; i < 500; i++) {
    sum += analogRead(SENSOR_PIN);
    delay(2);
  }
  offsetADC = sum / 500;

  Serial.print("Offset ADC: ");
  Serial.println(offsetADC);
}

// ================================================
float readCurrent_mA() {

  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(SENSOR_PIN);
    delayMicroseconds(200);
  }

  currentADC = sum / samples;

  float voltage = (currentADC - offsetADC) * (3.3 / 4095.0);
  float rawCurrent = (voltage / sensitivity) * 1000.0;

  // Dead zone
  if (abs(rawCurrent) < 10) rawCurrent = 0;

  // EMA filter
  filteredCurrent = (alpha * rawCurrent) + ((1 - alpha) * filteredCurrent);

  return filteredCurrent;
}

// ================================================
String getStatus(float mA) {
  if (abs(mA) < 10) return "No Load";
  if (mA < 0) return "FAULT";
  if (mA < 50) return "Low Load";
  if (mA < 200) return "Medium Load";
  return "Normal Load";
}

// ================================================
void sendToServer() {

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    return;
  }

  // 🔥 Fault condition
  if (current_mA < 0) {
    fault = true;
  } else {
    fault = false;
  }

  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  // ✅ JSON with fault field
  String json = "{";
  json += "\"current_mA\":" + String(current_mA, 2) + ",";
  json += "\"current_A\":" + String(current_A, 4) + ",";
  json += "\"adc\":" + String(currentADC) + ",";
  json += "\"offset\":" + String(offsetADC) + ",";
  json += "\"status\":\"" + getStatus(current_mA) + "\",";
  json += "\"fault\":" + String(fault ? "true" : "false");
  json += "}";

  int code = http.POST(json);

  if (code > 0) {
    Serial.println("POST OK");
  } else {
    Serial.println("POST FAILED");
  }

  http.end();
}

// ================================================
void loop() {

  current_mA = readCurrent_mA();
  current_A = current_mA / 1000.0;

  Serial.print("ADC: ");
  Serial.print(currentADC);

  Serial.print(" | Current: ");
  Serial.print(current_mA, 2);
  Serial.print(" mA | ");

  if (current_mA < 0) {
    Serial.println("⚠️ FAULT DETECTED");
  } else {
    Serial.println(getStatus(current_mA));
  }

  sendToServer();
  delay(500);
}
