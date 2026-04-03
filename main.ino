// ================================================
// ACS712 CURRENT SENSOR + WiFi HTTP POST
// ESP32 → Flask-SocketIO Server
// ================================================

#include <WiFi.h>
#include <HTTPClient.h>

// -------- WiFi Config --------
const char* ssid     = "Airtel_moha_4524";
const char* password = "Air@09640";

// -------- Server Config --------
const char* serverURL = "http://YOUR_SERVER_IP:5000/api/data";

// -------- Sensor Config --------
#define SENSOR_PIN 34
int offsetADC = 0;
float sensitivity = 0.185;  // 5A module (V/A)
int samples = 50;

// -------- State --------
float current_mA = 0;
float current_A = 0;
int currentADC = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  // Auto-calibrate (no load on sensor)
  Serial.println("Calibrating... Keep NO LOAD connected!");
  long sum = 0;
  for (int i = 0; i < 100; i++) {
    sum += analogRead(SENSOR_PIN);
    delay(5);
  }
  offsetADC = sum / 100;
  Serial.print("Calibration done. Offset ADC = ");
  Serial.println(offsetADC);
}

// ================================================
//  Read Current from ACS712
// ================================================
float readCurrent_mA() {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(SENSOR_PIN);
    delay(2);
  }
  currentADC = sum / samples;
  float voltage = (currentADC - offsetADC) * (3.3 / 4095.0);
  return (voltage / sensitivity) * 1000.0;
}

// ================================================
//  Get Status Text Based on Current
// ================================================
String getStatus(float mA) {
  if (mA < 1.0 && mA > -1.0) return "No Load";
  if (mA < 0)    return "Reverse Current";
  if (mA < 10)   return "Very Low Load";
  if (mA < 50)   return "Low Load";
  if (mA < 200)  return "Medium Load";
  if (mA < 1000) return "High Load";
  return "Very High Load!";
}

// ================================================
//  Send Data to Flask Server
// ================================================
void sendToServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    WiFi.reconnect();
    return;
  }

  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  String json = "{\"current_mA\":" + String(current_mA, 1)
              + ",\"current_A\":" + String(current_A, 3)
              + ",\"adc\":" + String(currentADC)
              + ",\"offset\":" + String(offsetADC)
              + ",\"status\":\"" + getStatus(current_mA) + "\"}";

  int code = http.POST(json);
  if (code > 0) {
    Serial.print("POST OK → ");
    Serial.println(code);
  } else {
    Serial.print("POST failed: ");
    Serial.println(http.errorToString(code));
  }
  http.end();
}

// ================================================
//  Main Loop
// ================================================
void loop() {
  current_mA = readCurrent_mA();
  current_A = current_mA / 1000.0;

  // Serial debug
  Serial.print("Current: ");
  Serial.print(current_mA, 1);
  Serial.print(" mA | Status: ");
  Serial.println(getStatus(current_mA));

  sendToServer();
  delay(500);
}
