# emBEDded

Welcome to my emBEDded systems project - aka my IoT controlled bed

List of commands:
    1. move
    2. calibrate (To be implemented)
    3. alarm (To be implemented)



JSON Format

{
    "data" :
    {
        "COMMAND"  :
        {
            *IF COMMAND IS "move":
            "dir"  : "up OR down",
            "mod"  : "seconds OR percent",
            "amt"  : "VALUE"

            *IF COMMAND IS "alarm":
            "time" : [HOUR, MIN],
            "ampm" : "am OR pm"
        }
    }
}