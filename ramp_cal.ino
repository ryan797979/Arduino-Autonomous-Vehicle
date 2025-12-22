#include <LiquidCrystal.h>
#include <Wire.h>

// --- LCD Setup (Backlight + 16x2 Fit) ---
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
const int lcdBacklightPin = 10;

// --- Pin Definitions ---
int enA = 0;    // PWM for right wheels
int in1 = 12;   // Right wheel direction 1
int in2 = 13;   // Right wheel direction 2
int enB = 11;   // PWM for left wheels
int in3 = 3;    // Left wheel direction 1
int in4 = 2;    // Left wheel direction 2
const int motorSpeed = 250;

// --- MPU-6050 Configuration (AXIS 3 = Y-) ---
const int MPU_ADDR = 0x68;
int16_t accelY, accelZ;
const float accelScale = 16384.0; // ±2g scale

// --- Peak Angle + Offset Configuration (KEY UPDATES) ---
float peakRampAngle = 0.0;
const float angleOffset = 0.0; // Counteract momentum spikes (-10° as requested)
const float spikeThreshold = 5.0; // Ignore spikes >5° above current peak (anti-spike)

// --- Motor Functions ---
void rightWheelsForward() { digitalWrite(in1, HIGH); digitalWrite(in2, LOW); }
void rightWheelsBackward() { digitalWrite(in1, LOW); digitalWrite(in2, HIGH); }
void leftWheelsForward() { digitalWrite(in3, LOW); digitalWrite(in4, HIGH); }
void leftWheelsBackward() { digitalWrite(in3, HIGH); digitalWrite(in4, LOW); }
void stopAll() { 
  digitalWrite(in1, LOW); digitalWrite(in2, LOW); 
  digitalWrite(in3, LOW); digitalWrite(in4, LOW); 
}

// --- MPU-6050 Initialization ---
void initMPU6050() {
  Wire.begin();
  // Wake up MPU-6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0x00);
  Wire.endTransmission(true);
  
  // Force ±2g accelerometer
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C); Wire.write(0x00);
  Wire.endTransmission(true);
}

// --- Read Angle (With Offset + Anti-Spike) ---
float readCalibratedAngle() {
  // Read Y and Z accelerometer data
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3D); // Start at ACCEL_YOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 4, true);
  
  accelY = Wire.read() << 8 | Wire.read();
  accelZ = Wire.read() << 8 | Wire.read();
  
  // Axis 3 calculation (Y-) + offset
  float ay_g = -accelY / accelScale;
  float az_g = accelZ / accelScale;
  float rawAngle = abs(atan2(ay_g, az_g) * 180.0 / PI);
  float calibratedAngle = rawAngle + angleOffset; // Apply -10° offset
  
  // Ensure angle doesn't go negative (filter noise)
  return (calibratedAngle < 0) ? 0.0 : calibratedAngle;
}

// --- LCD Updates (Calibrated Angles) ---
void updateLCD_RealTime(float currentAngle) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Curr: "); lcd.print(currentAngle, 1); lcd.print("°");
  lcd.setCursor(0, 1);
  lcd.print("Peak: "); lcd.print(peakRampAngle, 1); lcd.print("°");
}

// --- LCD Final Display (Peak Angle) ---
void updateLCD_FinalPeak() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MAX Ramp Angle:");
  lcd.setCursor(0, 1);
  lcd.print(peakRampAngle, 1); lcd.print("° (Final)");
}

// --- Setup ---
void setup() {
  // Motor pins
  pinMode(enA, OUTPUT); pinMode(in1, OUTPUT); pinMode(in2, OUTPUT);
  pinMode(enB, OUTPUT); pinMode(in3, OUTPUT); pinMode(in4, OUTPUT);
  analogWrite(enA, motorSpeed); analogWrite(enB, motorSpeed);

  // LCD Backlight (ON)
  pinMode(lcdBacklightPin, OUTPUT);
  digitalWrite(lcdBacklightPin, HIGH);

  // LCD + MPU Init
  lcd.begin(16, 2);
  lcd.print("Init + Offset -10°");
  initMPU6050();
  delay(1000);
}

// --- Main Loop (Peak + Offset + Anti-Spike) ---
void loop() {
  peakRampAngle = 0.0; // Reset peak at start

  // 1. Move Forward (2.5s) → Track peak (ignore spikes)
  rightWheelsForward(); leftWheelsForward();
  unsigned long start = millis();
  while (millis() - start < 2500) {
    float currentAngle = readCalibratedAngle();
    // Update peak ONLY if current angle is:
    // - Higher than previous peak
    // - Not a spike (≤5° above peak → prevents momentum/collision spikes)
    if (currentAngle > peakRampAngle && currentAngle <= peakRampAngle + spikeThreshold) {
      peakRampAngle = currentAngle;
    }
    updateLCD_RealTime(currentAngle);
    delay(50);
  }

  // 2. Stop (4s) → Show calibrated angles
  stopAll();
  start = millis();
  while (millis() - start < 4000) {
    float currentAngle = readCalibratedAngle();
    updateLCD_RealTime(currentAngle);
    delay(50);
  }

  // 3. Turn 360° (2000ms) → Don't update peak
  rightWheelsBackward(); leftWheelsForward();
  start = millis();
  while (millis() - start < 1900) {
    float currentAngle = readCalibratedAngle();
    updateLCD_RealTime(currentAngle);
    delay(50);
  }

  // 4. Stop (0.5s)
  stopAll();
  start = millis();
  while (millis() - start < 500) {
    float currentAngle = readCalibratedAngle();
    updateLCD_RealTime(currentAngle);
    delay(50);
  }

  // 5. Move Forward (2.5s) → Track peak (ignore spikes)
  rightWheelsForward(); leftWheelsForward();
  start = millis();
  while (millis() - start < 2500) {
    float currentAngle = readCalibratedAngle();
    if (currentAngle > peakRampAngle && currentAngle <= peakRampAngle + spikeThreshold) {
      peakRampAngle = currentAngle;
    }
    updateLCD_RealTime(currentAngle);
    delay(50);
  }

  // 6. Final Stop → Display MAX calibrated angle
  stopAll();
  updateLCD_FinalPeak();

  // Halt forever
  while(true) {
    updateLCD_FinalPeak();
    delay(200);
  }
}
