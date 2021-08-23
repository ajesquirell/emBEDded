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
#include <queue>
#include <stdlib.h>
#include <string.h>

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

unsigned long lastUpPress = 0;
unsigned long lastDownPress = 0;
bool bButtonPressedUp = false;
bool bButtonPressedDown = false;

int reconnectTime = 0;

int alarmHour = 0;
int alarmMin = 0;
bool alarmAm = true;
bool bAlarmSet = false;
bool bAlarmActivated = false;
void HandleInputs(unsigned long);
void HandleAlarm();

bool bWaitForCalibration = false;


WiFiClient espClient;
PubSubClient client(espClient);
//unsigned long lastMsg = 0;
//#define MSG_BUFFER_SIZE	(256)
//char msg[MSG_BUFFER_SIZE];
//int value = 0;

// Alarm queue for bed commands
std::queue<void(*)()> alarmQueue; // Queue of function pointers - Allows us to call ANY FUNCTION when alarm goes off :D
                                  // This works too --> std::queue<std::function<void()>> alarmQueue;

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
    //Serial.print("Time was: " + String(millis() - tmp));
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
        bed.Move_Automatic(BedHandler::UP, BedHandler::PERCENT, amount);
      }
      else if (strcmp(direction, (const char*)"down") == 0)
      {
        bed.Move_Automatic(BedHandler::DOWN, BedHandler::PERCENT, amount);
      }
    }

    free(mod); // Important to avoid eventual memory leaks
  }

  /*Alarm Set*/
  else if (doc["data"].containsKey("alarm") && doc["data"]["alarm"]["active"] == true)
  {
    bAlarmSet = true;

    // Reset Alarm to default (should be able to delete later when all cases handled)
    alarmHour = 0;
    alarmMin = 0;
    alarmAm = true;
    bool bMinutesGiven = false;
    bool bAmPmGiven = false;
    // Based on format Google assistant uses to fill payload
    const char* time = doc["data"]["alarm"]["time"];
    char* mutableTime = strdup(time);
    char* token = strtok(mutableTime, " ");
    for (int i = 0; token != NULL; i++) // While loop with index
    {
      if (i == 0) // Get hour
      {
        alarmHour = atoi(token); // string to int
      }
      else if (i == 1) // "10 : 30..." OR "10 a . m ."     --> Either ": Min" OR am/pm
      {
        if (*token == ':') // "10 : 30..."                 --> Minutes were given, get them next loop
        {
          bMinutesGiven = true;
        }
        else // "10 a . m ."                               --> Minutes not given - Set alarm on the hour, just get am/pm
        {
          alarmMin = 0;
          if (*token == 'a') {
            bAmPmGiven = true;
            alarmAm = true;
          }
          else if (*token == 'p')  {
            bAmPmGiven = true;
            alarmAm = false;
          }
        }
      }
      else if (i == 2 && bMinutesGiven == true) // "10 : 30..."
      {
        alarmMin = atoi(token);
      }
      else if (i == 3) // "10 : 30 a . m ."
      {
        if (*token == 'a') {
          bAmPmGiven = true;
          alarmAm = true;
        }
        else if (*token == 'p') {
          bAmPmGiven = true;
          alarmAm = false;
        }
      }
      else if (i >= 4)
      {
        break;
      }

      Serial.printf("i = %u\n", i);
      token = strtok (NULL, " ");
    }

    // Process hour to 24hr time
    // If am/pm given, use that. If not, use soonest (mimic Google behavior)
    if (bAmPmGiven)
    {
      alarmHour %= 12;
      if (!alarmAm)
        alarmHour += 12;
    }
    else
    {
      // Greedily get the first upcoming hour specified
      alarmHour %= 12;
      const float fCurrentTime = (float)timeClient.getHours() + ((float)timeClient.getMinutes() / 60.0f);
      const float fAlarmTime = (float)alarmHour + ((float)alarmMin / 60.0f); // Just for comparison. In 12 hour time

      // 1 2 3 4 5 6 7 8 9 *10* 11 12 (13) 14 15 16 17 18 19 20 21 *22* 23 24
      if (fCurrentTime >= fAlarmTime && fCurrentTime < (fAlarmTime + 12.0f)) {
        alarmHour += 12;
      }
    }
    
    Serial.printf("Current Time: %s\n", timeClient.getFormattedTime().c_str());
    Serial.printf("Hour: %u\n", alarmHour);
    Serial.printf("Min: %u\n", alarmMin);
    Serial.printf("AM: %u", alarmAm);
    if (!bAmPmGiven) Serial.printf(" (Not Given)\n");

    free(mutableTime);
  }

  else if (doc["data"].containsKey("calibrate"))
  {
    // Calibrate the Accel/Gyro/DMP
    if (!bed.IsMoving() && !bed.IsCalibrating())
      bed.Calibrate();
    
    bWaitForCalibration = true;
  }

  else if (doc["data"].containsKey("calibration_data"))
  {
    bed.SetCalibrationValuesManually(doc["data"]["calibration_data"][0], doc["data"]["calibration_data"][1]);
    Serial.println("Calibration values set");
    Serial.println(bed.GetCalibrationValues().first);
    Serial.println(bed.GetCalibrationValues().second);
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
      client.subscribe("IFTTT_Bed_with_RPi/cal", 1);
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

  bed.Init();
}



