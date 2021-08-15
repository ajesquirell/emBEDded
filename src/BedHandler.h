#ifndef BEDHANDLER_H
#define BEDHANDLER_H

#include <Ticker.h>
#include "MpuHandler.h"

// Pin definitions for bed movement
#define OUTPUT_UP D8
#define OUTPUT_DOWN D7


class BedHandler {
    public:
    //BedHandler();
    ~BedHandler();

    enum Direction {
        UP,
        DOWN
    };

    enum Modifier {
        SECONDS,
        PERCENT
    };

    bool IsMoving() { return bIsMoving; }

    void Move_Automatic(Direction dir, Modifier mod, uint8_t values);
    void Move_Manual(Direction dir);
    // Move_Random or Alarm

    void Update(); //-> Could be for if I wanted to ensure only this class can modify bed position, using private up/down states that are monitored... But This is only me and that is just more complex
    //Reset? -> Every time MQTT message is received?
    void Init();

    void Stop();
   

    private:
    Ticker ticker;
    MpuHandler mpu;

    bool bIsMoving = false; // Don't check this within this class - For user to check - Move/Stop methods here should be simple and not conditional

    int testValue = 0;
    float testYpr = 0.0f;

    void _Move(Direction dir);
};

#endif