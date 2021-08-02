#include "BedHandler.h"

#include <ESP8266WiFi.h>

void BedHandler::Move_Automatic(Direction dir, unsigned int seconds)
{
    _Move(dir);
    ticker.attach(seconds, std::bind(&BedHandler::Stop, this));
}

void BedHandler::Move_Automatic(Direction dir, short percent)
{
    
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
    //bIsMoving = false;
    //digitalWrite(OUTPUT_DOWN, LOW);
    //digitalWrite(OUTPUT_UP, LOW);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LED_BUILTIN_AUX, HIGH);
    
    Serial.println("STOP Called");

    ticker.detach();
}


/*********** Private ***********/

void BedHandler::_Move(Direction dir)
{
    //bIsMoving = true;

    switch(dir)
    {
        case (UP):
        {
            //digitalWrite(OUTPUT_UP, HIGH);
            //digitalWrite(OUTPUT_DOWN, LOW);
            digitalWrite(LED_BUILTIN, HIGH);
            digitalWrite(LED_BUILTIN_AUX, LOW);
            break;
        }
        case (DOWN):
        {
            //digitalWrite(OUTPUT_DOWN, HIGH);
            //digitalWrite(OUTPUT_UP, LOW);
            digitalWrite(LED_BUILTIN_AUX, HIGH);
            digitalWrite(LED_BUILTIN, LOW);
            break;
        }
    }
}