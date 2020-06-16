# ESP32-Remote-Rotor-Control
This add on can be used either stand alone, or as a wintest rotor interface. It is work in progress.

The hardware you need is an ESP32 Wrover, and a relay board. I used the HL-525 from Reichelt (or AliExpress)
https://www.reichelt.nl/ontwikkelaarsraden-relaismodule-2-kanaals-5-v-srd-05vdc-sl-debo-relais-2ch-p242810.html?PROVID=2788&gclid=CjwKCAjw26H3BRB2EiwAy32zhU72tswg4FBrAR0KWsQmJR1nNtFcYFlWGHHX8jrAQR5pAAK0ftkyDBoCavoQAvD_BwE&&r=1

Install the sketch using and Arduino IDE 1.8.12. Make sure youlocate the files as follows:
Create a directory called data in the root directory of the sketch

/webrotor.ini
/data/bootstrap.min.css
/data/gauge.min.js
/data/jquery.slim.min.js
/data/index.html

A number of javascripts and css files are taken from the internet, so in case you like to have them locally because you are not connected to the internet, download them and put them in this data directory and chanhe the index.html file accordingly

16-06-2020
Erik, PA0ESH
