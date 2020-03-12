#include <ResponsiveAnalogRead.h>
#include <eLite.h>
#include <EEPROM.h>
#include <Wire.h>
#include "RTClib.h"
RTC_DS3231 rtc;
uint8_t tSecond;
uint8_t tMinute;
uint8_t tHour;

////Color dials/potentiometers
#define redDial A3 //switched pins for red and blue, easier on the Eagle schematic
#define greenDial A2
#define blueDial A1

ResponsiveAnalogRead redInput(redDial, true);
ResponsiveAnalogRead greenInput(greenDial, true);
ResponsiveAnalogRead blueInput(blueDial, true);

#define stripPin 6
#define stripLength 70
#define displayQuantity 5
eLite eLite(stripPin, stripLength, false);


byte debounceTime = 50;

byte r = 255;
byte g = 255;
byte b = 255;

////Alarm-Time
typedef struct { //declaring a struct which can house the time to ring the alarm (or a notification of the time. [Clock beeps on the hour like a cookoo clock.])
  byte alarmHour = 1; //I had to declare my own struct for the record keeping because the alarm structs used in the RTC library provide inaccurate results when queried in succesion.
  byte alarmMin = 0; //seconds isn't needed, the alarms will only beep at the 0 second mark of the hour/minute which can be wrought from the RTC library.
  boolean state = true;
  boolean beenRung = false;
} alarmTime_type;
alarmTime_type alarmTime[2];

boolean dst;




//Alarm button pins
#define alarmButton 2 //quick press to display time / silence alarm. Long press [~2500 ms] to enter alarm editing mode.
#define notificationButton 3 //same use as alarmButton
#define notifOnBut 0
#define snoozeBut 1
#define alarmOnBut 13


boolean alarmEditMode = false;
boolean alarmToDrawInfo = false;
short alarmEditThreshold = 1500; //milliseconds
boolean alarmToBuzz = false;
unsigned long alarmToBuzzStartTime;
unsigned long alarmFirstBeepTime;
bool alarmBuzzerTimeUndefined = false;
bool toSnooze = false;
bool snoozeDown = true;
unsigned long snoozeStartTime;

boolean timerToBuzz = false;
unsigned long timerToBuzzStartTime;
unsigned long timerFirstBeepTime;
bool timerBuzzerTimeUndefined = false;
boolean notifToBuzz = false;
unsigned long notifToBuzzStartTime;
bool notifBuzzerTimeUndefined = false;
uint8_t notifBeepCount = 0;
boolean notifDown = true;

#define hmBut 4 //switches between allowing you to edit the hour, or the minute.
boolean hmMode;
uint8_t scrollRate = 250;
uint8_t timerScrollRate = 63;
#define hmMinusBut 5 //back 1 hour/min
#define hmPlusBut 7 //forwawrd 1 hour/min

boolean tfMode = false;

//Photoresistor
#define photoBut 8
boolean photoDown = true;
boolean photoOn = true;
boolean photoChanged = false;
//short photoBrightness;
short photoThreshold = 40; //In a range of 1-1024
//short averageBrightness = 64;
//short darkBrightness = 16;
#define photoresistor A0

//Buzzer
#define buzzer 9 //This is a PWM pin on the Atmega328, allows for different tones

//Timer buttons
#define timerSetBut 10
#define timerMinusBut 11
#define timerPlusBut 12
boolean timerCounting = false;
boolean timerSetDown = true;

uint8_t timerHour = 0; //I had to do hour / minute and fancy logic with this because writing it as simply 'how many minutes there are in the timer' and using that to derive time in hours would requrie 8 EEPROM bytes. [1799 minutes inside of 29 hours, 59 minutes.]
uint8_t timerMinute = 0;                                                                                                         
uint8_t timerSecond = 0; //Gets put into use when timer drops below 1 minute / 60 seconds. At which point it is displayed on the edge lit panels.
uint8_t timerCheckSecond = 0;
uint8_t timerStartSecond;
bool timerFirstTick = false;
#define longHoldThreshold 500 //how long to wait before each incrementation of the timer value when holding the timer buttons
#define longHoldIncrementValue 5
#define extraLongHoldIncrementValue 10

