# ESP32-Remote-Rotor-Control

Required:

- ESP32 Wrover or similar board
- HS-52S relay board. The ESP is programmed for a HIGH output as OFF function.

When pressing the CW button, the CW relay closes and if wired up correctly, your rotor turns Counterclockwise.
This is only possible if the brake switch is OFF.
The same applies for the Counterclockwise button.

In case the CW button is ON, and you press CCW, first the CW relays will be released, before the CCW is engaged.
In case you press the brake and either CW or CCW is still on, then they will first be released, and after 1 second the brake will be put one.

In case the rotor bearing is 1 or 359 degrees, the auto stop function is called, and the rotor stops. Both CW and CCW switches are set to OFF. This is still rather primitive, but better than nothing. Tested on a Kenpro KR400RC and worked

In case you are turning CW and you decide to reverse to CCW, the programme will first terminate CW and then turn on CCW.

Switches are showing spinners, indication the rotor is turning. They are not (yet) momentary switches. CSS and HTML is in the making.

In case you have a rotor brake and apply this break, first any turning is stopped and then the brake is applied. From then on you cannot switch CW or CCW on, until the brake is released.


# Compass indicator

Every rotor to be used should have a build in potentiometer of approx. 500 Ohm.
Connect one side to GND and the other side to the 3.3V of the ESP32. Connect the middle pin to pin 34 of the ESP32.
The values of the ADC1 are then converted from 0-4096, into 0 - 360 degrees.
Since the ADC is not 100% linear, you may have to put some small resistors from GND to the potentiometer as well as from 3.3V to the potentiometer. This is still WIP on my side/

A small capacitor of 1nF helps to remove some noise but you may experiment with other values. Currently I have a 1 nF and a 10uF in parallel connected to pun 34 (the ADC channels used for the bearing)


# In the making:

- Sending output values to the serial port for connection to other smart rotors.
- interface with ham radio like programmes.
- Tell back from the antenna to the ESP using another ESP32 and a 9-sensor board, telling
  - real bearing
  - elevation
  - temperature
 - PCB with all components on it.
  
And finally, still work to do on the page lay-out.
By the way, it is very simple to put a cheap 3,5 touchscreen to the ESP and make local control possible as well.
There are plenty of free switches available for controlling a lot of other stuff.



27-06-2020

Erik, PA0ESH
