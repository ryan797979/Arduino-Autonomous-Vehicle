// ------------ PIN SETUP --------------
int enA = 0; //temp
int in1 = 12;
int in2 = 13;

int enB = 11;
int in3 = 3;
int in4 = 2;

// IR sensors
int midIR = A1;
int homeIR = A2;
int leftIR = A3;

void setup() {

  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  pinMode(midIR, INPUT);
  pinMode(homeIR, INPUT);
  pinMode(leftIR, INPUT);

  analogWrite(enA, 120);
  analogWrite(enB, 120);

  
}

// ---------------- MOTOR FUNCTIONS -----------------

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



// ---------------- MAIN LOOP -----------------

void loop() {

  int midVal = digitalRead(midIR);   // 0 = black
  int homeVal = digitalRead(homeIR); // 1 = black
  int leftVal = digitalRead(leftIR); // 0 = black

  
  // ----------- BEHAVIOR LOGIC ----------------

  if (homeVal == 0 && leftVal == 1 && midVal == 1) {
    stopAll(); 
    delay(3000);
  }

  if (midVal == 1 && homeVal == 0 && leftVal == 0) {
    leftWheelsForward();
    rightWheelsForward();
    delay(350);
    leftWheelsForward();
    rightWheelsBackward();
    delay(900);
  }

  // Left IR detects → left wheels backward
  if (leftVal == 1) {
    leftWheelsBackward();
    rightWheelsForward();
    return;
  }

  // Homemade IR detects → turn right
  else if (homeVal == 0) {
    leftWheelsForward();
    rightWheelsBackward();
  }

  // Otherwise go straight
  else {
    rightWheelsForward();
    leftWheelsForward();
  }
}
