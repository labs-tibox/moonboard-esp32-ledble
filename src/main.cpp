#include "BLESerial.h"
#include <Arduino.h>

// constants
const int c_boardStandard = 0;
const int c_boardMini = 1;

// custom settings
int board = c_boardStandard; // Define the board type : mini or standard (to be changed depending of board type used)
const int c_ledOffset = 1;   // Use every "c_ledOffset" LED on the string

// variables used by the project

// variables used inside project
int ledsByBoard[] = {200, 150}; // Led: usually 150 for MoonBoard Mini, 200 for a standard MoonBoard
int rowsByBoard[] = {18, 12};   // Rows: usually 12 for MoonBoard Mini, 18 for a standard MoonBoard
BLESerial bleSerial;
String bleMessage = "";
bool bleMessageStarted = false;
bool bleMessageEnded = false;

/**
 * @brief Initialization
 *
 */
void setup()
{
  Serial.begin(9600);
  char bleName[] = "MoonBoard A";
  bleSerial.begin(bleName);
  Serial.println("-----------------");
  Serial.println("Waiting for the mobile app to connect ...");
  Serial.println("-----------------");
}

/**
 * @brief Light the LEDs for a Hold
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

  String color = "BLACK";

  switch (holdType)
  {
  case 'S':
    color = "GREEN";
    break;
  case 'P':
    color = "BLUE";
    break;
  case 'E':
    color = "RED";
    break;
  }
  Serial.print(" color = ");
  Serial.print(color);

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
      Serial.print(", led position above : ");
      Serial.print(ledAboveHoldPosition);
    }
  }

  Serial.println();
}

/**
 * @brief process the BLE message to light the matching LEDs
 *
 */
void processBleMesage()
{
  /*
   * Example off received BLE messages:
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
   *    - where the following numbers are the LED position in the string
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
 * @brief Infinite loop processed by the chip
 *
 */
void loop()
{
  if (bleSerial.connected()) // do something only if BLE connected
  {
    while (bleSerial.available())
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

      // process message if end detected
      if (bleMessageEnded)
      {
        processBleMesage();

        // reset data
        bleMessageEnded = false;
        bleMessageStarted = false;
        bleMessage = "";
      }
    }
  }
}