void setup() {
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // put your setup code here, to run once:
  eLite.defineChain(displayQuantity, 2, 9, 5, 9, 5); //There exists #displayQuantity displays [5] with zero-indexed panel count of 2, 9, 5, 9, and 5 respectively.
  eLite.defineLedPerPanel(displayQuantity, 2, 2, 2, 2, 2); //Each panel in the first display has 2 leds, and each panel in the second display has 2 leds, etc.
  eLite.redefinePanelOrder(1, 3, 1, 2, 0); //display #1 has 3 panels. In the order: 1, 2, 0
  eLite.redefinePanelOrder(2, 10, 1, 7, 5, 8, 4, 3, 9, 2, 0, 6);
  eLite.redefinePanelOrder(3, 6, 1, 5, 4, 3, 2, 0);
  eLite.redefinePanelOrder(4, 10, 1, 7, 5, 8, 4, 3, 9, 2, 0, 6);

  r = redInput.getValue(); //Updates the colors on first launch. otherwise each pot has to be turned on startup before active color is seen 
  g = greenInput.getValue();
  b = blueInput.getValue();
  
  eLite.clearChain();
  
  //Serial.begin(9600);
  
  //delay(3000);
  
  pinMode(alarmButton, INPUT_PULLUP);
  pinMode(alarmOnBut, INPUT_PULLUP);
  pinMode(notificationButton, INPUT_PULLUP);
  pinMode(notifOnBut, INPUT_PULLUP);
  pinMode(snoozeBut, INPUT_PULLUP);
  pinMode(hmBut, INPUT_PULLUP);
  pinMode(hmMinusBut, INPUT_PULLUP);
  pinMode(hmPlusBut, INPUT_PULLUP);

  pinMode(photoBut, INPUT_PULLUP);
  pinMode(photoresistor, INPUT);
  pinMode(buzzer, OUTPUT);

  pinMode(timerSetBut, INPUT_PULLUP);
  pinMode(timerMinusBut, INPUT_PULLUP);
  pinMode(timerPlusBut, INPUT_PULLUP);

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    rtc.adjust(DateTime(rtc.now().unixtime() + 17));
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //rtc.adjust(DateTime(rtc.now().unixtime() + 17));
  tSecond = 255; //Ensures the comparsion of these numbers against time succeeds because time will never be '255'
  tMinute = 255;
  tHour = 255;

  //Sets the LEDS for information display
  drawInfo();

  //EEPROM for whether or not daylight savings is on.
  if (EEPROM.read(9) != 0 && EEPROM.read(9) != 1) {
    EEPROM.write(9, 1); dst = 1; //Daylight savings is on by default. Doesn't matter because the script checks to turn it back off or keep it on later.
  } else { dst = EEPROM.read(9); }
  //If the alarm memory has never been set before, reset it.
  if (EEPROM.read(0) > 24 or EEPROM.read(2) > 24 or EEPROM.read(1) > 59 or EEPROM.read(3) > 59) {
  EEPROM.write(0, 24); EEPROM.write(1, 0); EEPROM.write(2, 24); EEPROM.write(3, 0); }
  //Writes alarm times and notification times from memory
  alarmTime[0].alarmHour = EEPROM.read(0);
  alarmTime[0].alarmMin = EEPROM.read(1);
  alarmTime[1].alarmHour = EEPROM.read(2);
  alarmTime[1].alarmMin = EEPROM.read(3);

  if (EEPROM.read(8) != 0 && EEPROM.read(8) != 1) {
    EEPROM.write(8, 0);
    alarmTime[1].state = EEPROM.read(8);
  } else {
    alarmTime[1].state = EEPROM.read(8);
  }
  //Writes various Setting states from memory
  //photoresistor memory
  if (EEPROM.read(5) == 255) { EEPROM.write(5, 0); }
  else { photoOn = EEPROM.read(5); }

  
  

  //timer Memory
  if (EEPROM.read(6) > 24 or EEPROM.read(7) > 59) { EEPROM.write(6, 0); EEPROM.write(7, 0); }
  timerHour = EEPROM.read(6);
  timerMinute = EEPROM.read(7);

}

