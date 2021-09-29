#include "BedHandler.h"
#include <ESP8266WiFi.h>

float BedHandler::GetCurrentData()
{
    float fCurrentData = mpu.GetYprData()[2];
    if (bUsing360 && fCurrentData < 0)
        fCurrentData = 180.0f + (180.0f - abs(fCurrentData));
    return fCurrentData;
}

BedHandler::~BedHandler()
{
    Stop();
    if (ticker.active()) ticker.detach();
}
void BedHandler::Move_Automatic(Direction dir, Modifier mod, uint8_t amount)
{
    if (mod == SECONDS) // Start bed movement now, end later via ticker without blocking code
    {
        if (amount > 30)
            amount = 30;

        _Move(dir);
        ticker.attach(amount, std::bind(&BedHandler::Stop, this));
    }
    

    if (mod == PERCENT) // Start bed movement now, end later via Update() which will check for bed angle
    {
        if (amount > 100)
            amount = 100;
        if (amount < 0)
            amount = 0;
            
        float fStartingPcnt = (( GetCurrentData() - fCalibrationValueDOWN ) / fHighLimit) * 100; // Get starting relative percentage

        if (abs(fStartingPcnt - amount) >= 0.5f) // Make sure we're actually moving
        {
            percentMovePacket.d = fStartingPcnt < amount ? UP : DOWN; // Set direction accordingly
            bPercentMoving = true;
            percentMovePacket.pcnt = amount;
            _Move(percentMovePacket.d);
        }
    }
}


/// @brief Overrides automatic movement
void BedHandler::Move_Manual(Direction dir)
{
    // Override / cancel automatic actions
    ticker.detach(); // --> Seconds
    bPercentMoving = false; // --> Percent

    _Move(dir);
}

void BedHandler::Stop()
{
    bIsMoving = false;
    digitalWrite(OUTPUT_DOWN, LOW);
    digitalWrite(OUTPUT_UP, LOW);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LED_BUILTIN_AUX, HIGH);
    
    Serial.println("STOP Called");

    ticker.detach();
}

void BedHandler::Calibrate()
{
    bCalibrating = true;
    nCalibrationStep = 0;
    fCalibrationValueUP = 0;
    fCalibrationValueDOWN = 0;    
}

void BedHandler::Update() // To be called every main loop
{
    mpu.Update();

    if (bCalibrating)
    {
        bCalibrating = _UpdateCalibration();
    }

    if (bPercentMoving)
    {
        if (fHighLimit == 0)
        {
            // Calibration values not correct, avoid divide by 0 and end now
            bPercentMoving = false;
            Stop();
            return;
        } 

        // Shift raw mpu data by same amount as fHighLimit and normalize to our calibrated range, then make it a percent
        float fCurrentPcnt = (( GetCurrentData() - fCalibrationValueDOWN ) / fHighLimit) * 100;

        //Debug
        //Serial.println(fCalibrationValueDOWN);
        //Serial.println(mpu.GetYprData()[2] - fCalibrationValueDOWN);
        //Serial.println(fHighLimit);
        //Serial.println(GetCurrentData());
        Serial.println(fCurrentPcnt);
        Serial.print("\n");

        // Check if arrived at desired bed angle
        if (percentMovePacket.d == UP && fCurrentPcnt >= percentMovePacket.pcnt)
        {
            Stop();
            bPercentMoving = false;
        }
        else if (percentMovePacket.d == DOWN && fCurrentPcnt <= percentMovePacket.pcnt)
        {
            Stop();
            bPercentMoving = false;
        }
    }
    /*Serial.print("ypr\t");
    Serial.print(mpu.GetYprData()[0] * 180 / M_PI);
    Serial.print("\t");
    Serial.print(mpu.GetYprData()[1] * 180 / M_PI);
    Serial.print("\t");
    Serial.print(mpu.GetYprData()[2] * 180 / M_PI);
    Serial.println();*/

    // digitalWrite(LED_BUILTIN, 0);
    // delayMicroseconds(testValue);
    // digitalWrite(LED_BUILTIN, 1);
    // delayMicroseconds(5000 - testValue);

    // testYpr = mpu.GetYprData()[2];
    // if (testYpr > 90)
    //     testYpr = 90;
    // if (testYpr < 0)
    //     testYpr = 0;
    
    // testValue = (testYpr / 90.0f) * 2000;

    // if (testValue > 5000)
    //     testValue = 5000;
    // if (testValue < 0)
    //     testValue = 0;

    //Serial.println(testValue);
}

void BedHandler::Init()
{
    mpu.Init();
}

/*********** Private ***********/

void BedHandler::_Move(Direction dir)
{
    bIsMoving = true;

    switch(dir)
    {
        case (UP):
        {
            digitalWrite(OUTPUT_UP, HIGH);
            digitalWrite(OUTPUT_DOWN, LOW);
            digitalWrite(LED_BUILTIN, LOW);
            digitalWrite(LED_BUILTIN_AUX, HIGH);
            break;
        }
        case (DOWN):
        {
            digitalWrite(OUTPUT_DOWN, HIGH);
            digitalWrite(OUTPUT_UP, LOW);
            digitalWrite(LED_BUILTIN, HIGH);
            digitalWrite(LED_BUILTIN_AUX, LOW);
            break;
        }
    }
}