void loop() {

  if (!client.connected()) {
    reconnect();
  }
  else {
    client.loop();
  }

  timeClient.update();
  
  HandleInputs(50); // 50 Plenty for debouncing and minimum relay operation

  HandleAlarm();

  bed.Update(); // For MPU related tasks

  if (!bed.IsCalibrating() && bWaitForCalibration) // Only happens for one loop when calibration finishes, so we know to update the resource with new calibration values
  {
    char msg[client.getBufferSize()];
    snprintf(msg, sizeof(msg), "{\"data\" : {\"calibration_data\" : [");
    strcat(msg, String(bed.GetCalibrationValues().first).c_str());
    strcat(msg, ", ");
    strcat(msg, String(bed.GetCalibrationValues().second).c_str());
    strcat(msg, "]}, \"write\" : true}");
    client.publish("IFTTT_Bed_with_RPi/cal", (const char*)msg);
    bWaitForCalibration = false;
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
      if (!bButtonPressedUp && !bAlarmActivated)
      {
        Serial.println("UP");
        bed.Move_Manual(BedHandler::UP);
      }

      if (!bButtonPressedUp && bAlarmActivated) // Idea is, when alarm is going off, to be able to stop alarm "on press" without also causing the bed to move on that one press
      {
        bAlarmActivated = false;
        bed.Stop();
      }

      
      bButtonPressedUp = true;
      lastUpPress = millis();
    }
    else
    {
      if (bButtonPressedUp == true) // Because we don't want this to constantly be stopping the bed when it is trying to move automatically, only after remote button is pressed
      {
        bButtonPressedUp = false;
        bed.Stop();
      }
    }
  }

  /************ DOWN BUTTON ************/
  if (millis() - lastDownPress > debounceDelay) // Debounce Delay
  {
    if (digitalRead(INPUT_DOWN) == LOW && digitalRead(INPUT_UP) == HIGH)
    {
      if (!bButtonPressedDown && !bAlarmActivated)
      {
        Serial.println("DOWN");
        bed.Move_Manual(BedHandler::DOWN);
      }
        
      if (!bButtonPressedDown && bAlarmActivated)
      {
        bAlarmActivated = false;
        bed.Stop();
      }

      
      bButtonPressedDown = true;
      lastDownPress = millis();
    }
    else
    {
      if (bButtonPressedDown == true)
      {
        bButtonPressedDown = false;
        bed.Stop();
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

void HandleAlarm()
{
  if (timeClient.getHours() == alarmHour && timeClient.getMinutes() == alarmMin && bAlarmSet == true)
  {
    bAlarmActivated = true;

    // Clear Alarm Queue
    while(!alarmQueue.empty())
    {
      Serial.println(alarmQueue.size());
      alarmQueue.pop();
    }

    // ==================== BED ALARM BEHAVIOR HERE ====================                                                                  --> Sure, I could've just made a cool (probably better looking) command struct to hold arguments
    // Add desired bed commands to alarmQueue                                                                                                 for bed.Move_Automatic(..), but function pointers are cool too...
    alarmQueue.push([] () {bed.Move_Automatic(BedHandler::UP, BedHandler::SECONDS, 8);});
    alarmQueue.push([] () {bed.Move_Automatic(BedHandler::DOWN, BedHandler::SECONDS, 8);});
    alarmQueue.push([] () {bed.Move_Automatic(BedHandler::UP, BedHandler::SECONDS, 8);});
    alarmQueue.push([] () {bed.Move_Automatic(BedHandler::DOWN, BedHandler::SECONDS, 8);});
    // ...
    // =================================================================
    
    bAlarmSet = false;
    client.publish("IFTTT_Bed_with_RPi/alarm", "{\"data\" : { \"alarm\" : { \"active\" : false}}, \"write\" : true}");
  }

  if (bAlarmActivated)
  {
    if (!bed.IsMoving()) // Way of making commands happen only after last one completes. Need to change if use functions that don't move bed
    {
      alarmQueue.front()(); // Call the command at beginning of queue
      alarmQueue.pop();
    }

    if (alarmQueue.empty())
    {
      bAlarmActivated = false;
    }
  }

}