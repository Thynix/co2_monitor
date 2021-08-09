# CO2 Monitor

Imagine the [OLED CO2 Monitor](https://github.com/adafruit/Adafruit_SCD30/tree/master/examples/oled_co2_monitor) SCD30 example code, but with more polish.

## Features

* Reliable startup (Delaying helps; I don't know why yet. Voltage stabilization?)
* Prose description of CO2 level
* Fahrenheit conversion
* NeoPixel CO2 level color codes

The NeoPixel brightens if the color just changed. Toggle the display and
NeoPixel with button A and button B.

## Hardware

* [SCD30 CO2 sensor](https://github.com/adafruit/Adafruit_SCD30)
* [Adafruit 128x32 OLED](https://www.adafruit.com/product/2900)
* [Adafruit QT PY SAMD21](https://www.adafruit.com/product/4600)
