# Moonboar ESP32 LED BLE: A Simple MoonBoard BLE LED System

This project started as a fork of the projet [Moonboard LED on Arduino 33 BLE](https://github.com/FabianRig/ArduinoMoonBoardLED) to add support for the ESP32 chip but now lives it's own live because the code was almost completely rewritten and library incompatiblity.

This project aims at providing an easy to use solution for building your own MoonBoard LED system. It is both compatible with a normal MoonBoard as well as with the MoonBoard Mini. You need an ESP32 Devkit, a WS2811 LED string with 25 cm wire length (or 15cm wire with the double of strings), and an appropriate power source.

If you want a product that just works, please buy the one offered by Moon Climbing! This is a project which requires some work and still might not work as well as the original. To be absolutely clear: This project is provided as-is. I take absolutely no responsibility that it works as expected. In fact, it might break at any time. You have been warned!

## Thanks
All the heavy lifting in this project is done by two awesome libraries: NeoPixelBus (for the LED string) and [BLESerial](https://github.com/iot-bus/BLESerial) (for BLE functionality on ESP32). They make it possible to keep this project quite short, easy to understand, and easily maintainable.

Thanks to the two following projects for inspiration and proving this kind of project was doable on Arduino chip:
- [Moonboard LED sytem on Arduino NRF52](https://github.com/e-sr/moonboard_nrf52)
- [Moonboard LED on Arduino 33 BLE](https://github.com/FabianRig/ArduinoMoonBoardLED)

## How to use
1. Download and install Visual Studio Code.
2. Install PlatformIO in Visual Studio Code.
3. Download and open this project.
4. Adjust to your needs (LED mapping, LED offset, LED pin, Moonboard type).
5. Compile and flash to an Arduino Nano 33 BLE.
6. Use the MoonBoard app to connect to the Arduino and show the problems on your board!

## LED Mapping
The most common LED wiring pattern (here for a MoonBoard Mini) goes like this (front view):
- start bottom left (A1),
- up the column (to A12),
- one column to the right (to B12),
- all the way down (to B1),
- one column to the right (to C1),
- and repeat.

The MoonBoard App encodes holds in the same way. Hold A1 is 0, hold A2 is 1, hold A3 is 2 and so on.


## Good to know
- Wiring: Usually, blue is GND/negative, brown is positive, yellow/green is data. Please double-check! It might be a good idea to use a resistor (e.g. 330 ohms) in the data line!
- Never power the Arduino only when it's connected to the LED string without powering the LED string! This might destroy the first LED!
- The Arduino does not need to be shutdown, you can simply unplug the power source! This is (at least for me) a big improvement when compared to a Raspberry Pi based solution.
