#include <Arduino.h>
#include <ESP8266WiFi.h>

// Pin definitions
#define OUTPUT_UP D8
#define OUTPUT_DOWN D7
#define INPUT_UP D5
#define INPUT_DOWN D6
#define INPUT_POWER 9

// Global pin interrupt flags (volatile because they are changed within the interrupt i.e. outside of the main loop)
volatile bool bButtonPressedUp = false;
volatile bool bButtonPressedDown = false;

// ISR Declarations
void ICACHE_RAM_ATTR IsrUp();
void ICACHE_RAM_ATTR IsrDown();


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
attachInterrupt(digitalPinToInterrupt(INPUT_UP), IsrUp, FALLING);
  attachInterrupt(digitalPinToInterrupt(INPUT_DOWN), IsrDown, FALLING);
  //pinMode(LED_BUILTIN, OUTPUT);
  //pinMode(LED_BUILTIN_AUX, OUTPUT);

  pinMode(OUTPUT_UP, OUTPUT);
  pinMode(OUTPUT_DOWN, OUTPUT);

  pinMode(INPUT_UP, INPUT);
  pinMode(INPUT_DOWN, INPUT);

  pinMode(INPUT_POWER, INPUT);

  

}

volatile unsigned long test1;
volatile unsigned long test2;
volatile bool flag1, flag2;

// ISR's
void ICACHE_RAM_ATTR IsrUp()
{
  //if (millis() > test1) {
    //Serial.println("IsrUp run");
    flag1 = true;
    //bButtonPressedUp = !digitalRead(INPUT_UP);
    bButtonPressedUp = !bButtonPressedUp;

    //digitalWrite(LED_BUILTIN, (bButtonPressedUp == true ? LOW : HIGH));
    digitalWrite(OUTPUT_UP, (bButtonPressedUp == true ? HIGH : LOW)); // This is handled in the ISR so that it occurs even when wifi is connecting in the beginning, as that blocks the rest of the code
    //test1 = millis() + 500;
  //}
}
void ICACHE_RAM_ATTR IsrDown()
{
  //if (millis() > test2){
    //Serial.println("IsrDown run");
    flag2 = true;
    //bButtonPressedDown = !digitalRead(INPUT_DOWN);
    bButtonPressedDown = !bButtonPressedDown;

    //digitalWrite(LED_BUILTIN_AUX, (bButtonPressedDown == true ? LOW : HIGH));
    digitalWrite(OUTPUT_DOWN, (bButtonPressedDown == true ? HIGH : LOW)); // This is handled in the ISR so that it occurs even when wifi is connecting in the beginning, as that blocks the rest of the code
    //test2 = millis() + 500;
  //}
}

void loop() {
  // put your main code here, to run repeatedly:
  if (flag1 == true)
  {
    Serial.println("UP");
    flag1 = false;
  }

  if (flag2 == true)
  {
    Serial.println("DOWN");
    flag2 = false;
  }

  // digitalWrite(OUTPUT_UP, HIGH);
  // digitalWrite(OUTPUT_DOWN, LOW);
  // delay(3000);

  // digitalWrite(OUTPUT_DOWN, HIGH);
  // digitalWrite(OUTPUT_UP, LOW);
  // delay(3000);



}