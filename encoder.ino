// --- Pin Definitions ---
// Motors
int enA = 0;    // PWM for right wheels
int in1 = 12;   // Right wheel direction 1
int in2 = 13;   // Right wheel direction 2
int enB = 11;   // PWM for left wheels
int in3 = 3;    // Left wheel direction 1
int in4 = 2;    // Left wheel direction 2

// Encoders
int encoderPinA = A0; // Encoder A pin (right motor) - ANALOG PIN
int encoderPinB = 10; // Encoder B pin (right motor) - DIGITAL PIN

// --- IR Sensors ---
int midIR = A1;  // 0 = black
int homeIR = A2; // 1 = black
int leftIR = A3; // 0 = black

// --- Distance Tracking Variables ---
float distanceTraveled = 0.0;
const float wheelDiameter = 0.065;
const int pulsesPerRevolution = 20;
float distancePerPulse;
volatile long encoderPulses = 0;
unsigned long lastEncoderUpdate = 0;
const unsigned long encoderUpdateInterval = 50; // Adjust as needed

const int motorSpeed = 100;

// --- Motor Functions ---
void rightWheelsForward() {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
}
void rightWheelsBackward() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
}
void leftWheelsForward() {
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}
void leftWheelsBackward() {
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}
void stopAll() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

// --- Encoder ISR ---
void encoderEvent() { encoderPulses++; }

void setup() {
  // Motor control pins
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  // IR sensor pins
  pinMode(midIR, INPUT);
  pinMode(homeIR, INPUT);
  pinMode(leftIR, INPUT);

  // Encoder pins
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderPinB), encoderEvent, RISING);

  // Calculate distance per pulse
  distancePerPulse = (PI * wheelDiameter) / pulsesPerRevolution;

  // Set initial motor speeds
  analogWrite(enA, motorSpeed);
  analogWrite(enB, motorSpeed);

  Serial.begin(9600); // For debugging
}

void loop() {
  // Line Following Logic
  int midVal = digitalRead(midIR);
  int homeVal = digitalRead(homeIR);
  int leftVal = digitalRead(leftIR);

  if (midVal == 1 && homeVal == 0 && leftVal == 0) {
    stopAll();
    delay(300);
    leftWheelsForward();
    rightWheelsForward();
    delay(300);
    leftWheelsForward();
    rightWheelsBackward();
    delay(600);
  } else if (leftVal == 1) {
    leftWheelsBackward();
    rightWheelsForward();
  } else if (homeVal == 0) {
    leftWheelsForward();
    rightWheelsBackward();
  } else {
    rightWheelsForward();
    leftWheelsForward();
  }

  // Encoder-Based Distance Tracking
  unsigned long currentTime = millis();
  if (currentTime - lastEncoderUpdate >= encoderUpdateInterval) {
    noInterrupts(); // Disable interrupts during calculations
    long pulseCount = encoderPulses;
    encoderPulses = 0; // Reset count
    interrupts();      // Re-enable interrupts

    float distanceChange = pulseCount * distancePerPulse;
    distanceTraveled += distanceChange;
    lastEncoderUpdate = currentTime;

    Serial.print("Distance: ");
    Serial.print(distanceTraveled);
    Serial.println(" m");
  }
}
