#include "BLESerial.h"
#include <Arduino.h>
#include <NeoPixelBus.h>

// constants
const int BOARD_STANDARD = 0;
const int BOARD_MINI = 1;
#define COLOR_SATURATION 255

// custom settings
int board = BOARD_STANDARD;           // Define the board type : mini or standard (to be changed depending of board type used)
const int LED_OFFSET = 2;             // Light every "LED_OFFSET" LED of the LEDs strip
const uint8_t PIXEL_PIN = 2;          // Use pin D2 of Arduino Nano 33 BLE (to be changed depending of your pin number used)
const bool CHECK_LEDS_AT_BOOT = true; // Test the led sysem at boot if true

// variables used inside project
int ledsByBoard[] = {200, 150};                                                  // LEDs: usually 150 for MoonBoard Mini, 200 for a standard MoonBoard
int rowsByBoard[] = {18, 12};                                                    // Rows: usually 12 for MoonBoard Mini, 18 for a standard MoonBoard
String namesByBoard[] = {"Moonboard Standard", "Moonboard Mini"};                // Names of moonboards
BLESerial bleSerial;                                                             // BLE serial emulation
String bleMessage = "";                                                          // BLE buffer message
bool bleMessageStarted = false;                                                  // Start indicator of problem message
bool bleMessageEnded = false;                                                    // End indicator of problem message
uint16_t leds = ledsByBoard[board];                                              // Number of LEDs in the LED strip (usually 150 for MoonBoard Mini, 200 for a standard MoonBoard)
NeoPixelBus<NeoRgbFeature, Neo800KbpsMethod> strip(leds *LED_OFFSET, PIXEL_PIN); // Pixel object to interact withs LEDs

// colors definitions
RgbColor red(COLOR_SATURATION, 0, 0);                           // Red color
RgbColor green(0, COLOR_SATURATION, 0);                         // Green color
RgbColor blue(0, 0, COLOR_SATURATION);                          // Blue color
RgbColor violet(COLOR_SATURATION / 2, 0, COLOR_SATURATION / 2); // Violet color
RgbColor black(0);                                              // Black color
RgbColor white(COLOR_SATURATION);

/**
 * @brief Return the string coordinates for a position as "X12" where X is the column letter and 12 the row number
 *
 * @param position
 * @return String
 */
