/*
 * Project Space Cat
 * Created by Flop 08.04.18
 * License WTFPL http://www.wtfpl.net/
 * 
*/
#include <limits.h>
#include <AccelStepper.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>


#define STEPPER_STEP_PIN D1
#define STEPPER_DIR_PIN D2

#define STEPPER_MIN_END_PIN D3
#define STEPPER_MAX_END_PIN D4


long maxPos = 1;
long minPos = 0;


AccelStepper stepper(AccelStepper::DRIVER, STEPPER_STEP_PIN, STEPPER_DIR_PIN);

void calibrateStepper() {

  stepper.moveTo(LONG_MAX);
  while(digitalRead(STEPPER_MAX_END_PIN) == LOW) {
     stepper.run();
     yield();
  }
  maxPos = stepper.currentPosition();
  stepper.moveTo(-LONG_MAX);
  while(digitalRead(STEPPER_MIN_END_PIN) == LOW) {
     stepper.run();
     yield();
  }
  minPos = stepper.currentPosition();
}

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  Serial.begin(115200);
  pinMode(STEPPER_STEP_PIN, OUTPUT);
  pinMode(STEPPER_DIR_PIN, OUTPUT);  
  pinMode(STEPPER_MIN_END_PIN, INPUT_PULLUP);
  pinMode(STEPPER_MAX_END_PIN, INPUT_PULLUP);
  stepper.setMaxSpeed(800.0);
  stepper.setAcceleration(8000.0);  
  calibrateStepper();
}

//int pos = 0;

// the loop function runs over and over again forever
void loop() {

  while (Serial.available() > 0) {
    int pos = Serial.parseInt();
    if (Serial.read() == '\n') {
      Serial.print(pos);
      stepper.moveTo(pos);    
    }
  }

  stepper.run();
  yield();
}
