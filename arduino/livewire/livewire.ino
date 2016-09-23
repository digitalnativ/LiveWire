#include <TimerOne.h>

#define EXTRUDER_EN 6
#define EXTRUDER_DIR 4
#define EXTRUDER_STEP 5

#define BENDER_EN 9
#define BENDER_DIR 7
#define BENDER_STEP 8

#define LIMIT 2

#define SOLENOID 3

#define COMMAND_BUFFER 110
#define BENDER_HOME_OFFSET 0

volatile long extruderStep = 0;
static unsigned long extruderDelay = 10000;

volatile long benderStep = 0;
static unsigned long benderDelay = 1000;
volatile boolean bender = false;

boolean messageReady = false;
static String message = "";

static String command[COMMAND_BUFFER] = {""};
static boolean noCommand = true;
static unsigned int currentCommandBuffer = 0;
static unsigned int messageCommandBuffer = 0;

boolean homePosition = false;
unsigned long limitSwitch1Time = 0;

void solenoidOn() {
  digitalWrite(SOLENOID, HIGH);
  delay(100);
}

void solenoidOff() {
  digitalWrite(SOLENOID, LOW);
  delay(100);
}

void stepper() {
  if (!noCommand) {
    //Extruder
    if (!bender) {
      if (extruderStep > 0) {
        digitalWrite(EXTRUDER_DIR, LOW);
        digitalWrite(EXTRUDER_STEP, digitalRead(EXTRUDER_STEP) ^ 1);
        extruderStep--;
      }
      else if (extruderStep < 0) {
        digitalWrite(EXTRUDER_DIR, HIGH);
        digitalWrite(EXTRUDER_STEP, digitalRead(EXTRUDER_STEP) ^ 1);
        extruderStep++;
      }
      else {
        bender = true;
        digitalWrite(EXTRUDER_EN, HIGH);
        if (benderStep == 0) {
          noCommand = true;
        }
        Timer1.initialize(benderDelay);
      }
    }

    //Bender
    if (bender) {
      if (benderStep > 0) {
        digitalWrite(BENDER_DIR, LOW);
        digitalWrite(BENDER_STEP, digitalRead(BENDER_STEP) ^ 1);
        benderStep--;
      }
      else if (benderStep < 0) {
        digitalWrite(BENDER_DIR, HIGH);
        digitalWrite(BENDER_STEP, digitalRead(BENDER_STEP) ^ 1);
        benderStep++;
      }
      else {
        bender = false;
        digitalWrite(BENDER_EN, HIGH);
        if (extruderStep == 0) {
          noCommand = true;
        }
        Timer1.initialize(extruderDelay);
      }
    }
  }
}

void executeMessage() {
  if (message == "?") {
    Serial.print("X");
    Serial.print(extruderStep);
    Serial.print(" Y");
    Serial.println(benderStep);
  }

  message = "";
  messageReady = false;
}