String positionToCoordinates(int position)
{
  String res = "";
  int rows = rowsByBoard[board];
  char columns[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K'};
  int column = (position / rows) + 1;
  int row = 0;
  res.concat(columns[column - 1]);

  if (column % 2 == 0) // even column
    row = rows - (position % rows);
  else if (column % 2 == 1) // odd column
    row = (position % rows) + 1;

  res.concat(row);
  return res;
}

/**
 * @brief Light the LEDs for a given hold
 *
 * @param holdType Hold type (S,P,E)
 * @param holdPosition Position of the mathcing LED
 * @param ledAboveHoldEnabled Enable the LED above the hold if possible
 */
void lightHold(char holdType, int holdPosition, bool ledAboveHoldEnabled)
{
  Serial.print("Light hold: ");
  Serial.print(holdType);
  Serial.print(", ");
  Serial.print(holdPosition);

  String colorLabel = "BLACK";
  RgbColor colorRgb = black;

  switch (holdType)
  {
  case 'S':
    colorLabel = "GREEN";
    colorRgb = green;
    break;
  case 'P':
    colorLabel = "BLUE";
    colorRgb = blue;
    break;
  case 'E':
    colorLabel = "RED";
    colorRgb = red;
    break;
  }
  Serial.print(" color = ");
  Serial.print(colorLabel);
  Serial.print(", coordinates: ");
  Serial.print(positionToCoordinates(holdPosition));

  // Ligth Hold
  strip.SetPixelColor(holdPosition * LED_OFFSET, colorRgb);
  strip.Show();

  // Find the LED position above the hold
  if (ledAboveHoldEnabled)
  {
    int ledAboveHoldPosition = holdPosition;
    int gapLedAbove = 0;
    int rows = rowsByBoard[board];
    int cell = holdPosition + 1;
    int column = (cell / rows) + 1;

    if ((cell % rows == 0) || ((cell - 1) % rows == 0)) // start or end of the column
      gapLedAbove = 0;
    else if (column % 2 == 0) // even column
      gapLedAbove = -1;
    else if (column % 2 == 1) // odd column
      gapLedAbove = 1;
    else
      gapLedAbove = 9;

    if (gapLedAbove != 0 && gapLedAbove != 9)
    {
      ledAboveHoldPosition = holdPosition + gapLedAbove;
      Serial.print(", led position above: ");
      Serial.print(ledAboveHoldPosition);

      // Light LED above hold
      strip.SetPixelColor(ledAboveHoldPosition * LED_OFFSET, violet);
    }
  }

  Serial.println();
}

/**
 * @brief Process the BLE message to light the matching LEDs
 *
 */
void processBleMesage()
{
  /*
   * Example of received BLE messages:
   *    "~D*l#S69,S4,P82,P8,P57,P49,P28,E54#"
   *    "l#S69,S4,P93,P81,P49,P28,P10,E54#"
   *
   * first message part (separator = '#')
   *    - "~D*1" : light 2 LEDs, the selected hold and the LED above it
   *    - "1" : light the the selected hold
   *
   * second message part (separator = '#') is the problem string separated by ','
   *    - format: "S12,P34, ... ,E56"
   *    - where S = starting hold, P = intermediate hold, E = ending hold
   *    - where the following numbers are the LED position on the strip
   */

  Serial.println("--------------------");
  Serial.print("Message: ");
  Serial.println(bleMessage);
  Serial.println();

  int indexHashtag1 = 0;
  int indexHashtag2 = 0;
  bool ledAboveHoldEnabled = false;

  // explode the message with char '#'
  while ((indexHashtag2 = bleMessage.indexOf('#', indexHashtag1)) != -1)
  {
    String splitMessage = bleMessage.substring(indexHashtag1, indexHashtag2);
    indexHashtag1 = indexHashtag2 + 1;

    if (splitMessage[0] == 'l') // process conf part of the ble message
    {
      ledAboveHoldEnabled = false;
    }
    else if (splitMessage[0] == '~') // process conf part of the ble message
    {
      ledAboveHoldEnabled = true;
    }
    else // process the problem part of the ble message
    {
      int indexComma1 = 0;
      int indexComma2 = 0;
      while (indexComma2 != -1)
      {
        indexComma2 = splitMessage.indexOf(',', indexComma1);
        String holdMessage = splitMessage.substring(indexComma1, indexComma2);
        indexComma1 = indexComma2 + 1;

        char holdType = holdMessage[0];                         // holdType is the first char of the string
        int holdPosition = holdMessage.substring(1).toInt();    // holdPosition start at second char of the string
        lightHold(holdType, holdPosition, ledAboveHoldEnabled); // light the hold on the board
      }
    }
  }
}

/**
 * @brief Turn off all LEDs
 *
 */
void resetLeds()
{
  strip.ClearTo(black);
  strip.Show();
}

/**
 * @brief Check LEDs by cycling through the colors red, green, blue, violet and then turning the LEDs off again
 *
 */
void checkLeds()
{
  if (CHECK_LEDS_AT_BOOT)
  {
    RgbColor colors[] = {red, green, blue, violet};
    int fadeDelay = 25;

    // light each leds one by one
    for (int indexColor = 0; indexColor <= 3; indexColor++)
    {
      strip.SetPixelColor(0, colors[indexColor]);
      strip.Show();
      delay(fadeDelay);
      for (int i = 0; i < leds; i++)
      {
        strip.ShiftRight(1 * LED_OFFSET);
        strip.Show();
        delay(fadeDelay);
      }
    }
    resetLeds();

    // blink each color
    for (int indexColor = 0; indexColor <= 3; indexColor++)
    {
      delay(fadeDelay * 50);
      for (int indexLed = 0; indexLed < leds; indexLed++)
        strip.SetPixelColor(indexLed * LED_OFFSET, colors[indexColor]);
      strip.Show();
      delay(fadeDelay * 50);
      resetLeds();
    }
  }
}

/**
 * @brief Initialization
 *
 */
void setup()
{
  Serial.begin(115200);
  char bleName[] = "MoonBoard A";
  bleSerial.begin(bleName);

  strip.Begin();
  strip.Show();

  checkLeds();

  Serial.println("-----------------");
  Serial.print("Initialization completed for ");
  Serial.println(namesByBoard[board]);
  Serial.println("Waiting for the mobile app to connect ...");
  Serial.println("-----------------");
}

/**
 * @brief Infinite loop processed by the chip
 *
 */
void loop()
{
  if (bleSerial.connected()) // do something only if BLE connected
  {
    while (bleSerial.available()) // loop until no more data available
    {
      // read first char
      char c = bleSerial.read();

      // message state
      if (c == '#' && !bleMessageStarted) // check start delimiter
        bleMessageStarted = true;
      else if (c == '#' && bleMessageStarted) // check end delimiter
        bleMessageEnded = true;

      // construct ble message
      bleMessage.concat(c);

      // process message if at the end of the message
      if (bleMessageEnded)
      {
        resetLeds();
        processBleMesage();

        // reset BLE data
        bleMessageEnded = false;
        bleMessageStarted = false;
        bleMessage = "";
      }
    }
  }
}
