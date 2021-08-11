/*
 ESP8266 MQTT Bed Project

 NOTE: It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
*/



#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "BedHandler.h"
#include "credentials.h"

BedHandler bed;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, -14400); //EST

// ISR Declarations
// void ICACHE_RAM_ATTR IsrUp();
// void ICACHE_RAM_ATTR IsrDown();
//void ICACHE_RAM_ATTR printDot();

// Pin definitions for the "application" side of things (BedHandler takes care of the details of actually moving i.e. outputs)
#define INPUT_UP D5
#define INPUT_DOWN D6
#define INPUT_POWER 9

void HandleInputs(unsigned long);
unsigned long lastUpPress = 0;
unsigned long lastDownPress = 0;
bool bButtonPressedUp = false;
bool bButtonPressedDown = false;
bool flag1, flag2; // Used for serial output debugging of button presses

int reconnectTime = 0;

int alarmHour = 0;
int alarmMin = 0;
bool alarmAm = true;

WiFiClient espClient;
PubSubClient client(espClient);
//unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
//int value = 0;

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
    double tmp = millis();
    for (int i = 0; i < 10; i++)
    {
      delay(50);
      HandleInputs(50);
    }
    Serial.print("Time was: " + String(millis() - tmp));
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();

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

  /********** Fetch values ***********/

  // Do checks for 1. Direction, 2. Modifier 3. Amount -OR- 1. Calibrate
    // 1. Direction: UP, DOWN
    // 2. Modifier: 
    //      Time: "5 seconds", etc (Cap to abount 25-30 sec)
    //      Amount: 50%, 100%, "All the way", "All the way up", etc
    // 3. Calibrate: Take about a minute, put bed down and calibrate to that, then up

  /*Bed Movement*/
  if (doc["data"].containsKey("move"))
  {
    const char* direction = doc["data"]["move"]["dir"];
    const char* tempMod = doc["data"]["move"]["mod"];
    uint8_t amount = doc["data"]["move"]["amt"];
    
    // Change modifier lowercase (Google Assistant sometimes randomly capitalizes)
    char* mod = strdup(tempMod); // const char* -> char* (mod stored in heap)
    unsigned char* umod = (unsigned char*)mod; // bc tolower() behavior is undefined if arg is not unsigned char
    for(uint8_t i = 0; i < sizeof(umod)/sizeof(umod[0]); i++) {
      umod[i] = tolower(umod[i]);
    }
    const char* modifier = mod;
    //yes I know I could've use Arduino's String class to make life easier but that's no fun
  
    // Print values.
    Serial.printf("Direction: %s\n", direction);
    Serial.printf("Modifier: %s\n", modifier);
    Serial.printf("Amount: %u\n", amount);

    //Casting to const char* should be okay, because a string literal is just a const char* anyways
    if (strcmp(modifier, (const char*)"seconds") == 0 || strcmp(modifier, (const char*)"second") == 0)
    {
      if (amount > 30) amount = 30;
      if (amount < 0) amount = 0;

      if (strcmp(direction, (const char*)"up") == 0)
      {
        bed.Move_Automatic(BedHandler::UP, BedHandler::SECONDS, amount);
      }
      else if (strcmp(direction, (const char*)"down") == 0)
      {
        bed.Move_Automatic(BedHandler::DOWN, BedHandler::SECONDS, amount);
      }
    }


    else if (strcmp(modifier, (const char*)"percent") == 0 || strcmp(modifier, (const char*)"%") == 0)
    {
      if (amount > 100) amount = 100;
      if (amount < 0) amount = 0;
      
      if (strcmp(direction, (const char*)"up") == 0)
      {
        //TBD
      }
      else if (strcmp(direction, (const char*)"down") == 0)
      {
        //TBD
      }
    }

    free(mod); // Important to avoid eventual memory leaks
  }

  /*Alarm Set*/
  else if (doc["data"].containsKey("alarm"))
  {
    // Based on format Google assistant uses to fill payload
    const char* time = doc["data"]["alarm"]["time"];
    char* mutableTime = strdup(time);
    char* token = strtok(mutableTime, ":");
    alarmHour = atoi(token); // string to int
        Serial.print(token);

    Serial.print(alarmHour);
        Serial.print(alarmHour);

    
    token = strtok (NULL, " ");
    Serial.println(token);

    free(mutableTime);
  }


  else if (doc["data"].containsKey("calibrate"))
  {
    // Calibrate the Accel/Gyro/DMP
  }
  
  

  

  

}

void reconnect() {
  // Loop until we're reconnected
  //while (!client.connected()) {
  if (millis() - reconnectTime > 5000) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe("IFTTT_Bed_with_RPi/ga", 1);
      client.subscribe("IFTTT_Bed_with_RPi/alarm", 1);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      //delay(5000);
      reconnectTime = millis();
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

  //bed.Init();
}



void loop() {

  if (!client.connected()) {
    reconnect();
  }
  else {
      client.loop();
  }

  timeClient.update();
  //Serial.println(timeClient.getFormattedTime());
  
  HandleInputs(50); // 50 Plenty for debouncing and minimum relay operation

  //bed.Update(); // For MPU related tasks

    
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

  if (digitalRead(INPUT_POWER) == HIGH) //System OFF
  {
    //ESP.reset();
  }

  else
  {

  }
}

