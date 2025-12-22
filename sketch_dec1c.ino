// --- Motor Pin Definitions (match your wiring) ---
int enA = 0;    // Right wheel PWM
int in1 = 12;   // Right wheel direction 1
int in2 = 13;   // Right wheel direction 2
int enB = 11;   // Left wheel PWM
int in3 = 3;    // Left wheel direction 1
int in4 = 2;    // Left wheel direction 2
const int motorSpeed = 100; // Adjust speed (0-255)

// --- HC-SR04 Sensor Pins ---
const int trigPin = A4;  // Trigger (output)
const int echoPin = A5;  // Echo (input)
const float stopDistance = 15.0; // Stop if obstacle <10cm (requested)

// --- Motor Control Functions ---
void driveForward() {
  // Right wheels forward
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  // Left wheels forward
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  // Apply motor speed
  analogWrite(enA, motorSpeed);
  analogWrite(enB, motorSpeed);
}

void stopTemporarily() {
  // Stop motors (but allow restart later)
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  analogWrite(enA, 0);
  analogWrite(enB, 0);
}

// --- Measure Distance with HC-SR04 ---
float getDistance() {
  // Send 10us ultrasonic trigger pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo duration (timeout after 20ms = max ~3.4m)
  long echoTime = pulseIn(echoPin, HIGH, 20000);

  // Calculate distance (speed of sound = 343m/s)
  float distance = (echoTime * 0.0343) / 2; // Divide by 2 (round trip)

  // Filter invalid readings (HC-SR04 range: 2cm-400cm)
  return (distance < 2 || distance > 400) ? 500.0 : distance;
}

// --- Setup ---
void setup() {
  // Initialize motor pins as outputs
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  // Initialize HC-SR04 pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

// --- Main Loop: Core Logic (Stop Only When Obstacle Is Present) ---
void loop() {
  // Constantly measure distance
  float currentDistance = getDistance();

  // Decision Logic (exactly as requested):
  if (currentDistance < stopDistance) {
    // Obstacle detected → STOP
    stopTemporarily();
  } else {
    // No obstacle → DRIVE FORWARD
    driveForward();
  }

  // Small delay to stabilize sensor readings (prevents jitter)
  delay(30);
}