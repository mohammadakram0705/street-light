// ================================================
// ESP32: INA219 (LED1) + ACS712 (LED2) → FLASK
// ================================================
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

// -------- WiFi --------
const char* ssid     = "Airtel_moha_4524";
const char* password = "Air@09640";

// -------- Server --------
const char* serverURL = "http://192.168.1.20:5000/api/data";

// -------- Street Info --------
String STREET = "MG Road";
int    LIGHT1 = 101;
int    LIGHT2 = 102;

// -------- INA219 (LED1) --------
Adafruit_INA219 ina219;

// -------- ACS712 (LED2) --------
#define SENSOR_PIN      34
#define ACS_SENSITIVITY 0.185f
#define ACS_SAMPLES     50
#define ACS_NOISE_CUT   20.0f
int offsetADC = 0;

// -------- Fault thresholds --------
#define LED1_FAULT_THRESH  1.0f
#define LED2_FAULT_THRESH 30.0f
#define FAULT_CONFIRM      4

// -------- State --------
float led1_mA      = 0;
float led2_mA      = 0;
int   faultCounter = 0;
bool  faultLatched = false;

// ================================================
float readACS712() {
  long sum = 0;
  for (int i = 0; i < ACS_SAMPLES; i++) {
    sum += analogRead(SENSOR_PIN);
    delayMicroseconds(100);
  }
  int   avgADC  = sum / ACS_SAMPLES;
  float voltage = (avgADC - offsetADC) * (3.3f / 4095.0f);
  float current = fabsf((voltage / ACS_SENSITIVITY) * 1000.0f);
  return (current < ACS_NOISE_CUT) ? 0.0f : current;
}

// ================================================
String buildStatus(bool f1, bool f2) {
  if (f1 && f2) return "BOTH_FAULT";
  if (f1)       return "LIGHT1_FAULT";
  if (f2)       return "LIGHT2_FAULT";
  return "ALL_WORKING";
}

// ================================================
void sendData(bool led1Fault, bool led2Fault, String status) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return;
  }

  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  String json = "{";
  json += "\"street\":\""        + STREET            + "\",";
  json += "\"light1_id\":"       + String(LIGHT1)     + ",";
  json += "\"light2_id\":"       + String(LIGHT2)     + ",";
  json += "\"led1_current\":"    + String(led1_mA, 2) + ",";
  json += "\"led2_current\":"    + String(led2_mA, 2) + ",";
  json += "\"led1_fault\":"      + String(led1Fault ? "true" : "false") + ",";
  json += "\"led2_fault\":"      + String(led2Fault ? "true" : "false") + ",";
  json += "\"system_status\":\"" + status             + "\"";
  json += "}";

  int code = http.POST(json);
  if (code > 0) {
    Serial.println("POST OK: " + String(code));
  } else {
    Serial.println("POST FAILED: " + http.errorToString(code));
  }
  http.end();
}

// ================================================
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);  // SDA=21, SCL=22

  // INA219
  if (!ina219.begin()) {
    Serial.println("INA219 NOT FOUND – check wiring!");
    while (1) delay(100);
  }
  Serial.println("INA219 OK");

  // ACS712 ADC
  analogReadResolution(12);
  analogSetPinAttenuation(SENSOR_PIN, ADC_11db);

  // WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());

  // ACS712 zero calibration (run with LED2 OFF)
  Serial.println("Calibrating ACS712 offset...");
  long sum = 0;
  for (int i = 0; i < 200; i++) {
    sum += analogRead(SENSOR_PIN);
    delay(5);
  }
  offsetADC = sum / 200;
  Serial.println("ACS712 Offset ADC = " + String(offsetADC));

  Serial.println("=== System Ready ===");
}

// ================================================
void loop() {
  // Read sensors
  led1_mA = fabsf(ina219.getCurrent_mA());
  led2_mA = readACS712();

  bool led1Fault = (led1_mA < LED1_FAULT_THRESH);
  bool led2Fault = (led2_mA < LED2_FAULT_THRESH);
  String status  = buildStatus(led1Fault, led2Fault);

  // Anti-flicker: confirm fault over N readings
  if (status != "ALL_WORKING") faultCounter++;
  else                         faultCounter = 0;
  faultLatched = (faultCounter > FAULT_CONFIRM);

  // Serial monitor
  Serial.println("==============================");
  Serial.println("Street : " + STREET);
  Serial.println("LED1 (INA219) : " + String(led1_mA, 2) + " mA  " + (led1Fault ? "FAULT" : "OK"));
  Serial.println("LED2 (ACS712) : " + String(led2_mA, 2) + " mA  " + (led2Fault ? "FAULT" : "OK"));
  Serial.println("Status        : " + status);

  sendData(led1Fault, led2Fault, status);

  delay(500);
}
