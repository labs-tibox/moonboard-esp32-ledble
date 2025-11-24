#include <Arduino.h>
#include <BLESerial.h>
#include <FastLED.h>
#include <SPI.h>
#include <Wire.h>

// Select the board type your are using (comment / uncomment) the matching line below
// #define MOONBOARD_STANDARD
#define MOONBOARD_MINI

// custom settings
const int NEOPIXEL_LED_OFFSET = 2;                // Light every "NEOPIXEL_LED_OFFSET" LED of the LEDs strip
const uint8_t NEOPIXEL_PIN = 2;                   // Use pin D2 (to be changed depending of your pin number used)
const bool NEOPIXEL_CHECK1_AT_BOOT = true;        // Check1 the neo pixel led sysem at boot if true (led by led)
const bool NEOPIXEL_CHECK2_AT_BOOT = true;        // Check2 the neo pixel led sysem at boot if true (blink each led)
const float NEOPIXEL_BRIGHTNESS = 0.8;            // Neopixel brightness setting (0 to 1)
const float NEOPIXEL_BRIGHTNESS_ABOVE_HOLD = 0.1; // Neopixel brightness setting (0 to 1)
char bleName[] = "MoonBoard";                     // Bluetooth name displayed by the esp32 BLE

// Constants
#if defined(MOONBOARD_MINI)
const uint16_t ledsCount = 150; // Leds count
String boardName = "Mini";      // Moonboard name
int boardRows = 12;             // Rows count: 12 for MoonBoard Mini
#elif defined(MOONBOARD_STANDARD)
const uint16_t ledsCount = 200; // Leds count
String boardName = "Standard";  // Moonboard name
int boardRows = 18;             // Rows count: 18 for MoonBoard Standard
#endif
