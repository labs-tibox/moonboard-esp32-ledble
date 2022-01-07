#include "BLESerial.h"
#include <Arduino.h>

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
  Serial.println('-----------------');
  Serial.println('Waiting for the mobile app to connect ...');
  Serial.println('-----------------');
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

  // TODO find the LED position above the hold !

  Serial.println(color);
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
