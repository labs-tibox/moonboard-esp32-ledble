#include "BLESerial.h"
#include <Arduino.h>
#include <NeoPixelBus.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// constants
#define BOARD_STANDARD 0
#define BOARD_MINI 1
#define OLED_WIDTH 128    // OLED display width, in pixels
#define OLED_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1     // Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_ADDRESS 0x3C // Use I2cScanner to find the address value
#define BITMAP_HEIGHT 14  // Standard bitmap height
#define BITMAP_WIDTH 14   // Standard bitmap width

const unsigned char BITMAP_BLUETOOTH[] PROGMEM = {
    0x01, 0x00, 0x01, 0x80, 0x01, 0xc0, 0x19, 0x60, 0x0d, 0x60, 0x07, 0xc0, 0x03, 0x80, 0x03, 0x80,
    0x07, 0xc0, 0x0d, 0x60, 0x19, 0x60, 0x01, 0xc0, 0x01, 0x80, 0x01, 0x00};

const unsigned char BITMAP_BULB[] PROGMEM = {
    0x07, 0x00, 0x18, 0xc0, 0x32, 0x60, 0x24, 0x20, 0x28, 0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x40,
    0x08, 0x80, 0x0f, 0x80, 0x00, 0x00, 0x0f, 0x80, 0x0f, 0x80, 0x07, 0x00};

// custom settings
int board = BOARD_STANDARD;                // Define the board type : mini or standard (to be changed depending of board type used)
const int NEOPIXEL_LED_OFFSET = 2;         // Light every "NEOPIXEL_LED_OFFSET" LED of the LEDs strip
const int NEOPIXEL_COLOR_SATURATION = 255; // NeoPixel default brightness
const uint8_t NEOPIXEL_PIN = 2;            // Use pin D2 (to be changed depending of your pin number used)
const bool NEOPIXEL_CHECK_AT_BOOT = true;  // Test the neo pixel led sysem at boot if true

// variables used inside project
int ledsByBoard[] = {200, 150};                                                                 // LEDs: usually 150 for MoonBoard Mini, 200 for a standard MoonBoard
int rowsByBoard[] = {18, 12};                                                                   // Rows: usually 12 for MoonBoard Mini, 18 for a standard MoonBoard
String namesByBoard[] = {"Standard", "Mini"};                                                   // Names of moonboards
BLESerial bleSerial;                                                                            // BLE serial emulation
String bleMessage = "";                                                                         // BLE buffer message
String problemMessage = "";                                                                     // Problem buffer message
bool bleMessageStarted = false;                                                                 // Start indicator of problem message
bool bleMessageEnded = false;                                                                   // End indicator of problem message
uint16_t leds = ledsByBoard[board];                                                             // Number of LEDs in the LED strip (usually 150 for MoonBoard Mini, 200 for a standard MoonBoard)
int moonState = 0;                                                                              // used to set the Moon Logo
int bleState = 0;                                                                               // used to set the BLE bitmap
int neoPixelState = 0;                                                                          // used to set the Bulb bitmap
int bleConnected = 0;                                                                           // ble connected state
int setupState = 0;                                                                             // setup in progress (0) or done (1)
String oledText[] = {"", "", "", "", "", ""};                                                   // oled text buffer
int oledTextIndex = -1;                                                                         // current index of oled text buffer
unsigned long previousMillisBle = 0;                                                            // last time BLE bitmap was updated
unsigned long previousMillisMoon = 0;                                                           // last time Moon logo was updated
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);                              // Oled screen object
NeoPixelBus<NeoRgbFeature, Neo800KbpsMethod> neoPixel(leds *NEOPIXEL_LED_OFFSET, NEOPIXEL_PIN); // Pixel object to interact withs LEDs

