/*
 Basic ESP8266 MQTT example
 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off
 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/



#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "BedHandler.h"

BedHandler bed;
// Global pin interrupt flags (volatile because they are changed within the interrupt i.e. outside of the main loop)
// volatile bool bButtonPressedUp = false;
// volatile bool bButtonPressedDown = false;

// ISR Declarations
// void ICACHE_RAM_ATTR IsrUp();
// void ICACHE_RAM_ATTR IsrDown();
void ICACHE_RAM_ATTR printDot();

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
  timer1_attachInterrupt(printDot);
  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);
  //timer1_write(1000);

  // attachInterrupt(digitalPinToInterrupt(INPUT_UP), IsrUp, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(INPUT_DOWN), IsrDown, CHANGE);

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void ICACHE_RAM_ATTR printDot()
{
  if (digitalRead(INPUT_UP) == LOW)
    {
      if (!flag1)
        Serial.println("UP");
      digitalWrite(LED_BUILTIN, HIGH);
      flag1 = true;
    }
    else
    {
      digitalWrite(LED_BUILTIN, LOW);
      flag1 = false;
    }

    if (digitalRead(INPUT_DOWN) == LOW)
    {
      if (!flag2)
      Serial.println("DOWN");
      digitalWrite(LED_BUILTIN_AUX, HIGH);
      flag2 = true;
    }
    else
    {
      digitalWrite(LED_BUILTIN_AUX, LOW);
      flag2 = false;
    }
  timer1_write(2500000);
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

  
  HandleInputs(30);

  if (digitalRead(INPUT_POWER) == HIGH) //System OFF
    {
      //digitalWrite(LED_BUILTIN, LOW);
      //digitalWrite(LED_BUILTIN_AUX, HIGH);
    }

  else
  {
    //digitalWrite(LED_BUILTIN_AUX, LOW);
    //digitalWrite(LED_BUILTIN, HIGH);
  }  
}

// Buttons need to override network commands (For safety I guess?)
void HandleInputs(unsigned long debounceDelay) 
{
  if (bButtonPressedUp == false || bButtonPressedDown == false)
  {
    // This is only executed if both buttons are NOT pressed

    /************ UP BUTTON ************/
    if (millis() - lastUpPress > debounceDelay) // Debounce Delay
    {
      if (digitalRead(INPUT_UP) == LOW)
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
        if (bButtonPressedUp == true) // Because we don't want this to constantly be stopping the bed when it is trying to move automatically, only when moving via remote
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
      if (digitalRead(INPUT_DOWN) == LOW)
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
}

