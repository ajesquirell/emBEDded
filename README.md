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
    "data" :
    {
        "move"  :
        {
            "dir"  : "up OR down",
            "mod"  : "seconds OR percent",
            "amt"  : "VALUE"
        }
    }
}
```

If COMMAND is "alarm":
```json
{
    "data" :
    {
        "alarm"  :
        {
            "time" : ["HOUR", "MIN"],
            "ampm" : "am OR pm"
        }
    }
}
```

NOTE: Some MQTT brokers automatically include "data" in the JSON payload. If that is the case, simply start with your "COMMAND".
