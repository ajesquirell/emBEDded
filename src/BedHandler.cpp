#include "BedHandler.h"
#include <ESP8266WiFi.h>


BedHandler::~BedHandler()
{
    Stop();
    if (ticker.active()) ticker.detach();
}
void BedHandler::Move_Automatic(Direction dir, Modifier mod, uint8_t seconds)
{
    if (mod == SECONDS)
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
    // Bed Controller Buttons override / cancel automatic actions
    //ticker.detach();
    // Cancel Percent callback/whatever

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