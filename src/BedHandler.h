#ifndef BEDHANDLER_H
#define BEDHANDLER_H

#include <Ticker.h>
#include <queue>
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

    void Move_Automatic(Direction dir, Modifier mod, uint8_t amount);
    void Move_Manual(Direction dir);
    // Move_Random or Alarm

    void Update(); //-> Could be for if I wanted to ensure only this class can modify bed position, using private up/down states that are monitored... But This is only me and that is just more complex
    //Reset? -> Every time MQTT message is received?
    void Init();

    void Stop();
   
    void Calibrate();
    bool IsCalibrating() { return bCalibrating; }

    ///@retval std::pair<float, float> first: UP, second: DOWN
    std::pair<float, float> GetCalibrationValues() { return std::pair<float, float> { fCalibrationValueUP, fCalibrationValueDOWN}; }
    void SetCalibrationValuesManually(float fUp, float fDown)
    {
        fCalibrationValueUP = fUp;
        fCalibrationValueDOWN = fDown;
        fHighLimit = fUp - fDown;
        bUsing360 = (fCalibrationValueUP > 180 || fCalibrationValueDOWN > 180) ? true : false;
    }

    private:
    MpuHandler mpu;
    Ticker ticker;

    struct
    {
        Direction d;
        float pcnt;
    } percentMovePacket;
    bool bPercentMoving = false;

    bool bIsMoving = false; // Don't check this within this class - For user to check via IsMoving() - Move/Stop methods here should be simple and not conditional

    bool bCalibrating = false;
    float fCalibrationValueUP = 0;
    float fCalibrationValueDOWN = 0;
    float fCalibrationMidpoint = 0;
    float fHighLimit = 0; // The shifted calibration "up" value to where the low/down limit is 0 (for normalizing and calculating percent)  
    bool bUsing360 = false;
    unsigned long lastTime = 0;
    uint8_t nCalibrationStep = 0;
    int nSamples = 20;
    float GetCurrentData();
    bool _UpdateCalibration();
    void _Move(Direction dir);
};

#endif