# ESP32-Remote-Rotor-Control
This add on can be used either stand alone, or as a wintest rotor interface. It is work in progress.

The hardware you need is an ESP32 Wrover, and a relay board. I used the HL-525 from Reichelt (or AliExpress)
https://www.reichelt.nl/ontwikkelaarsraden-relaismodule-2-kanaals-5-v-srd-05vdc-sl-debo-relais-2ch-p242810.html?PROVID=2788&gclid=CjwKCAjw26H3BRB2EiwAy32zhU72tswg4FBrAR0KWsQmJR1nNtFcYFlWGHHX8jrAQR5pAAK0ftkyDBoCavoQAvD_BwE&&r=1

Install the sketch using and Arduino IDE 1.8.12. Make sure you locate the files as follows:
Create a directory called data in the root directory of the sketch

/webrotor.ini

/data/bootstrap.min.css

/data/gauge.min.js

/data/jquery.slim.min.js

/data/index.html

A number of javascripts and css files are taken from the internet, so in case you like to have them locally because you are not connected to the internet, download them and put them in this data directory and chanhe the index.html file accordingly.

Functionality

When pressing the CW button, the CW relais closes and if wired up correctly, your rotor turns Counter Clockwise.
This is only possible if the brake switch is OFF.
The same applies for the Counter Clock Wise button.

In case the CW button is ON, and you press CCW, first the CW relais will be released, before the CCW is engaged.
In case you press the brake and either CW or CCW is still on, then they will first be released, and after 1 second the brake will be put one.


Compass indictator

Every rotor has a build in potentiometer of approx 500 Ohm.
Connect one side to GND and the other side to the 3.3V of the ESP32. Connect the middle pin to pin 34 of the ESP32.
The values of the ADC1 are then converted from 0-4096, into 0 - 360 degreees.
Since the ADC is not 100% lineair, you may have to put some small resistors from GND to the potentiometer as well as from 3.3V to the potentiometer. This is still WIP on my side/

A small capacitor og 1nF helps to remove some noise.
The algoritm for the compas is looking for changes more than 3 degrees, before the compass is turning. Every 60 seconds, the real value is sent, so drifting in the wind will result in the compass rose slowly following.

In the making:

- Auto stop when reaching 0 or 360 degrees.
- Sending outut values to the serial port for connection to other smart rotors.
- interface with hamradio like programmes.
- 
- Tell back from the antenna to the ESP using another ESP32 and a 9 sensor borad, telling
  - real bearing
  - elevation
  - temp
  
  




16-06-2020

Erik, PA0ESH
