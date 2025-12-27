#include <SoftwareSerial.h>

// --- Pin Definitions ---
// Motors
int enA = 0;    // PWM for right wheels
int in1 = 12;   // Right wheel direction 1
int in2 = 13;   // Right wheel direction 2
int enB = 11;   // PWM for left wheels
int in3 = 3;    // Left wheel direction 1
int in4 = 2;    // Left wheel direction 2

// HC-05 Bluetooth Module
SoftwareSerial bluetooth(A5, A4); // RX, TX  (Connect HC-05 TX to Arduino pin A5, HC-05 RX to Arduino pin A4)

// --- Motor Functions ---
void rightWheelsForward() {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  analogWrite(enA, 255); //Full speed
}
void rightWheelsBackward() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  analogWrite(enA, 255); //Full speed
}
void leftWheelsForward() {
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  analogWrite(enB, 255); //Full speed
}
void leftWheelsBackward() {
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  analogWrite(enB, 255); //Full speed
}
void stopAll() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  analogWrite(enA, 0);
  analogWrite(enB, 0);
}

void setup() {
  // Motor control pins
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  // Stop the robot initially
  stopAll();

  // Bluetooth setup
  bluetooth.begin(9600);
  Serial.begin(9600); //For debugging
  Serial.println("Bluetooth ready!");
}

void loop() {
  if (bluetooth.available() > 0) {
    char command = bluetooth.read();
    Serial.print("Received: ");
    Serial.println(command);

    switch (command) {
      case 'F': // Move Forward
        rightWheelsForward();
        leftWheelsForward();
        break;
      case 'B': // Move Backward
        rightWheelsBackward();
        leftWheelsBackward();
        break;
      case 'L': // Turn Left
        rightWheelsForward();
        leftWheelsBackward();
        break;
      case 'R': // Turn Right
        rightWheelsBackward();
        leftWheelsForward();
        break;
      case 'S': // Stop
        stopAll();
        break;
      default:
        Serial.println("Invalid command.");
        break;
    }
  }
}
