/*
 ESP8266 MQTT Bed Project

 NOTE: It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
*/



#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "BedHandler.h"

BedHandler bed;

// ISR Declarations
// void ICACHE_RAM_ATTR IsrUp();
// void ICACHE_RAM_ATTR IsrDown();
//void ICACHE_RAM_ATTR printDot();

void HandleInputs(unsigned long);
unsigned long lastUpPress = 0;
unsigned long lastDownPress = 0;
bool bButtonPressedUp = false;
bool bButtonPressedDown = false;
bool flag1, flag2; // Used for serial output debugging of button presses

// Network Values
const char* ssid = "Direction and ___";
const char* password = "Step Size";
//const char* ssid = "Smabs 2.4G";
//const char* password = "icon5662exit865row";
const char* mqtt_server = "mqtt.beebotte.com";
const char* mqtt_user = "token:token_GdXdLP595Cqirx0s";
const char* mqtt_pass = "";


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  bool tmp = false;
  while (WiFi.status() != WL_CONNECTED) {
    // Note: Need delay in this loop or ESP crashes

    // Alternate lights every 500ms to show we are connecting
    digitalWrite(LED_BUILTIN, tmp);
    digitalWrite(LED_BUILTIN_AUX, !tmp);
    tmp = !tmp;

    // Total delay of 500ms - Broken up into smaller chunks so that we can still poll for inputs
    for (int i = 0; i < 10; i++)
    {
      delay(50);
      HandleInputs(30);
    }
    
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Pulse lights to show connected
  for (int i = 0; i < 8; i++)
    {
      digitalWrite(LED_BUILTIN, LOW);
      digitalWrite(LED_BUILTIN_AUX, LOW);
      delay(80);
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(LED_BUILTIN_AUX, HIGH);
      delay(80);
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Deserialize the json payload
  StaticJsonDocument<200> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, payload);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  // Fetch values.
  //
  // Most of the time, you can rely on the implicit casts.
  // In other case, you can do doc["time"].as<long>();
  //const char* jsonData = doc["data"];

  const char* direction = doc["data"]["dir"];
  const char* modifier = doc["data"]["modifier"];
  unsigned int amount = doc["data"]["amt"];


  // Print values.
  Serial.printf("Direction: %s\n", direction);
  Serial.printf("Modifier: %s\n", modifier);
  Serial.printf("Amount: %u\n", amount);



  // Switch on the LED if an 1 was received as first byte of "data" object (simply test that byte)
  //if ((char)payload[9] == '1') {

  // Do checks for 1. Direction, 2. Modifier 3. Calibrate
  // 1. Direction: UP, DOWN
  // 2. Modifier: 
  //      Time: "5 seconds", etc (Cap to abount 25-30 sec)
  //      Amount: 50%, 100%, "All the way", "All the way up", etc
  // 3. Calibrate: Take about a minute, put bed down and calibrate to that, then up

  // If a new command comes in, make sure you stop current command and default to latest one

  //Casting to const char* should be okay, because a string literal is just a const char* anyways
  if (strcmp(direction, (const char*)"up") == 0)
  {
    if (strcmp(modifier, (const char*)"seconds") == 0 || strcmp(modifier, (const char*)"Seconds") == 0)
    {
      // Start bed movement now, end later via ticker without blocking code
      bed.Move_Automatic(BedHandler::UP, amount);
    }
    else if (strcmp(modifier, (const char*)"percent") == 0 || strcmp(modifier, (const char*)"%") == 0)
    {
      //TBD
    }


  }
  
  else if (strcmp(direction, (const char*)"down") == 0)
  {
    if (strcmp(modifier, (const char*)"seconds") == 0 || strcmp(modifier, (const char*)"Seconds") == 0)
    {
      bed.Move_Automatic(BedHandler::DOWN, amount);
    }
    else if (strcmp(modifier, (const char*)"percent") == 0 || strcmp(modifier, (const char*)"%") == 0)
    {
      //TBD
    }
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("IFTTT_Bed_with_RPi/ga", 1);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}




void setup() {

  // Pin Modes
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_BUILTIN_AUX, OUTPUT);

  pinMode(OUTPUT_UP, OUTPUT);
  pinMode(OUTPUT_DOWN, OUTPUT);

  pinMode(INPUT_UP, INPUT_PULLUP);
  pinMode(INPUT_DOWN, INPUT_PULLUP);

  pinMode(INPUT_POWER, INPUT);

  // Initialize LED's to be off
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_BUILTIN_AUX, HIGH);

  // Initialize Timer
  //timer1_attachInterrupt(printDot);
  //timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);
  //timer1_write(1000);

  // attachInterrupt(digitalPinToInterrupt(INPUT_UP), IsrUp, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(INPUT_DOWN), IsrDown, CHANGE);

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}



void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    /*snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("outTopic", msg);*/
  }

  
  HandleInputs(50); // 50 Plenty for debouncing and minimum relay operation

  if (digitalRead(INPUT_POWER) == HIGH) //System OFF
  {

  }

  else
  {

  }  
}

// Buttons need to override network commands (For safety I guess?)
// Debounce delay is ALSO for ensuring the relay achieves its full operating time before we potentially switch it off again
// EX. With the way the system is now, if both buttons are pressed at the same time, it will start by turning on the output of the corresponding input it received first,
//     then very shortly after when recognizing both buttons are pressed, turn OFF both relays (Stop()). This is just to make sure we are not operating at to high a switching "frequency"
// This is (probably not) important because originally with the remote directly connected, when both buttons were pressed, both relays would be on. So the fastest possible switching "frequency" would be
//     whatever speed the user could toggle the button. This is obviously much longer than its operating time, but with a uC controlling it now, I just want to make sure we are not abusing the relay's timing characteristics.
// debounceDelay is obviously for button debouncing too!
void HandleInputs(unsigned long debounceDelay) 
{

  /************ UP BUTTON ************/
  if (millis() - lastUpPress > debounceDelay) // Debounce Delay
  {
    if (digitalRead(INPUT_UP) == LOW && digitalRead(INPUT_DOWN) == HIGH)
    {
      bButtonPressedUp = true;
      if (!flag1)
        Serial.println("UP");
      bed.Move_Manual(BedHandler::UP);
      flag1 = true;
      lastUpPress = millis();
    }
    else
    {
      if (bButtonPressedUp == true) // Because we don't want this to constantly be stopping the bed when it is trying to move automatically, only after remote button is pressed
      {
        bButtonPressedUp = false;
        bed.Stop();
        flag1 = false;
      }
    }
  }

  /************ DOWN BUTTON ************/
  if (millis() - lastDownPress > debounceDelay) // Debounce Delay
  {
    if (digitalRead(INPUT_DOWN) == LOW && digitalRead(INPUT_UP) == HIGH)
    {
      bButtonPressedDown = true;
      if (!flag2)
        Serial.println("DOWN");
      bed.Move_Manual(BedHandler::DOWN);
      flag2 = true;
      lastDownPress = millis();
    }
    else
    {
      if (bButtonPressedDown == true)
      {
        bButtonPressedDown = false;
        bed.Stop();
        flag2 = false;
      }
    }
  }
}

