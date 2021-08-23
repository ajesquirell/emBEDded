# emBEDded

Welcome to my emBEDded systems project - aka my IoT controlled bed

# (IN PROGRESS)


## List of commands:

    1. move
    2. calibrate (To be implemented)
    3. alarm (To be implemented)

## JSON Format

If COMMAND is "move":

```json
{
  "data": {
    "move": {
      "dir": "up OR down",
      "mod": "seconds OR percent",
      "amt": "VALUE"
    }
  }
}
```

If COMMAND is "alarm":

```json
{
  "data": {
    "alarm": {
      "time" : "hour : min a/p . m .",
      "active" : true/false
    }
  }
}
```

If using Beebotte as the Websocket-MQTT Bridge, you are able to create a "Send-on-Subscribe" resource (SoS) for your alarm which will automatically send the device the latest alarm set when it boots. This is useful in case of power outages or in other cases where your device might reboot.

NOTE: Some MQTT brokers automatically include "data" in the JSON payload. If that is the case, simply start with your "COMMAND".

Can calibrate max and min bed angle values and save them to the cloud. They are automatically retrieved on boot.
Alarm values are also saved to the cloud and are automatically retrieved by the device on boot. Just in case of a power outage.


credentials.h file format:
(Example shown for using beebotte.com for MQTT)
```C
// Network Values
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* mqtt_server = "mqtt.beebotte.com";
const char* mqtt_user = "token:YOUR_TOKEN";
const char* mqtt_pass = "";
```