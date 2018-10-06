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
![Numerical](https://github.com/ArmaniKorsich/Edge-Lit-Clock/blob/master/Images/Numerical.png)  

The off order resembles the old Nixie tube number ordering meant to maximize visiblity of all the digits.

*The last display has various symbols instead:*  
![alpha Symbols](https://github.com/ArmaniKorsich/Edge-Lit-Clock/blob/master/Images/AlphaChar.png)

Using the Atmega328p reading unix time from a DS32321 time-keeping IC.  
To protect the information against power outages, the project uses a readily available DS3231 breakout board with CR2032 3.0v coin battery.