// colors definitions
RgbColor red(NEOPIXEL_COLOR_SATURATION, 0, 0);                                    // Red color
RgbColor green(0, NEOPIXEL_COLOR_SATURATION, 0);                                  // Green color
RgbColor blue(0, 0, NEOPIXEL_COLOR_SATURATION);                                   // Blue color
RgbColor violet(NEOPIXEL_COLOR_SATURATION / 2, 0, NEOPIXEL_COLOR_SATURATION / 2); // Violet color
RgbColor black(0);                                                                // Black color
RgbColor white(NEOPIXEL_COLOR_SATURATION);

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
void neoPixelShowHold(char holdType, int holdPosition, bool ledAboveHoldEnabled)
{
    neoPixelState = 1;
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
    String coordinates = positionToCoordinates(holdPosition);
    Serial.print(coordinates);
    problemMessage.concat(coordinates);
    problemMessage.concat(' ');

    // Ligth Hold
    neoPixel.SetPixelColor(holdPosition * NEOPIXEL_LED_OFFSET, colorRgb);
    neoPixel.Show();

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
            neoPixel.SetPixelColor(ledAboveHoldPosition * NEOPIXEL_LED_OFFSET, violet);
        }
    }

    Serial.println();
}

/**
 * @brief Update the oled screen
 *
 */
void oledRefresh()
{
    // Compute blink values
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillisBle >= 400)
    {
        previousMillisBle = currentMillis;
        bleState = bleState == 0 ? 1 : 0;
    }
    if (currentMillis - previousMillisMoon >= 250)
    {
        previousMillisMoon = currentMillis;
        moonState = moonState == 0 ? 10 : moonState - 1;
    }

    // Clear display
    oled.clearDisplay();

    //-----------------------------------------------------------------
    // Actionbar area
    //-----------------------------------------------------------------

    // Moon logo
    int moonX, moonY, moonR;
    int moonX1, moonX2;
    double phase = (double)moonState / 10.0;
    for (moonY = 0; moonY <= BITMAP_HEIGHT / 2; moonY++)
    {
        moonX = (int)(sqrt(BITMAP_HEIGHT / 2 * BITMAP_HEIGHT / 2 - moonY * moonY));
        oled.drawLine(BITMAP_HEIGHT / 2 - moonX, moonY + BITMAP_HEIGHT / 2, moonX + BITMAP_HEIGHT / 2, moonY + BITMAP_HEIGHT / 2, SSD1306_WHITE);
        oled.drawLine(BITMAP_HEIGHT / 2 - moonX, BITMAP_HEIGHT / 2 - moonY, moonX + BITMAP_HEIGHT / 2, BITMAP_HEIGHT / 2 - moonY, SSD1306_WHITE);

        // Determine the edges of the lighted part of the moon
        moonR = 2 * moonX;
        if (phase < 0.5)
        {
            moonX1 = -moonX;
            moonX2 = (int)(moonR - 2 * phase * moonR - moonX);
        }
        else
        {
            moonX1 = moonX;
            moonX2 = (int)(moonX - 2 * phase * moonR + moonR);
        }
        // Draw the lighted part of the moon
        oled.drawLine(moonX1 + BITMAP_HEIGHT / 2, BITMAP_HEIGHT / 2 - moonY, moonX2 + BITMAP_HEIGHT / 2, BITMAP_HEIGHT / 2 - moonY, SSD1306_BLACK);
        oled.drawLine(moonX1 + BITMAP_HEIGHT / 2, moonY + BITMAP_HEIGHT / 2, moonX2 + BITMAP_HEIGHT / 2, moonY + BITMAP_HEIGHT / 2, SSD1306_BLACK);
    }

    // Moonboard and type text
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(22, 0);
    oled.println("MoonBoard");
    oled.setCursor(22, 8);
    oled.println(namesByBoard[board]);

    // Bluetooth bitmap
    if ((bleState || bleConnected) && setupState)
        oled.drawBitmap(128 - BITMAP_WIDTH, 0, BITMAP_BLUETOOTH, BITMAP_WIDTH, BITMAP_HEIGHT, 1);

    // Bulb bitmap
    if (neoPixelState)
        oled.drawBitmap(128 - BITMAP_WIDTH - BITMAP_WIDTH, 0, BITMAP_BULB, BITMAP_WIDTH, BITMAP_WIDTH, 1);

    //-----------------------------------------------------------------
    // Text area
    //-----------------------------------------------------------------
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 16);

    for (size_t i = 0; i < 6; i++)
        oled.println(oledText[i]);

    // Show diplay
    oled.display();
}

/**
 * @brief Print a new line to the text area of the oled screen
 *
 * @param str String to be added
 */