void displayAlarmTime(uint8_t toEdit) {
  uint8_t alarmTimeHour;
  eLite.clearDisplay(4);
  if (!tfMode) {
    alarmTimeHour = alarmTime[toEdit].alarmHour;
    if (alarmTimeHour > 12) {
      alarmTimeHour -= 12;
    }
    if ((alarmTime[toEdit].alarmHour >=1 and alarmTime[toEdit].alarmHour <= 11) or alarmTime[toEdit].alarmHour == 24) { eLite.drawPanel(r, g, b, 4, 1); } //Draws the AM
    else if (alarmTime[toEdit].alarmHour >= 12 and alarmTime[toEdit].alarmHour <= 23) { eLite.drawPanel(r, g, b, 4, 0); }
  if (toEdit == 1) {
    eLite.drawPanel(r, g, b, 4, 5);    
  } else {
    eLite.drawPanel(r, g, b, 4, 4);
  }
  eLite.clearDisplay(0); eLite.drawPanel(r, g, b, 0, (alarmTimeHour / 10) % 10);
  eLite.clearDisplay(1); eLite.drawPanel(r, g, b, 1, alarmTimeHour % 10);  
  } else {
  eLite.clearDisplay(0); eLite.drawPanel(r, g, b, 0, (alarmTime[toEdit].alarmHour / 10) % 10);
  eLite.clearDisplay(1); eLite.drawPanel(r, g, b, 1, alarmTime[toEdit].alarmHour % 10);
  }
  eLite.clearDisplay(2); eLite.drawPanel(r, g, b, 2, (alarmTime[toEdit].alarmMin / 10) % 10);
  eLite.clearDisplay(3); eLite.drawPanel(r, g, b, 3, alarmTime[toEdit].alarmMin % 10);
  
}
void drawAlarmHour(uint8_t toEdit) {
  uint8_t alarmTimeHour;
  eLite.clearChain(); if (!tfMode) { //converts the 1-24 o'clock of the alarm buttons into 0-12 AM and PM if we're not in 24 hour mode. [tfMode]
  alarmTimeHour = alarmTime[toEdit].alarmHour; 
  if (alarmTimeHour > 12) {
    alarmTimeHour -= 12;
  }
  if ((alarmTime[toEdit].alarmHour >=1 and alarmTime[toEdit].alarmHour <= 11) or alarmTime[toEdit].alarmHour == 24) { eLite.drawPanel(r, g, b, 4, 1); } //Draws the AM
  else if (alarmTime[toEdit].alarmHour >= 12 and alarmTime[toEdit].alarmHour <= 23) { eLite.drawPanel(r, g, b, 4, 0); }
    eLite.drawPanel(r, g, b, 0, (alarmTimeHour / 10) % 10);
    eLite.drawPanel(r, g, b, 1, alarmTimeHour % 10); 
  } else {
    eLite.drawPanel(r, g, b, 0, (alarmTime[toEdit].alarmHour / 10) % 10);
    eLite.drawPanel(r, g, b, 1, alarmTime[toEdit].alarmHour % 10);
  }
}
void drawAlarmMinute(uint8_t toEdit) {
  eLite.clearChain(); if (!tfMode) {
    if ((alarmTime[toEdit].alarmHour >=1 and alarmTime[toEdit].alarmHour <= 11) or alarmTime[toEdit].alarmHour == 24) { eLite.drawPanel(r, g, b, 4, 1); } //Draws the AM
    else if (alarmTime[toEdit].alarmHour >= 12 and alarmTime[toEdit].alarmHour <= 23) { eLite.drawPanel(r, g, b, 4, 0); }
  }
  eLite.drawPanel(r, g, b, 2, (alarmTime[toEdit].alarmMin / 10) % 10);
  eLite.drawPanel(r, g, b, 3, alarmTime[toEdit].alarmMin % 10);
}