bool BedHandler::_UpdateCalibration()
{   
    switch (nCalibrationStep)
    {
        case(0):
        {
            if (!IsMoving()) // Start by moving bed all the way down
            {
                Move_Automatic(DOWN, SECONDS, 30);
                nCalibrationStep = 1;
            } 
        }
        case(1):
        {
            if (!IsMoving()) // Bed is down, get calibration value and go up
            {
                for (int i = 0; i < nSamples; i++)
                {
                    mpu.Update();
                    fCalibrationValueDOWN += mpu.GetYprData()[2];
                }
                fCalibrationValueDOWN /= (float)nSamples; // Average
                Move_Automatic(UP, SECONDS, 5);
                nCalibrationStep = 2;
            }  
        }
        case(2):
        {
            if (!IsMoving()) // quickly get 'midpoint' to tell direction then go the rest of the way up
            {
                for (int i = 0; i < 5; i++)
                {
                    mpu.Update();
                    fCalibrationMidpoint += mpu.GetYprData()[2];
                }
                fCalibrationMidpoint /= 5.0f;
                Move_Automatic(UP, SECONDS, 25);
                nCalibrationStep = 3;
            }
        }
        case(3):
        {
            if (!IsMoving()) // Bed is up, get calibration vaue and go back down
            {
                for (int i = 0; i < nSamples; i++)
                {
                    mpu.Update();
                    fCalibrationValueUP += mpu.GetYprData()[2];
                }
                fCalibrationValueUP /= (float)nSamples; // Average
                Move_Automatic(DOWN, SECONDS, 30); // Reset to down position
                nCalibrationStep = -1;

                // Blink to show done
                for (int i = 0; i < 6; i++)
                {
                    digitalWrite(LED_BUILTIN, LOW);
                    digitalWrite(LED_BUILTIN_AUX, LOW);
                    delay(80);
                    digitalWrite(LED_BUILTIN, HIGH);
                    digitalWrite(LED_BUILTIN_AUX, HIGH);
                    delay(80);
                }
                Serial.print("\n\n");
                Serial.println("Calibration values:");
                Serial.println(fCalibrationValueDOWN);
                Serial.println(fCalibrationValueUP);
                Serial.print("\n\n");

                // Problems could occur because when going the same direction, ypr values will "reset" at a certain point
                // This is at -180/180 for normal ypr values and 0/360 if we were to convert values 360 degrees
                // The raw values come in between -180/180 degrees. If this point occurs between our calibration values, it will mess up the simple interpolation I want to do for the percentage
                // Converting to 360 degrees will fix the problem at 180, as this will shift that problem point to be at 0
                // The bed will not go more than 180 degrees (not even close), so we don't have to worry about hitting both problem points
                // (There is the case where UP and DOWN values could have the same sign and still cross one of these problems, but that requires going > 180 deg,
                // which doesn't make sense for a bed, so I'm not going to worry about it)
                
                struct {
                    unsigned int bits : 2;
                } bProblem {0b00};

                // If opposite signs, our values cross one of the problem spots
                if (fCalibrationValueDOWN < 0 && fCalibrationValueUP >= 0)
                    bProblem.bits = 0b01;
                else if (fCalibrationValueDOWN >= 0 && fCalibrationValueUP < 0)
                    bProblem.bits = 0b10;
                
                if (bProblem.bits != 0b00)
                {
                    bool bCalibrationValueDir = fCalibrationMidpoint - fCalibrationValueDOWN < 0 ? false : true; // If shifted values so Calib-Down value was at 0, the sign of midpoint will give dir.

                    if ((bProblem.bits == 0b01 && !bCalibrationValueDir) ||
                        (bProblem.bits == 0b10 && bCalibrationValueDir)) // Start neg, going neg OR Start pos, going pos - means we cross 180
                    {
                        // Convert to 360 deg
                        if (fCalibrationValueUP < 0)
                            fCalibrationValueUP = 180.0f + (180.0f - abs(fCalibrationValueUP));
                        if (fCalibrationValueDOWN < 0)
                            fCalibrationValueDOWN = 180.0f + (180.0f - abs(fCalibrationValueDOWN));
                        
                        bUsing360 = true;
                    }
                }

                // Essentially shift the calibrated range so that low limit of this range is 0, so we can normalize using the high limit
                fHighLimit = fCalibrationValueUP - fCalibrationValueDOWN;

                Serial.print("\n\n");
                Serial.println("Calibration values after fixing:");
                Serial.println(fCalibrationValueDOWN);
                Serial.println(fCalibrationValueUP);
                Serial.println(fHighLimit);
                Serial.print("\n\n");

                //while(IsMoving()) yield();
                return false;
            }
        }
    }
    
    //Move_Automatic(DOWN, SECONDS, 5);
    //while (IsMoving()) yield(); // wait --> Later make this non blocking
    // for (int i = 0; i < nSamples; i++)
    // {
    //     mpu.Update();
    //     fCalibrationValueDOWN += mpu.GetYprData()[2];
    // }
    // fCalibrationValueDOWN /= (float)nSamples; // Average
    
    //Move_Automatic(UP, SECONDS, 5);
    //while (IsMoving()) yield(); // wait
    // for (int i = 0; i < nSamples; i++)
    // {
    //     mpu.Update();
    //     fCalibrationValueUP += mpu.GetYprData()[2];
    // }
    // fCalibrationValueUP /= (float)nSamples; // Average
    

    // // Reset to down
    // Move_Automatic(DOWN, SECONDS, 5);

    return true; // True when currently running calibration, false when done
}