// ================================================
// ACS712 CURRENT SENSOR (CALIBRATED VERSION)
// ESP32 - Accurate Reading with Noise Filtering
// Beautiful Serial Output Format
// ================================================

#define SENSOR_PIN 34

int offsetADC = 0;           // Will be auto-calibrated
float sensitivity = 0.185;   // For 5A module (V/A)
int samples = 50;            // Number of samples for averaging

// For status display
unsigned long lastDisplay = 0;
float current_mA = 0;
float current_A = 0;
int currentADC = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n╔══════════════════════════════════════════════════╗");
  Serial.println("║        ACS712 CURRENT SENSOR CALIBRATION         ║");
  Serial.println("╚══════════════════════════════════════════════════╝");
  Serial.println();
  Serial.println("🔧 Calibrating... Keep NO LOAD connected to IP+ and IP-!");
  Serial.println();

  // 🔹 Auto Calibration (take 100 readings)
  long sum = 0;
  for (int i = 0; i < 100; i++) {
    sum += analogRead(SENSOR_PIN);
    delay(5);
  }
  offsetADC = sum / 100;

  Serial.print("✅ Calibration Complete! Offset ADC = ");
  Serial.println(offsetADC);
  Serial.println("═══════════════════════════════════════════════════");
  Serial.println();
  Serial.println("📊 Starting Current Monitoring...");
  Serial.println();
}

// ================================================
//  FUNCTION: Read Current from ACS712
// ================================================
float readCurrent_mA() {
  // Take multiple samples for stability
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(SENSOR_PIN);
    delay(2);
  }
  
  currentADC = sum / samples;
  
  // Convert ADC difference to voltage
  float voltage = (currentADC - offsetADC) * (3.3 / 4095.0);
  
  // Convert voltage to current (Amps)
  float currentAmps = voltage / sensitivity;
  
  return currentAmps * 1000;  // Return in mA
}

// ================================================
//  FUNCTION: Get Status Text Based on Current
// ================================================
String getStatusText(float mA) {
  if (mA < 1.0 && mA > -1.0) {
    return "No Load";
  } else if (mA < 0) {
    return "Reverse Current";
  } else if (mA < 10.0) {
    return "Very Low Load";
  } else if (mA < 50.0) {
    return "Low Load";
  } else if (mA < 200.0) {
    return "Medium Load";
  } else if (mA < 1000.0) {
    return "High Load";
  } else {
    return "Very High Load!";
  }
}

// ================================================
//  FUNCTION: Display Beautiful Table Format
// ================================================
void displayReadings() {
  // Read current
  current_mA = readCurrent_mA();
  current_A = current_mA / 1000;
  
  // Clear screen every 10 readings (optional - comment if not wanted)
  // static int counter = 0;
  // counter++;
  // if (counter >= 10) {
  //   Serial.print("\033[2J\033[H");  // Clear screen (works in some terminals)
  //   counter = 0;
  // }
  
  Serial.println("┌─────────────────────────────────────────────────┐");
  Serial.println("│           CURRENT SENSOR READING                │");
  Serial.println("├─────────────────────────────────────────────────┤");
  
  // ADC Value
  Serial.print("│  ADC Value: ");
  Serial.print(currentADC);
  for (int i = String(currentADC).length(); i < 47; i++) Serial.print(" ");
  Serial.println("│");
  
  // Offset ADC
  Serial.print("│  Offset ADC: ");
  Serial.print(offsetADC);
  for (int i = String(offsetADC).length(); i < 47; i++) Serial.print(" ");
  Serial.println("│");
  
  // Current in Amps
  Serial.print("│  Current: ");
  Serial.print(current_A, 3);
  Serial.print(" A");
  for (int i = String(current_A, 3).length() + 3; i < 47; i++) Serial.print(" ");
  Serial.println("│");
  
  // Current in mA
  Serial.print("│  Current: ");
  Serial.print(current_mA, 1);
  Serial.print(" mA");
  for (int i = String(current_mA, 1).length() + 4; i < 47; i++) Serial.print(" ");
  Serial.println("│");
  
  // Status
  Serial.print("│  Status: ");
  Serial.print(getStatusText(current_mA));
  int statusLen = getStatusText(current_mA).length();
  for (int i = statusLen; i < 47; i++) Serial.print(" ");
  Serial.println("│");
  
  Serial.println("└─────────────────────────────────────────────────┘");
  Serial.println();
}

// ================================================
//  FUNCTION: Handle Serial Commands
// ================================================
void handleSerialCommands() {
  if (Serial.available()) {
    String cmd = Serial.readString();
    cmd.trim();
    cmd.toLowerCase();
    
    if (cmd == "status") {
      Serial.println("\n╔══════════════════════════════════════════════════╗");
      Serial.println("║              SENSOR STATUS                       ║");
      Serial.println("╠══════════════════════════════════════════════════╣");
      Serial.print  ("║  Current ADC: ");
      Serial.print(currentADC);
      for (int i = String(currentADC).length(); i < 31; i++) Serial.print(" ");
      Serial.println("║");
      Serial.print  ("║  Offset ADC: ");
      Serial.print(offsetADC);
      for (int i = String(offsetADC).length(); i < 31; i++) Serial.print(" ");
      Serial.println("║");
      Serial.print  ("║  Current: ");
      Serial.print(current_mA, 1);
      Serial.print(" mA");
      for (int i = String(current_mA, 1).length() + 4; i < 31; i++) Serial.print(" ");
      Serial.println("║");
      Serial.print  ("║  Status: ");
      Serial.print(getStatusText(current_mA));
      for (int i = getStatusText(current_mA).length(); i < 31; i++) Serial.print(" ");
      Serial.println("║");
      Serial.println("╚══════════════════════════════════════════════════╝\n");
    }
    else if (cmd == "calibrate") {
      Serial.println("\n🔧 Re-calibrating...");
      long sum = 0;
      for (int i = 0; i < 100; i++) {
        sum += analogRead(SENSOR_PIN);
        delay(5);
      }
      offsetADC = sum / 100;
      Serial.print("✅ Calibration Complete! New Offset ADC = ");
      Serial.println(offsetADC);
      Serial.println();
    }
    else if (cmd == "help") {
      Serial.println("\n╔══════════════════════════════════════════════════╗");
      Serial.println("║                   COMMANDS                       ║");
      Serial.println("╠══════════════════════════════════════════════════╣");
      Serial.println("║  status    - Show sensor status                  ║");
      Serial.println("║  calibrate - Re-calibrate sensor                 ║");
      Serial.println("║  help      - Show this menu                      ║");
      Serial.println("╚══════════════════════════════════════════════════╝\n");
    }
    else if (cmd.length() > 0) {
      Serial.print("\n❌ Unknown command: '");
      Serial.print(cmd);
      Serial.println("'");
      Serial.println("   Type 'help' to see available commands\n");
    }
  }
}

// ================================================
//  MAIN LOOP
// ================================================
void loop() {
  displayReadings();
  handleSerialCommands();
  delay(500);
}