void editAlarmTime(uint8_t button, uint8_t toEdit) {
  Serial.println("editAlarmTime");
  blinkDigits(250); //250 ms blink time
  hmMode = true; drawAlarmHour(toEdit); //Immediately enters into Hour +- mode. (Dealer's choice vs hour or minute first)
  
  boolean alarmButDown = true;
  boolean alarmButRepressed = false;
  boolean hmButDown = true;
  alarmEditMode = true;
  
  while (true) {  
    //checks for an exit to edit mode                                                             //these three lines ensure that the button function is called once.    
    if (digitalRead(button) && alarmButDown == true) {alarmButDown = false; delay(debounceTime);} //It does this by marking when the button is high for its first time, and then marking when it's set low for the first time, and then calling                                                                             
      if (!digitalRead(button) && alarmButDown == false) { alarmButRepressed = true; }              //the button function once it is released again.                                                                                               
      if (digitalRead(button) && alarmButRepressed == true) {Serial.println("You've exited alarm edit mode\n"); alarmButDown = true; alarmButRepressed = false; delay(debounceTime); alarmTime[toEdit].beenRung = false; 
      blinkDigits(250); forceClockTime(); return;} //quarter-second blink time
    //
    if (!digitalRead(hmBut)) {hmButDown = false; delay(debounceTime);}  //this button debounce effect is done again for the various buttons, but can't be applied as a function because the processor would be blocked in / stuck in a waiting loop. Done in-line.
      if (digitalRead(hmBut) && hmButDown == false) { ////Hour / Minute Select Button
        hmButDown = true; hmMode = !hmMode; 
        if (hmMode) {
          Serial.println("Editing Hours");
          drawAlarmHour(toEdit); } 
        else { 
          Serial.println("Editing Minutes");
          drawAlarmMinute(toEdit); }}

          
      if (!digitalRead(hmMinusBut)) { ////Hour/Minute Minus
          if (hmMode) { //Decreasing Hours
            if (alarmTime[toEdit].alarmHour > 1) {alarmTime[toEdit].alarmHour -= 1;}
            else if (alarmTime[toEdit].alarmHour == 1) {alarmTime[toEdit].alarmHour = 24;}
            
            if (toEdit == 0) {
               EEPROM.write(0, alarmTime[toEdit].alarmHour);
            } else { EEPROM.write(2, alarmTime[toEdit].alarmHour); }
            Serial.println(alarmTime[toEdit].alarmHour);
            drawAlarmHour(toEdit);
          } 
          if (!hmMode){ //Decreasing Minutes
            if (alarmTime[toEdit].alarmMin > 0) {alarmTime[toEdit].alarmMin -= 1;}
            else if (alarmTime[toEdit].alarmMin == 0) {alarmTime[toEdit].alarmMin = 59;}
            if (toEdit == 0) {
               EEPROM.write(1, alarmTime[toEdit].alarmMin);
            } else { EEPROM.write(3, alarmTime[toEdit].alarmMin); }
            Serial.println(alarmTime[toEdit].alarmMin);
            drawAlarmMinute(toEdit);
          } 
          delay(scrollRate);
    }

    
    if (!digitalRead(hmPlusBut)) { ////Hour/Minute Plus
          if (hmMode) { //Increasing Hours
            if (alarmTime[toEdit].alarmHour < 24) {alarmTime[toEdit].alarmHour += 1;}
            else if (alarmTime[toEdit].alarmHour == 24) {alarmTime[toEdit].alarmHour = 1;}
            if (toEdit == 0) {
               EEPROM.write(0, alarmTime[toEdit].alarmHour);
            } else { EEPROM.write(2, alarmTime[toEdit].alarmHour); }
            Serial.println(alarmTime[toEdit].alarmHour);
            drawAlarmHour(toEdit);
          } 
          if (!hmMode){ //Increasing Minutes
            if (alarmTime[toEdit].alarmMin < 59) {alarmTime[toEdit].alarmMin += 1;}
            else if (alarmTime[toEdit].alarmMin == 59) {;alarmTime[toEdit].alarmMin = 0;}
            if (toEdit == 0) {
               EEPROM.write(1, alarmTime[toEdit].alarmMin);
            } else { EEPROM.write(3, alarmTime[toEdit].alarmMin); }
            Serial.println(alarmTime[toEdit].alarmMin);
            drawAlarmMinute(toEdit); 
          }
          delay(scrollRate);
    }
  }
  return;
}
void alarmPressed(uint8_t button, uint8_t toEdit) {
  displayAlarmTime(toEdit);
  if (button == alarmButton) {Serial.println("Alarm Button Pressed"); }
  else if (button == notificationButton) {Serial.println("Notification Button Pressed"); }
  unsigned long timeAlarmPressed = millis();
  delay(50);
    while (!digitalRead(button)) { //while you're still pressing, if the threshhold for a long press is passed, call the function to change the time modes. Then break to pass the rest of the checks.
      if (millis() - timeAlarmPressed >= alarmEditThreshold) { editAlarmTime(button, toEdit); return;}
    }
    if (digitalRead(button) && millis() - timeAlarmPressed <= alarmEditThreshold) { displayAlarmTime(toEdit); delay(2000); forceClockTime(); } //If it was released, find the new time. If this time is less than 2500, call the display Alarm / silence buzzer function.
  //if (alarmTime[0].state) { Serial.println("buzz"); delay(250);} 
  Serial.print("\n");
}

void timerPressed() {
  //bool tmpDown = true; //timer minutes plus down
  //bool tmmDown = true; //timer minutes minus down
  Serial.println("timer Menu");
  Serial.println("TimerHour: "); Serial.println(timerHour);
  Serial.println("TimerMinute: "); Serial.println(timerMinute);
  eLite.clearChain(); delay(250); //250 ms blink time. clears information display too.
  if (timerCounting) {
    timerCounting = false;
    forceClockTime();
    timerHour = EEPROM.read(6);
    timerMinute = EEPROM.read(7);
    timerSecond = 0; 
    return; 
  }
  eLite.drawPanel(r, g, b, 4, 2);
  drawTimer();
  
  while (digitalRead(timerSetBut)) {  
    //if (!digitalRead(timerPlusBut)) {tmpDown = false; delay(debounceTime);}  //this button debounce effect is done again for the various buttons, but can't be applied as a function because the processor would be blocked in / stuck in a waiting loop. Done in-line.
      //if (digitalRead(timerPlusBut) && tmpDown == false) { ////Hour / Minute Select Button
      if (!digitalRead(timerPlusBut)) {
        //tmpDown = true;    
        if (timerMinute < (59)) {
          timerMinute += 1;
          //Serial.print("Timer:  :"); Serial.println(timerMinute);
        } else {
          if (timerHour < 29) {
              timerMinute = 0;
              //Serial.print("Timer:  : "); Serial.println(timerMinute);
              timerHour += 1;
              //Serial.print("Timer: "); Serial.print(timerHour); Serial.println(":  ");
          } else {
              timerMinute = 59;
              //Serial.print("Timer:  : "); Serial.println(timerMinute);
          }  
        } drawTimer(); delay(scrollRate);
      }
      //if (!digitalRead(timerMinusBut)) {tmmDown = false; delay(debounceTime);}  //this button debounce effect is done again for the various buttons, but can't be applied as a function because the processor would be blocked in / stuck in a waiting loop. Done in-line.
      //if (digitalRead(timerMinusBut) && tmmDown == false) { ////Hour / Minute Select Button
      if (!digitalRead(timerMinusBut)) {
        //tmmDown = true;
        if (timerMinute > 0) {
          timerMinute -= 1;
          //Serial.print("Timer:  :"); Serial.println(timerMinute);
        } else {
          if (timerHour > 0) {
            timerMinute = 59; //Rolls over the number. rolling over from 58, with an increment value of 5 means adding 1 to the hour, and setting the minutes to 3. (60-58+5)
            //Serial.print("Timer:  : "); Serial.println(timerMinute);
            timerHour -= 1;
            //Serial.print("Timer: "); Serial.print(timerHour); Serial.println(":  ");
          } else {
            timerMinute = 1;
            //Serial.print("Timer:  : "); Serial.println(timerMinute);
          }
        } drawTimer(); delay(scrollRate);  
      }  
  }
  eLite.clearChain();
  EEPROM.write(6, timerHour); EEPROM.write(7, timerMinute); Serial.println("Timer Set Button Pressed. Memory saved");
  delay(scrollRate);
  timerCounting = true;
  timerFirstTick = true;
  timerStartSecond = rtc.now().second();
  eLite.clearDisplay(4); eLite.drawPanel(r, g, b, 4, 2);
  forceClockTime();
}

