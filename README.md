# Edge Lit Clock

An Atmega328p powered clock using acryllic panels and ws2812b addressable LEDs as displays.
#
![Front Render](https://github.com/ArmaniKorsich/Edge-Lit-Clock/blob/master/Images/Clock%20Views.png)

## CAD

All drawings created in Inventor 2016.

The clock consists of 5 seperate displays chained together, with:
* 3
* 10
* 6
* 10
* 6*

panels each. Each panel has a number from 0-9 etched in the following order:  
![Numerals](https://github.com/ArmaniKorsich/Edge-Lit-Clock/blob/master/Images/Numerals.png)

The off order resembles the old Nixie tube number ordering meant to maximize visiblity of all the digits.

\* *The last display has various symbols instead:*  
![alpha Symbols](https://github.com/ArmaniKorsich/Edge-Lit-Clock/blob/master/Images/AlphaChar.png)

## Electronics

The Atmega328p drives the WS2812B leds using the eLite and FastLED libraries.

The Atmega328p reads time from a DS32321 RTC.  

To protect time information against power outages, I selected a DS3231 which accepts backup battery voltage on PIN14 in the event that VCC cuts out.

The circuit was constructed on one 2 layer circuit board manufactured by JLCPCB and designed using Autodesk Eagle.

### Input

The device takes 9-40VDC input with at least 2A current capacity. The LM2575-5V buck converter transforms the input to 5V with a 3A limit.

The average current draw of the Edge Lit CLock is ~350 mA with an average of 16 WS2812B activated at any moment.

## Schematics

![Front Render](https://github.com/ArmaniKorsich/Edge-Lit-Clock/blob/master/Images/Schematic.png)

## Source

This project takes advantage of Arduino's public library system, using 
stdio, FastLED ResponsiveAnalaogRead, EEPROM, Wire, RTCLib
and the provided eLite library for controlling the displays.

ResponsiveAnalogRead: Provides fast and mathematically continuous input values. Minimizes noise in low-cost potentiometers.

stdio: Optional, provides useful debugging information in Arduino IDE.

FastLED: A solution to controlling a large quantity of LEDs rapidly and arbitrarily.

RTClib: Interfaces with the DS3231 Real Time Clock, functions for timekeeping.  

## eLite Library

The eLite library represents the bulk of the content of this project, the clock and physical shape of the project are an example applying this library as my entry to Eisenhower Senior High School's 'Yearly Engineering and Innovation Design Scholarship.'    

`uint8_t defineChain(uint8_t numDisplays,...);`. 
> Inputs(NumDisplays): Number of chained displays in the system.    
> return(chainLength): returns the number of displays in the system.

`void defineLedPerPanel(uint8_t led,...);`. 
> Inputs(led): How many displays there are  
> Inputs(,...): The number of leds per panel in a display, seperated by commas.  
> Tells the system how many LEDs each display has. Useful with physically large plexiglass panels that need more lighting.  
> For a system with 5 displays and 2 leds per panel on each display:  
`void defineLedPerPanel(5, 2, 2, 2, 2, 2)`. 

`void setStandardOrder(uint8_t displayToConfigure);`. 
> inputs(displayToConfigure): Which display to configure. 
> Resets the numercal order of panels in a given display to 0, 1, 2, ... , n. 

`void redefinePanelOrder(uint8_t displayToConfigure, uint8_t quantityPanels,...);`. 
> Inputs(displayToConfigure): Which display to configure   
> Inputs(QuantityPanels): Quantity of panels in display 'DisplayToConfigure'   
> Inputs(,...): The custom order of panels in the display, expects 'QuantityPanels' items, seperated by commas.  

`void drawPanel(uint8_t r, uint8_t g, uint8_t b, uint8_t displayCount, int panelToDraw);`. 
> Inputs(R, G, B): 8-bit integer (0-255), sets the primary color of the displays.     
> Inputs(displayCount): Which display to draw to.  
> Inputs(panelToDraw): The panel to activate in color ##RRGGBB   

`void drawChain(uint8_t r, uint8_t g, uint8_t b, uint8_t lengthOfChain,...);`. 
> Inputs(R, G< B): 8-bit integer (0-255), sets the primary color of the displays.  
> Inputs(lengthOfChain): Number of displays to activate in the chain (0-indexed).  
> inputs(,...): The panel number to activate in each display, seperated by commas.   
 
 
`void cycleDisplays(int delayTime, int acceleration, bool direc, uint8_t r, uint8_t g, uint8_t b, uint8_t qtyToCycle,...);`. 
> Inputs(delayTime): Pause between advancing the panels (mS).  
> Inputs(acceleration): Time to decrese delayTime by after each iteration. (mS).  
> Inputs(direc): Cycles forwards (1) or backwards (0).   
> Inputs(R, G, B): 8-bit integer (0-255), sets the primary color of the displays.   
> Inputs(qtyToCycle): Number of displays in the chain to cycle through.   
> Inputs(,...): The index of the displays you wish to cycle through.  
> In a chain of 5 displays, if we wish to cycle the 1st, 3rd, and 4th display:  
`void cycleDisplays(delay, acceleration, forward, R, G, B, 3, 0, 2, 3)`.

`void clearDisplay(uint8_t displayToClear);`
> Inputs(displayToClear): Which display to clear, all panels will be deactivated.  

`void clearChain();`
> Turns off all LEDs in the system, clears all displays.

`uint8_t setChainBrightness(uint8_t brightness);`
> Inputs(brightness): 8-bit integer (0-255), sets the overall opacity (brightness) of the LEDs in FastLED.
> return(brightnessLevel): returns the brightness of the LEDs.

`void randomizeDisplays(int delayTime, int acceleration, int iterations, uint8_t r, uint8_t g, uint8_t b, uint8_t qtyToRandomize, ...);`
> Inputs(delayTime): Pause between iterations in milliseconds (mS).    
> Inputs(acceleration): Time to decrease delayTime by after each iteration. (mS).   
> Inputs(iterations): How many times to activate a random panel on the given displays.    
> Inputs(R, G, B): 8-bit integer (0-255), sets the primary color of the displays.   
> Inputs(qtyToRandomize): Number of displays in the chain to randomize.  
> Inputs(,...): The index of the displays you wish to randomize.  
> If iterations is 1, delay and acceleration aren't important, as the given displays will simply output a random panel. Otherwise, this function provides a simple animation to your chain.  