void executeCommand() {
  if (command[currentCommandBuffer] != "") {
    Serial.println(command[currentCommandBuffer]);
    if (command[currentCommandBuffer].charAt(0) == 'X') { //extruder
      uint8_t i = 1;
      String stepString = "";
      if (command[currentCommandBuffer].charAt(1) == '-') {
        i = 2;
        stepString = "-";
      }
      for (i; i < command[currentCommandBuffer].length(); i++) {
        if (isDigit(command[currentCommandBuffer].charAt(i))) {
          stepString += command[currentCommandBuffer].charAt(i);
        }
        else {
          break;
        }
      }
      if (i == command[currentCommandBuffer].length()) {
        digitalWrite(EXTRUDER_EN, LOW);
        extruderStep += stepString.toInt();
      }
      else {
        Serial.println("Error command");
      }
    }
    else if (command[currentCommandBuffer].charAt(0) == 'Y') { //bender
      uint8_t i = 1;
      String stepString = "";
      if (command[currentCommandBuffer].charAt(1) == '-') {
        i = 2;
        stepString = "-";
      }
      for (i; i < command[currentCommandBuffer].length(); i++) {
        if (isDigit(command[currentCommandBuffer].charAt(i))) {
          stepString += command[currentCommandBuffer].charAt(i);
        }
        else {
          break;
        }
      }
      if (i == command[currentCommandBuffer].length()) {
        digitalWrite(BENDER_EN, LOW);
        benderStep += stepString.toInt();
      }
      else {
        Serial.println("Error command");
      }
    }
    else if (command[currentCommandBuffer].charAt(0) == 'D') { //delay
      uint8_t i = 1;
      String timeString = "";
      for (i; i < command[currentCommandBuffer].length(); i++) {
        if (isDigit(command[currentCommandBuffer].charAt(i))) {
          timeString += command[currentCommandBuffer].charAt(i);
        }
        else {
          break;
        }
      }
      if (i == command[currentCommandBuffer].length()) {
        delay(timeString.toInt());
      }
      else {
        Serial.println("Error command");
      }
    }
    else if (command[currentCommandBuffer] == "S0") {
      solenoidOff();
    }
    else if (command[currentCommandBuffer] == "S1") {
      solenoidOn();
    }

    noCommand = false;
    command[currentCommandBuffer] = "";
    currentCommandBuffer++;
    if (currentCommandBuffer > COMMAND_BUFFER) {
      currentCommandBuffer = 0;
    }
  }
}

void serialEvent() {
  while (Serial.available()) {
    if (messageReady) {
      message = "";
      messageReady = false;
    }
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      if (message.charAt(0) == 'X' ||
          message.charAt(0) == 'Y' ||
          message.charAt(0) == 'S' ||
          message.charAt(0) == 'D') {
        command[messageCommandBuffer] = message;
        messageCommandBuffer++;
        if (messageCommandBuffer > COMMAND_BUFFER) {
          messageCommandBuffer = 0;
        }
        message = "";
        messageReady = false;
      }
      else {
        messageReady = true;
      }
    }
    else {
      message += inChar;
    }
  }
}



void limitSwitched() {
  benderStep = 0;
  digitalWrite(BENDER_EN, LOW);
  digitalWrite(BENDER_DIR, LOW);
  while (digitalRead(LIMIT)) {
    digitalWrite(BENDER_STEP, HIGH); // Output high
    delay(10); // Wait
    digitalWrite(BENDER_STEP, LOW); // Output low
    delay(10); // Wait
  }
  int i;
  for(i = BENDER_HOME_OFFSET; i > 0; i--) {
    digitalWrite(BENDER_STEP, HIGH); // Output high
    delay(10); // Wait
    digitalWrite(BENDER_STEP, LOW); // Output low
    delay(10); // Wait
  }
  
  homePosition = true;
  digitalWrite(BENDER_EN, HIGH);
  Serial.println("Limited");
  delay(1000);
}

void setup() {
  pinMode(EXTRUDER_EN, OUTPUT);
  pinMode(EXTRUDER_DIR, OUTPUT);
  pinMode(EXTRUDER_STEP, OUTPUT);

  pinMode(BENDER_EN, OUTPUT);
  pinMode(BENDER_DIR, OUTPUT);
  pinMode(BENDER_STEP, OUTPUT);


  pinMode(LIMIT, INPUT);
  attachInterrupt(digitalPinToInterrupt(LIMIT), limitSwitched, RISING);

  pinMode(SOLENOID, OUTPUT);

  digitalWrite(EXTRUDER_EN, HIGH);
  digitalWrite(BENDER_EN, HIGH);

  Timer1.attachInterrupt(stepper);
  Timer1.initialize(extruderDelay);
  Serial.begin(115200);
}


void loop() {
  if (messageReady) {
    executeMessage();
  }
  if (noCommand) {
    executeCommand();
  }
}

