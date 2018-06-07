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
#include <NeoPixelBus.h>

#define STEPPER_STEP_PIN D1
#define STEPPER_DIR_PIN D2

#define LED_PIN D4

#define STEPPER_MIN_END_PIN D5
#define STEPPER_MAX_END_PIN D6


#define MIN_END_ON (digitalRead(STEPPER_MIN_END_PIN)==LOW)
#define MAX_END_ON (digitalRead(STEPPER_MAX_END_PIN)==LOW)

#define MIN_END_OFF (digitalRead(STEPPER_MIN_END_PIN)==HIGH)
#define MAX_END_OFF (digitalRead(STEPPER_MAX_END_PIN)==HIGH)

#define min(a,b) (a<b ? a : b)



long maxPos = 1;
long minPos = 0;
long revSteps = 200;
long calibrationSpeed = 50;
long moveSpeed = 200;
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

//led
NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> led(1, LED_PIN);
#define colorSaturation 128
//define some colors
RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor violet(colorSaturation, 0, colorSaturation);
RgbColor black(0);

HslColor hslRed(red);
HslColor hslGreen(green);
HslColor hslBlue(blue);
HslColor hslWhite(white);
HslColor hslBlack(black);

template<typename T_COLOR_FEATURE> void ledShow(typename T_COLOR_FEATURE::ColorObject color) {
  led.SetPixelColor(0, color);
  led.Show();
}


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
  stepper.setMaxSpeed(calibrationSpeed);

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

  moveRockerToTargetPos(0);
  
  while(stepper.distanceToGo()) {
     yield();
     stepper.run();
  }
  
  stepper.setMaxSpeed(moveSpeed);
  
  delay(100);
  Serial.println(" ... ");
}

void doConfiguration() {
  Serial.print("Configuration.");
  while (true) {
    ledShow<NeoGrbFeature>(violet);
    delay(200);
    ledShow<NeoGrbFeature>(black);    
    delay(200);    
    Serial.print("*");
  }
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
  char buf[200];
  memset(buf,0,200);
  memcpy(buf, payload, min(198,length));
  
  char *ptr;
  long pos = strtol(buf, &ptr, 10); 
  
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



//
//  MAIN SETUP FUCTION
//

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("RockerLaucher");
  
  pinMode(STEPPER_STEP_PIN, OUTPUT);
  pinMode(STEPPER_DIR_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);  
  pinMode(STEPPER_MIN_END_PIN, INPUT_PULLUP);
  pinMode(STEPPER_MAX_END_PIN, INPUT_PULLUP);

  led.Begin();
  ledShow<NeoGrbFeature>(red);
  delay(100);
  ledShow<NeoGrbFeature>(blue);
  delay(100);
  ledShow<NeoGrbFeature>(red);
  delay(100);
  ledShow<NeoGrbFeature>(blue);
  delay(100);
  ledShow<NeoGrbFeature>(black);
  
  if (MIN_END_ON && MIN_END_ON ) {
    delay(100);
    doConfiguration();
  }
  
  stepper.setMinPulseWidth(100);
  stepper.setMaxSpeed(moveSpeed);
  stepper.setAcceleration(3200.0);  

  calibrateStepper();

  ledShow<NeoGrbFeature>(white);
  delay(100);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  ledShow<NeoGrbFeature>(black);  
  
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

  long d = stepper.distanceToGo();
  if (d > 0) {
    ledShow<NeoGrbFeature>(blue);
  } else if ( d < 0) {
    ledShow<NeoGrbFeature>(red);
  } else {
    ledShow<NeoGrbFeature>(black);
  }

  if (MIN_END_ON && MIN_END_ON ) {
    delay(100);
    doConfiguration();
  } else if ( MIN_END_ON || MIN_END_ON ) {
    delay(100);
    Serial.println("Calibrate again ! ");
    calibrateStepper();
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();  
}
