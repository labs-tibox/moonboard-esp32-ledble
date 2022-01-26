# OLED Documentation

## Hardware
 - Oled 0.96
 - Resolution: 128x64
 - Type: SSD1306

## Pins wiring
  -  (D)21 => SDA
  -  (D)22 => SCL
  -  3v3 => VCC
  -  GND => GND

## Settings:
  - The screen I used had the SCREEN_ADDRESS = 0x3C
  - I used i2cScanner sktech found here: https://playground.arduino.cc/Main/I2cScanner/

## Bitmap conversion

Just use https://javl.github.io/image2cpp/
 - Invert image colors = true
 - Code output format = Arduino code

## Sources
 - https://projetsdiy.fr/ssd1306-mini-ecran-oled-i2c-128x64-arduino/
 - https://randomnerdtutorials.com/esp32-ssd1306-oled-display-arduino-ide/
