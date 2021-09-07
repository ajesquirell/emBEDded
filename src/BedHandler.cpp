#include "BedHandler.h"
#include <ESP8266WiFi.h>


BedHandler::~BedHandler()
{
    Stop();
    if (ticker.active()) ticker.detach();
}
void BedHandler::Move_Automatic(Direction dir, Modifier mod, uint8_t amount)
{
    if (mod == SECONDS) // Start bed movement now, end later via ticker without blocking code

    {
        _Move(dir);
        ticker.attach(amount, std::bind(&BedHandler::Stop, this));
    }
    

    if (mod == PERCENT) // Start bed movement now, end later via Update() which will check for bed angle
    {
        // Shift calibrated range so that low limit of our range is 0
        fHighLimit = fCalibrationValueUP - fCalibrationValueDOWN;

        float fStartingPcnt = (( mpu.GetYprData()[2] - fCalibrationValueDOWN ) / fHighLimit) * 100; // Get starting relative percentage

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
    // Cancel Percent callback/whatever --> Percent
    bPercentMoving = false;

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
            return;
        } 

        // Shift raw mpu data by same amount as fHighLimit and normalize to our calibrated range, then make it a percent
        float fPosPcnt = (( mpu.GetYprData()[2] - fCalibrationValueDOWN ) / fHighLimit) * 100;

        //Debug
        /*Serial.println(fCalibrationValueDOWN);
        Serial.println(mpu.GetYprData()[2] - fCalibrationValueDOWN);
        Serial.println(fHighLimit);
        Serial.println(fPosPcnt);
        Serial.print("\n\n\n");*/

        // Check if arrived at desired bed angle
        if (percentMovePacket.d == UP && fPosPcnt >= percentMovePacket.pcnt)
        {
            Stop();
            bPercentMoving = false;
        }
        else if (percentMovePacket.d == DOWN && fPosPcnt <= percentMovePacket.pcnt)
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
                Move_Automatic(UP, SECONDS, 30);
                nCalibrationStep = 2;
            }  
        }
        case(2):
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