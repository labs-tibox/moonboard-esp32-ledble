#include "BLESerial.h"
#include <Arduino.h>

BLESerial bleSerial;
String bleMessage = "";
bool bleMessageStarted = false;
bool bleMessageEnded = false;
bool lightLedAboveHoldEnabled = false;

void setup()
{
  Serial.begin(9600);
  char bleName[] = "MoonBoard A";
  bleSerial.begin(bleName);
}

void lightHold(char holdType, int holdPosition)
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
  Serial.println(color);
}

void processBleMesage()
{
  /*
   * Example off received messages:
   *
   * ~D*l#S69,S4,P82,P8,P57,P49,P28,E54#
   * l#S69,S4,P93,P81,P49,P28,P10,E54#
   */
  Serial.println("--------------------");
  Serial.print("Message: ");
  Serial.println(bleMessage);

  int index1 = 0;
  int index2 = 0;

  while ((index2 = bleMessage.indexOf('#', index1)) != -1)
  {
    String splitMessage = bleMessage.substring(index1, index2);
    index1 = index2 + 1;

    if (splitMessage[0] == 'l')
    {
      lightLedAboveHoldEnabled = false;
      Serial.println("Light led above hold enabled: FALSE");
    }
    else if (splitMessage[0] == '~')
    {
      lightLedAboveHoldEnabled = true;
      Serial.println("Light led above hold enabled: TRUE");
    }
    else
    {
      int index3 = 0;
      int index4 = 0;
      while (index4 != -1)
      {
        index4 = splitMessage.indexOf(',', index3);
        String holdMessage = splitMessage.substring(index3, index4);
        index3 = index4 + 1;

        char holdType = holdMessage[0];
        int holdPosition = holdMessage.substring(1).toInt();
        lightHold(holdType, holdPosition);
      }
    }
  }
}

void loop()
{
  if (bleSerial.connected())
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
