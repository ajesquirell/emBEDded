#ifndef BedHandler_h
#define BedHandler_h

#include <Ticker.h>

// Pin definitions
#define OUTPUT_UP D8
#define OUTPUT_DOWN D7
#define INPUT_UP D5
#define INPUT_DOWN D6
#define INPUT_POWER 9

class BedHandler {
    public:
    //BedHandler();

    enum Direction {
        UP,
        DOWN
    };

    //bool IsMoving() { return bIsMoving; }

    void Move_Automatic(Direction dir, unsigned int seconds);
    void Move_Automatic(Direction dir, short percentHeight);
    void Move_Manual(Direction dir);
    // Move_Random or Alarm

    //void Update(); -> Could be for if I wanted to ensure only this class can modify bed position, using private up/down states that are monitored... But This is only me and that is just more complex
    //Reset? -> Every time MQTT message is received?

    void Stop();
   

    private:
    Ticker ticker;

    //bool bIsMoving = false; // Don't check this within this class - For user to check - Move/Stop methods here should be simple and not conditional

    void _Move(Direction dir);
};

#endif