void drawTimer() {
  for (uint8_t i = 0; i <= 3; i ++) {
    eLite.clearDisplay(i);  
  } eLite.drawPanel(r, g, b, 0, (timerHour / 10) % 10);
  eLite.drawPanel(r, g, b, 1, timerHour % 10);
  eLite.drawPanel(r, g, b, 2, (timerMinute / 10) % 10);
  eLite.drawPanel(r, g, b, 3, timerMinute % 10);
}

void drawInfo(){ //sets the LEDS for the various informational options
  eLite.clearDisplay(4);
  uint8_t tempHour = rtc.now().hour();
  if (photoOn) {eLite.drawPanel(r, g, b, 4, 3); }
  if (alarmTime[0].state){eLite.drawPanel(r, g, b, 4, 4); }
  if (alarmTime[1].state){eLite.drawPanel(r, g, b, 4, 5); }
  if (timerCounting) {eLite.drawPanel(r, g, b, 4, 2); }
  if (tempHour >= 12 && !tfMode) {eLite.drawPanel(r, g, b, 4, 0); }
  if (tempHour < 12 && !tfMode) {eLite.drawPanel(r, g, b, 4, 1); }
}

void setClockTime() {
  if (!timerCounting) {
    if (tHour != rtc.now().hour()) {
      tHour = rtc.now().hour();
      uint8_t tempHour = tHour;
      if (tHour > 12) {tempHour -= 12; }
      if (tHour == 0) {tempHour = 12; } //Does not affect actual time, only what is displayed. 0 o'clock = 12 am.
      eLite.clearDisplay(0); eLite.drawPanel(r, g, b, 0, (tempHour  / 10) % 10);
      eLite.clearDisplay(1); eLite.drawPanel(r, g, b, 1, tempHour % 10);
    } if (tMinute != rtc.now().minute()) {
      tMinute = rtc.now().minute();
      eLite.clearDisplay(2); eLite.drawPanel(r, g, b, 2, (tMinute  / 10) % 10);
      eLite.clearDisplay(3); eLite.drawPanel(r, g, b, 3, tMinute % 10);
      drawInfo();
    }
  } if (timerCounting) {
    if (timerCheckSecond != rtc.now().second()) {
      timerCheckSecond = rtc.now().second();
      
      if (timerHour == 0 && timerMinute <=29) { //Start of timer arithmetic Section          
        if (!timerFirstTick) {
          if (timerSecond == 0) { //////First Major Timer arithmetic Section
            if (timerMinute > 0) {
              timerSecond = 59; timerMinute -=1;
            } else { //Timer finishes
              Serial.println("Timer Done"); 
              timerCounting = false;
              timerToBuzz = true;
              timerBuzzerTimeUndefined = true;
              timerFirstBeepTime = rtc.now().unixtime();
                
              timerHour = EEPROM.read(6); //Resets timer values to their memory values for next use.
              timerMinute = EEPROM.read(7);
              timerSecond = 0;       
              blinkDigits(250);
              forceClockTime();
              
              return;
              }
            } else {timerSecond -=1;}
        } 
        
        eLite.clearDisplay(0); eLite.drawPanel(r, g, b, 0, (timerMinute / 10) % 10);
        eLite.clearDisplay(1); eLite.drawPanel(r, g, b, 1, timerMinute % 10);
        eLite.clearDisplay(2); eLite.drawPanel(r, g, b, 2, (timerSecond / 10) % 10);
        eLite.clearDisplay(3); eLite.drawPanel(r, g, b, 3, timerSecond % 10);
        //Serial.print("timer second: "); Serial.println(timerSecond);
        timerFirstTick = false;
        
      } else { //anytime when timerhour > 0 or timerMinute >= 29
        if (timerCheckSecond == timerStartSecond) { //does a check every 60 seconds from the time the timer started.
          if (!timerFirstTick) {
            if (timerHour > 0) { /////Second Major Timer Arithmetic Section
              if (timerMinute == 0) {  //Having this before drawing the numbers would otherwise cause the timer to start 1 minute short without some form of a check.    
                timerMinute = 59;      //But having the draw stuff before it leads to the first minute being only visible for a few seconds
                timerHour -=1;
                //Serial.print("minutes decreasing: "); Serial.println(timerMinute);
              } else { //timer minute > 0
                timerMinute -= 1; }                                
            } else { //timer hour == 0
              if (timerMinute > 0) {
                timerMinute -=1;
                //Serial.print("minutes decreasing: "); Serial.println(timerMinute);
              }
            }
          } 
          eLite.clearDisplay(0); eLite.drawPanel(r, g, b, 0, (timerHour / 10) % 10);
          eLite.clearDisplay(1); eLite.drawPanel(r, g, b, 1, timerHour % 10);
          eLite.clearDisplay(2); eLite.drawPanel(r, g, b, 2, (timerMinute / 10) % 10);
          eLite.clearDisplay(3); eLite.drawPanel(r, g, b, 3, timerMinute % 10);
          timerFirstTick = false;
        }    
      }     
    } 
  }
}

