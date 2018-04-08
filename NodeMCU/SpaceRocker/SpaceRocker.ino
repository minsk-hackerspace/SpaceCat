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
long revSteps = 400;
long targetPos = 0;

const char* ssid = "hackerspace";
const char* password = "hs_pass_0";
const char* mqtt_server = "178.172.172.103";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

AccelStepper stepper(AccelStepper::DRIVER, STEPPER_STEP_PIN, STEPPER_DIR_PIN);

//pos 0..10000
long moveRockerToTargetPos(long pos) {
    targetPos = pos;

    if (targetPos < 0) {
      targetPos = 0;
    }
    
    if (targetPos > 10000) {
      targetPos = 10000;
    }
    
    Serial.println("-");
    Serial.print("Moving Rocket to Rpos: ");
    Serial.print(targetPos);

    long tpos = map(pos, 0, 10000, (minPos+revSteps), (maxPos-revSteps));
    
    stepper.moveTo(tpos);

    Serial.print(" Spos: ");
    Serial.println(stepper.targetPosition());

    Serial.println("-");

    return stepper.targetPosition();
}

void calibrateStepper() {
  
  Serial.println("Release ends...");

  //release high end
  stepper.setCurrentPosition(LONG_MAX);
  stepper.moveTo(0);
  while(digitalRead(STEPPER_MAX_END_PIN) == LOW) {
     yield();
     stepper.run();
  }
  
  //release low end
  stepper.setCurrentPosition(1-LONG_MAX);
  stepper.moveTo(0);
  while(digitalRead(STEPPER_MIN_END_PIN) == LOW) {
     yield();
     stepper.run();
  }

  Serial.println("Calibrating...");
  stepper.setCurrentPosition(0);
  
  stepper.moveTo(LONG_MAX);
  Serial.print("goto ");
  Serial.println(stepper.targetPosition());
  while(digitalRead(STEPPER_MAX_END_PIN) == HIGH) {
     yield();
     stepper.run();
  }
  maxPos = stepper.currentPosition();
  
  stepper.moveTo(0);
  Serial.print("goto ");
  Serial.println(stepper.targetPosition());
  while(digitalRead(STEPPER_MIN_END_PIN) == HIGH && stepper.distanceToGo()) {
     yield();
     stepper.run();
  }
  
  stepper.moveTo(1-LONG_MAX);
  Serial.print("goto ");
  Serial.println(stepper.targetPosition());
  while(digitalRead(STEPPER_MIN_END_PIN) == HIGH) {
     yield();
     stepper.run();
  }
  
  minPos = stepper.currentPosition();
  Serial.println(" ... ");
  Serial.print("Calibration done: max ");
  Serial.print(maxPos);     
  Serial.print(" min ");
  Serial.println(minPos);     
  Serial.println(" ... ");

  delay(200);
  moveRockerToTargetPos(0);
  delay(200);
  Serial.println(" ... ");
}


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.setOutputPower(0);
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    randomSeed(micros());  
    //WiFi.begin(ssid, password);
  }


  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  Serial.println();
  
  char *ptr;
  long pos = strtol((const char*)payload, &ptr, 10); 
  
  Serial.println(pos);
  moveRockerToTargetPos(pos);

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "SpaceCat";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("cat/wards", "hello world");
      // ... and resubscribe
      client.subscribe("voting/pos");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(2000);
    }
  }
}

// setup
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("RockerLaucher");
  
  pinMode(STEPPER_STEP_PIN, OUTPUT);
  pinMode(STEPPER_DIR_PIN, OUTPUT);  
  pinMode(STEPPER_MIN_END_PIN, INPUT_PULLUP);
  pinMode(STEPPER_MAX_END_PIN, INPUT_PULLUP);
  stepper.setMaxSpeed(800.0);
  stepper.setAcceleration(8000.0);  
  calibrateStepper();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
    
}

// the loop function runs over and over again forever
void loop() {

  while (Serial.available() > 0) {
    int pos = Serial.parseInt();
    if (Serial.read() == '\n') {
      moveRockerToTargetPos(pos);
    }
  }

  stepper.run();
  
  if (digitalRead(STEPPER_MIN_END_PIN) == LOW || digitalRead(STEPPER_MAX_END_PIN) == LOW ) {
    delay(100);
    Serial.println("Calibrate again ! ");
    delay(100);
    calibrateStepper();
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();  
}
