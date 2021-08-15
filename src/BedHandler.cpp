#include "BedHandler.h"
#include <ESP8266WiFi.h>


BedHandler::~BedHandler()
{
    Stop();
    if (ticker.active()) ticker.detach();
}
void BedHandler::Move_Automatic(Direction dir, Modifier mod, uint8_t seconds)
{
    if (mod == SECONDS) // Start bed movement now, end later via ticker without blocking code

    {
        _Move(dir);
        ticker.attach(seconds, std::bind(&BedHandler::Stop, this));
    }
    

    if (mod == PERCENT)
    {
        
    }
}


/// @brief Overrides automatic movement
void BedHandler::Move_Manual(Direction dir)
{
    // Override / cancel automatic actions
    ticker.detach(); // --> Seconds
    // Cancel Percent callback/whatever --> Percent

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

void BedHandler::Update() 
{
    mpu.Update();

    /*Serial.print("ypr\t");
    Serial.print(mpu.GetYprData()[0] * 180 / M_PI);
    Serial.print("\t");
    Serial.print(mpu.GetYprData()[1] * 180 / M_PI);
    Serial.print("\t");
    Serial.print(mpu.GetYprData()[2] * 180 / M_PI);
    Serial.println();*/

    digitalWrite(LED_BUILTIN, 0);
    delayMicroseconds(testValue);
    digitalWrite(LED_BUILTIN, 1);
    delayMicroseconds(5000 - testValue);

    testYpr = mpu.GetYprData()[2] * 180 / M_PI;
    if (testYpr > 90)
        testYpr = 90;
    if (testYpr < 0)
        testYpr = 0;
    
    testValue = (testYpr / 90.0f) * 2000;

    if (testValue > 5000)
        testValue = 5000;
    if (testValue < 0)
        testValue = 0;

    Serial.println(testValue);
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