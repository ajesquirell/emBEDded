//#include <Arduino.h>
/*#include <ESP8266WiFi.h>

void setup() {
  // put your setup code here, to run once:
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D5, INPUT_PULLUP);
  pinMode(D6, INPUT_PULLUP);

}

void loop() {
  // put your main code here, to run repeatedly:
  /*digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_BUILTIN_AUX, LOW);
  delay(2000);
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(LED_BUILTIN_AUX, HIGH);
  delay(2000);*/
/*
  // Testing inputs
  if (digitalRead(D5) == LOW)
    digitalWrite(D2, HIGH);
  else
    digitalWrite(D2, LOW);


  if (digitalRead(D6) == LOW)
    digitalWrite(D1, HIGH);
  else
    digitalWrite(D1, LOW);
}
*/

  
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

// Update these with values suitable for your network.

//const char* ssid = "Direction and ___";
//const char* password = "Step Size";
const char* ssid = "Smabs 2.4G";
const char* password = "icon5662exit865row";
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

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

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

  // Deserialize the json payload
  StaticJsonDocument<200> doc;

  //char json[] =
      //"{\"sensor\":\"gps\",\"time\":1351824120,\"data\":[48.756080,2.302038]}";

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
  const char* jsonData = doc["data"];

  // Print values.
  Serial.println(jsonData);

  // Switch on the LED if an 1 was received as first byte of "data" object (simply test that byte)
  //if ((char)payload[9] == '1') {

  //Casting to const char* should be okay, because a string literal is just a const char* anyways
  if (strcmp(jsonData, (const char*)"up") == 0) {
    for (int i = 0; i < 10; i++)
    {
      digitalWrite(D1, HIGH);
      delay(100);
      digitalWrite(D1, LOW);
      delay(100);
    }
  } else if (strcmp(jsonData, (const char*)"down") == 0) {
    for (int i = 0; i < 10; i++)
    {
     digitalWrite(D2, HIGH);  // Turn the LED off by making the voltage HIGH
      delay(100);
      digitalWrite(D2, LOW);
      delay(100);
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
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D5, INPUT_PULLUP);
  pinMode(D6, INPUT_PULLUP);

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

  // Testing inputs
  if (digitalRead(D5) == LOW)
    digitalWrite(D1, HIGH);
  else
    digitalWrite(D1, LOW);


  if (digitalRead(D6) == LOW)
    digitalWrite(D2, HIGH);
  else
    digitalWrite(D2, LOW);


}