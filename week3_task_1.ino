#include <LiquidCrystal.h>

// --- LCD Setup ---
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// --- Pin Definitions (YOURS) ---
// Motors
int enA = 0;    // PWM for right wheels
int in1 = 12;   // Right wheel direction 1
int in2 = 13;   // Right wheel direction 2
int enB = 11;   // PWM for left wheels
int in3 = 3;    // Left wheel direction 1
int in4 = 2;    // Left wheel direction 2

// --- IR Sensors ---
int midIR = A1;  // 0 = black
int homeIR = A2; // 1 = black
int leftIR = A3; // 0 = black

// --- Distance Tracking Variables ---
unsigned long distanceTimer = 0;  // Timer for distance updates
float distanceTraveled = 0.0;     // Total distance (m)
const float speedFactor = 0.0005; // Calibration factor (for distance tracking)
const int motorSpeed = 100;       // Motor speed (PWM)
const long distanceUpdateInterval = 50; // Update distance every 50ms

// --- Time-Based Line-Follow Stop (Calibrate This!) ---
const unsigned long lineFollowDuration = 10000; // Initial: 10s - TWEAK FOR 230cm!
bool hasCompletedTimedLineFollow = false;      // Run once
unsigned long lineFollowStartTime = 0;         // Track when line following started
unsigned long totalTravelTime = 0;             // Total time from start to end (ms)

// --- Fixed Distance Values (As Requested) ---
const float target230cm = 2.30;   // 230cm = 2.30m (display this at first stop)
const float total487cm = 4.87;    // 487cm = 4.87m (display this at final stop)

// --- Motor Functions (YOURS) ---
void rightWheelsForward() { digitalWrite(in1, HIGH); digitalWrite(in2, LOW); }
void rightWheelsBackward() { digitalWrite(in1, LOW); digitalWrite(in2, HIGH); }
void leftWheelsForward() { digitalWrite(in3, LOW); digitalWrite(in4, HIGH); }
void leftWheelsBackward() { digitalWrite(in3, HIGH); digitalWrite(in4, LOW); }
void stopAll() { 
  digitalWrite(in1, LOW); digitalWrite(in2, LOW); 
  digitalWrite(in3, LOW); digitalWrite(in4, LOW); 
}

// --- Setup ---
void setup() {
  // Initialize pins
  pinMode(enA, OUTPUT); pinMode(in1, OUTPUT); pinMode(in2, OUTPUT);
  pinMode(enB, OUTPUT); pinMode(in3, OUTPUT); pinMode(in4, OUTPUT);
  pinMode(midIR, INPUT); pinMode(homeIR, INPUT); pinMode(leftIR, INPUT);

  // Set motor speed
  analogWrite(enA, motorSpeed);
  analogWrite(enB, motorSpeed);

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dist: 0.00 m");
  lcd.setCursor(0, 1);
  lcd.print("Time: 0.0 s");

  // Initialize timers
  distanceTimer = millis();
  lineFollowStartTime = millis(); // Start timing line following from boot
}

// --- Main Loop ---
void loop() {
  unsigned long currentTime = millis();
  totalTravelTime = currentTime - lineFollowStartTime; // Update total time

  int midVal = digitalRead(midIR);
  int homeVal = digitalRead(homeIR);
  int leftVal = digitalRead(leftIR);
  bool isMoving = true;  // Assume moving unless stopped

  // --- 1. Time-Based Stop (For 230cm) ---
  if (!hasCompletedTimedLineFollow) {
    if (currentTime - lineFollowStartTime >= lineFollowDuration) {
      stopAll();                          // Stop the robot
      hasCompletedTimedLineFollow = true; // Prevent repeat stops
      
      // Display FIXED 230cm (2.30m) and time traveled (in seconds)
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("230cm Reached!");
      lcd.setCursor(0, 1);
      lcd.print("D:");
      lcd.print(target230cm, 2); // Fixed 2.30m (230cm)
      lcd.print("m T:");
      lcd.print(totalTravelTime / 1000.0, 1); // Time in seconds (1 decimal)
      lcd.print("s");
      
      delay(2000); // Pause for 2 seconds
      
      // Restore normal LCD display (distance + time in seconds)
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Dist: ");
      lcd.print(distanceTraveled, 2);
      lcd.print(" m   ");
      lcd.setCursor(0, 1);
      lcd.print("Time: ");
      lcd.print(totalTravelTime / 1000.0, 1);
      lcd.print(" s   ");
    }
  }

  // --- 2. Final Stop (End of Line) ---
  if (homeVal == 0 && leftVal == 1 && midVal == 1) {
    stopAll(); 
    isMoving = false;  // Robot is stopped
    
    // Display FIXED total 487cm (4.87m) and total time (in seconds)
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Final Stop!");
    lcd.setCursor(0, 1);
    lcd.print("D:");
    lcd.print(total487cm, 2); // Fixed 4.87m (487cm)
    lcd.print("m T:");
    lcd.print(totalTravelTime / 1000.0, 1); // Total time in seconds
    lcd.print("s");
    
    while(true); // Halt forever (keep original behavior)
  }

  // --- YOUR EXACT LINE-FOLLOWING LOGIC (100% UNMODIFIED) ---
  if (midVal == 1 && homeVal == 0 && leftVal == 0) {
    stopAll();
    delay(300);
    leftWheelsForward();
    rightWheelsForward();
    delay(300);
    leftWheelsForward();
    rightWheelsBackward();
    delay(600);
  }
  if (leftVal == 1) {
    leftWheelsBackward();
    rightWheelsForward();
    return;  // Exit loop early to prioritize line following
  }
  if (homeVal == 0) {
    leftWheelsForward();
    rightWheelsBackward();
  }
  else {
    rightWheelsForward();
    leftWheelsForward();
  }

  // --- BACKGROUND DISTANCE TRACKING ---
  if (isMoving && (currentTime - distanceTimer >= distanceUpdateInterval)) {
    float elapsedTime = (currentTime - distanceTimer) / 1000.0;  // Seconds
    distanceTraveled += speedFactor * motorSpeed * elapsedTime;  // Update distance
    distanceTimer = currentTime;
    
    // Update LCD (normal display: distance + time in seconds)
    if (hasCompletedTimedLineFollow || (currentTime - lineFollowStartTime < lineFollowDuration)) {
      lcd.setCursor(0, 0);
      lcd.print("Dist: ");
      lcd.print(distanceTraveled, 2);
      lcd.print(" m   ");
      lcd.setCursor(0, 1);
      lcd.print("Time: ");
      lcd.print(totalTravelTime / 1000.0, 1);
      lcd.print(" s   ");
    }
  }

  delay(10);  // Small delay to stabilize sensor readings
}