void oledPrintln(String str)
{
    if (oledTextIndex < 5)
    {
        oledTextIndex++;
    }
    else
    {
        for (size_t i = 0; i < oledTextIndex; i++)
            oledText[i] = oledText[i + 1];
    }
    oledText[oledTextIndex] = str.substring(0, 20);
    oledRefresh();
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
     *    - "1" : light the selected hold
     *
     * second message part (separator = '#') is the problem string splitted by ','
     *    - format: "S12,P34, ... ,E56"
     *    - where S = starting hold, P = intermediate hold, E = ending hold
     *    - where the following numbers are the LED position on the strip
     */

    Serial.println("--------------------");
    Serial.print("Message: ");
    Serial.println(bleMessage);
    Serial.println();
    oledPrintln("Coordinates:");
    // oledPrintln(bleMessage);

    problemMessage = " ";
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

                char holdType = holdMessage[0];                                // holdType is the first char of the string
                int holdPosition = holdMessage.substring(1).toInt();           // holdPosition start at second char of the string
                neoPixelShowHold(holdType, holdPosition, ledAboveHoldEnabled); // light the hold on the board
            }
        }
    }
    oledPrintln(problemMessage);
}

/**
 * @brief Turn off all LEDs
 *
 */
void neoPixelReset()
{
    neoPixel.ClearTo(black);
    neoPixel.Show();
    neoPixelState = 0;
}

/**
 * @brief Check LEDs by cycling through the colors red, green, blue, violet and then turning the LEDs off again
 *
 */
void neoPixelCheck()
{
    if (NEOPIXEL_CHECK_AT_BOOT)
    {
        RgbColor colors[] = {red, green, blue, violet};
        int fadeDelay = 25;

        // light each leds one by one
        for (int indexColor = 0; indexColor <= 3; indexColor++)
        {
            neoPixelState = 1;
            neoPixel.SetPixelColor(0, colors[indexColor]);
            neoPixel.Show();
            delay(fadeDelay);
            oledRefresh();
            for (int i = 0; i < leds; i++)
            {
                neoPixel.ShiftRight(1 * NEOPIXEL_LED_OFFSET);
                neoPixel.Show();
                delay(fadeDelay);
                oledRefresh();
            }
        }
        neoPixelReset();
        oledRefresh();

        // blink each color
        for (int indexColor = 0; indexColor <= 3; indexColor++)
        {
            for (size_t i = 0; i < 50; i++)
            {
                delay(fadeDelay);
                oledRefresh();
            }
            neoPixelState = 1;

            for (int indexLed = 0; indexLed < leds; indexLed++)
                neoPixel.SetPixelColor(indexLed * NEOPIXEL_LED_OFFSET, colors[indexColor]);
            neoPixel.Show();
            for (size_t i = 0; i < 50; i++)
            {
                delay(fadeDelay);
                oledRefresh();
            }
            neoPixelReset();
            oledRefresh();
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

    oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
    oled.cp437(true); // Use full 256 char 'Code Page 437' font
    oled.clearDisplay();
    oledRefresh();

    oledPrintln("OK| BLE init");

    char bleName[] = "MoonBoard A";
    bleSerial.begin(bleName);

    neoPixel.Begin();
    neoPixel.Show();

    oledPrintln("..| LEDS check");
    neoPixelCheck();
    oledText[oledTextIndex] = "OK| LEDS check";
    oledRefresh();

    Serial.println("-----------------");
    Serial.print("Initialization completed for ");
    Serial.println(namesByBoard[board]);
    Serial.println("Waiting for the mobile app to connect ...");
    Serial.println("-----------------");
    oledPrintln("OK| Setup");
    oledPrintln("Waiting APP");

    setupState = 1;
}

/**
 * @brief Infinite loop processed by the chip
 *
 */
void loop()
{
    bleConnected = bleSerial.connected();
    oledRefresh();

    if (bleConnected) // do something only if BLE connected
    {
        while (bleSerial.available()) // loop until no more data available
        {
            // read first char
            char c = bleSerial.read();
            // Serial.write(c);

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
                // Serial.write('\n');
                neoPixelReset();
                processBleMesage();

                // reset BLE data
                bleMessageEnded = false;
                bleMessageStarted = false;
                bleMessage = "";
            }
        }
    }
}