void forceClockTime() {
  if (!timerCounting) {
    tHour = rtc.now().hour();
    uint8_t tempHour = tHour;
    if (tHour >= 12) {tempHour -= 12; }
    if (tHour == 0) {tempHour = 12; } //Does not affect actual time, only what is displayed. 0 o'clock = 12 am.
    eLite.clearDisplay(0); eLite.drawPanel(r, g, b, 0, (tempHour  / 10) % 10);
    eLite.clearDisplay(1); eLite.drawPanel(r, g, b, 1, tempHour % 10);
    tMinute = rtc.now().minute();
    eLite.clearDisplay(2); eLite.drawPanel(r, g, b, 2, (tMinute  / 10) % 10);
    eLite.clearDisplay(3); eLite.drawPanel(r, g, b, 3, tMinute % 10);
    drawInfo();
  } else { //If there's a timer running
    if (timerHour == 0 && timerMinute <= 29) {
      eLite.clearDisplay(0); eLite.drawPanel(r, g, b, 0, (timerMinute / 10) % 10);
      eLite.clearDisplay(1); eLite.drawPanel(r, g, b, 1, timerMinute % 10);
      eLite.clearDisplay(2); eLite.drawPanel(r, g, b, 2, (timerSecond / 10) % 10);
      eLite.clearDisplay(3); eLite.drawPanel(r, g, b, 3, timerSecond % 10);
      
    } else {
      eLite.clearDisplay(0); eLite.drawPanel(r, g, b, 0, (timerHour / 10) % 10);
      eLite.clearDisplay(1); eLite.drawPanel(r, g, b, 1, timerHour % 10);
      eLite.clearDisplay(2); eLite.drawPanel(r, g, b, 2, (timerMinute / 10) % 10);
      eLite.clearDisplay(3); eLite.drawPanel(r, g, b, 3, timerMinute % 10);
      
    } eLite.clearDisplay(4); eLite.drawPanel(r, g, b, 4, 2);
  }
}
void blinkDigits(short blinkTime) {
  for (uint8_t i = 0; i <= 3; i++) {
    eLite.clearDisplay(i);
  } delay(blinkTime);
}
void loop() {
  if (tSecond != rtc.now().second()) {
    tSecond = rtc.now().second();
    setClockTime();
  } //daylight savings. Going to have to hope that the date logic works can't really test it atm.
  if (rtc.now().dayOfTheWeek() == 0 && rtc.now().month() == 3 && rtc.now().day() >= 8 && rtc.now().hour() >= 2 && dst == 0) {
    dst = 1;
    rtc.adjust(DateTime(rtc.now().unixtime() + 3600)); //Sets clock forward 1 hour.
    EEPROM.write(9, 1);
  }
  if (rtc.now().dayOfTheWeek() == 0 && rtc.now().month() == 11 && rtc.now().day() >= 1 && rtc.now().hour() >= 2 && dst == 1) {
    dst = 0;
    rtc.adjust(DateTime(rtc.now().unixtime() - 3600)); //Sets clock backward 1 hour.
    EEPROM.write(9, 0);
  }

  //Quick and Easy control of the RGB values of the clock.
  redInput.update(); //ResponiveAnalogRead requires this. This removes noise from the dials. Necessary so the ForceClockTime function isn't constantly being called and causing flickering
  greenInput.update();
  blueInput.update();
  
  if (redInput.hasChanged()) { r = redInput.getValue(); forceClockTime(); }//Serial.println("red");}
  if (greenInput.hasChanged()) { g = greenInput.getValue(); forceClockTime(); }// Serial.println("green"); }
  if (blueInput.hasChanged()) { b = blueInput.getValue(); forceClockTime(); }//Serial.println("blue"); }

  if (!digitalRead(alarmButton)) { 
    
    timerToBuzz = false;
    alarmToBuzz = false;
    Serial.println("alarmButton");
    analogWrite(buzzer, 0);
    alarmPressed(alarmButton, 0);
  } //View or edit the alarm time.
  if (!digitalRead(notificationButton)) { alarmPressed(notificationButton, 1);} //View or edit the alarm time.
  
  
  //Allows a photoresistor to dim the click in dark situations. But if the button is pressed, you can switch this feature off. Leaving the clock at your average brightness setting.
  if (!digitalRead(photoBut)) {photoDown = false; delay(debounceTime);}
      if (digitalRead(photoBut) && photoDown == false) {photoDown = true; photoOn = !photoOn; EEPROM.write(5, photoOn);
        if (!photoOn) {
          eLite.setChainBrightness(255);
          EEPROM.write(5, 0);
        }
        forceClockTime();
        if (photoOn) {
          EEPROM.write(5, 1);
          if (analogRead(photoresistor) < photoThreshold) {
            photoChanged = false;
          } else { photoChanged = true; }
          Serial.println("Photoresistor Enabled");
          Serial.println(analogRead(photoresistor));
          //Serial.println(photoBrightness);
          Serial.print("\n"); 
        } else {Serial.println("Photoresistor Disabled\n");
          eLite.setChainBrightness(255);
        }
      }
  if (photoOn) {
    if (analogRead(photoresistor) < photoThreshold) {
      eLite.setChainBrightness(20); 
      if (!photoChanged) {
        //Serial.println("Its set dark!");
        photoChanged = true;
        forceClockTime();
      }
    
   } else { 
      eLite.setChainBrightness(255);
      if (photoChanged) {
        //Serial.println("Its Bright!");
        photoChanged = false;
        forceClockTime();
      }
   }
  }
  
  //A timer functionality.
  if (!digitalRead(timerPlusBut)) {Serial.println("timerPlus"); timerPressed(); drawTimer(); }
  if (!digitalRead(timerMinusBut)) {Serial.println("timerMinus"); timerPressed(); drawTimer(); }
  if (!digitalRead(timerSetBut)) {timerSetDown = false; delay(debounceTime);}
      if (digitalRead(timerSetBut) && timerSetDown == false) {timerSetDown = true; timerPressed(); }
  //Serial.println(analogRead(photoresistor));


  //Alarms
  if (alarmTime[0].state) { //ALARM
    if (tHour == alarmTime[0].alarmHour && tMinute == alarmTime[0].alarmMin && alarmTime[0].beenRung == false) { //requires a flag to be false before the alarm buzzing can be enabled that way it does not keep buzzing if the user turns off the alarm.
      alarmToBuzz = true;
      alarmFirstBeepTime = rtc.now().unixtime(); //In case the millis() number overflows to zero, im referencing unix time. Most other instances can use 
                                     //millis() (such as beep patterns) because if millis() overflows, the pattern will skip one measure. If millis() overflows for this check, the alarm will not turn off
                                     //if no one is there to attend it. The neighbours would not like that. The alarm has to turn off after 20 minutes.
      alarmTime[0].beenRung = true;
      Serial.println("beginning alarm beeping");
      alarmBuzzerTimeUndefined = true;
  }
  } if (tMinute-1 == alarmTime[0].alarmMin && alarmTime[0].beenRung == true) { //Waits 1 minute before letting the flag reset, thus requiring 24 hours to pass before alarm buzzing can be enabled again.
    alarmTime[0].beenRung = false;                                             //This event has to occur [outside of //if(alarmTime[0].state)//] even if the alarm is disabled because the alarms state could potentially be disabled
    Serial.println("resetting alarm variables");                               //before the reset, and then if the alarm is reenabled within 24 hours, the buzzer will not activate because the flags haven't been set.
  }
///////////////
  if (alarmTime[1].state) { //NOTIFICATION
    if (tHour == alarmTime[1].alarmHour && tMinute == alarmTime[1].alarmMin && alarmTime[1].beenRung == false) { //requires a flag to be false before the alarm buzzing can be enabled that way it does not keep buzzing if the user turns off the alarm.
      Serial.println("beginning notif beeping");
      notifBeepCount = 0;
      notifToBuzz = true;
      alarmTime[1].beenRung = true;
      notifBuzzerTimeUndefined = true;
  }
  } if (tMinute-1 == alarmTime[1].alarmMin && alarmTime[1].beenRung == true) {
    Serial.println("reset notif");
    alarmTime[1].beenRung = false;                                             
  }
  //Sets up a pulsing buzzer without using a blocking delay function because of the micronctrollers single thread.
  if (timerToBuzz) {
    if (rtc.now().unixtime() - timerFirstBeepTime >= 90) { //After 90 seconds the timer stops beeping.
      timerToBuzz = false;
      forceClockTime();
      Serial.println("timer will now cease");
    }
    Serial.println("buzz timer");
    if (timerBuzzerTimeUndefined == true) { 
      timerToBuzzStartTime = millis(); 
      timerBuzzerTimeUndefined = false; }
    if ((millis() - timerToBuzzStartTime) >= 1 && (millis() - timerToBuzzStartTime <= 500) ) {
      analogWrite(buzzer, 125);
    }
    if ((millis() - timerToBuzzStartTime) > 500 && (millis() - timerToBuzzStartTime <= 1000) ) {
      analogWrite(buzzer, 0);
    } if ((millis() - timerToBuzzStartTime) > 1000) {
      timerToBuzzStartTime = millis();
    }
  } //Snooze function
  
  if (alarmToBuzz) {
    if (rtc.now().unixtime() - alarmFirstBeepTime >= 1800) { //1800 seconds = 30 minutes. after 30 minutes the alarm will cease beeping.
      
      alarmToBuzz = false;
      forceClockTime();
      Serial.println("alarm will now cease.");
      analogWrite(buzzer, 0);
    }
    if (!digitalRead(snoozeBut)) {snoozeDown = false; delay(debounceTime);}
        if (digitalRead(snoozeBut) && snoozeDown == false) {
          Serial.println("starting snooze");
          snoozeDown = true;
          toSnooze = true;
          snoozeStartTime = millis();
          analogWrite(buzzer, 0);
          forceClockTime();
        }
    if (toSnooze) {
      if (millis() - snoozeStartTime >= 300000) { //5 minutes = 300000
        Serial.println("should continue beeping");
        toSnooze = false;
        alarmBuzzerTimeUndefined = true;
      }
    }
  } 
  if (alarmToBuzz == true && toSnooze == false) {
    Serial.println("atoBuzz");
    if (alarmBuzzerTimeUndefined == true) { alarmToBuzzStartTime = millis(); alarmBuzzerTimeUndefined = false; }
    if ((millis() - alarmToBuzzStartTime) >= 1 && (millis() - alarmToBuzzStartTime) <= 300 ) {
      analogWrite(buzzer, 255);
      eLite.clearChain();
    }
    if ((millis() - alarmToBuzzStartTime) > 300 && (millis() - alarmToBuzzStartTime) <= 600 ) {
      analogWrite(buzzer, 0);
      forceClockTime();
      
    } if ((millis() - alarmToBuzzStartTime) > 600) {
      Serial.println("settting start time");
      alarmToBuzzStartTime = millis();
    }
  }
  if (notifToBuzz) {
    if (notifBeepCount < 2) { //Notification should only beep twice before stopping itself.
      //Serial.println("test1");
      if (notifBuzzerTimeUndefined) { notifToBuzzStartTime = millis(); notifBuzzerTimeUndefined = false;}
      //Serial.println("will buzz notif");
      if (((millis() - notifToBuzzStartTime) >= 1) && (millis() - notifToBuzzStartTime <= 150) ) {
        analogWrite(buzzer, 30);
        //Serial.println("on");
      }
      if (((millis() - notifToBuzzStartTime) > 150) && (millis() - notifToBuzzStartTime <= 300) ) {
        analogWrite(buzzer, 0);
        //Serial.println("off");
        
      } if ((millis() - notifToBuzzStartTime) > 300) {
        notifToBuzzStartTime = millis();
        notifBeepCount++; 
      }
    } if (notifBeepCount == 2) { //After the second beep, reset the variables.
      //Serial.println("reset notif variables");
      notifBeepCount = 0;
      notifToBuzz = false;
      notifBuzzerTimeUndefined = false;
      //analogWrite(buzzer, 0);
    }
  }

  if (!digitalRead(notifOnBut)) {notifDown = false; delay(debounceTime);} //Switches the notification state when the button is pressed.
    if (digitalRead(notifOnBut) && notifDown == false) {notifDown = true; alarmTime[1].state = !alarmTime[1].state; EEPROM.write(8, alarmTime[1].state); drawInfo(); }
  
  
  if (!digitalRead(alarmOnBut)) { //sets the alarm state to the value of the flip switch.
    if (alarmTime[0].state == false) {
      alarmTime[0].state = true;
      drawInfo();
      //Serial.println("alarm State = true");
    }
  } else {
    if (alarmTime[0].state == true) {
      alarmTime[0].state = false;
      alarmToBuzz = false;//Also turns off the alarm. Easy way to shut it off.
      analogWrite(buzzer, 0);
      forceClockTime();
      //Serial.println("alarm State = false");
      drawInfo();
    }
  }
  /*if (!notifToBuzz && !alarmToBuzz && !timerToBuzz) {
    analogWrite(buzzer, 0);
  }
*/

}








