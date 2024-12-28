#include <config.h>

// constants
const int OLED_WIDTH = 128;    // OLED display width, in pixels
const int OLED_HEIGHT = 64;    // OLED display height, in pixels
const int OLED_RESET = -1;     // Reset pin # (or -1 if sharing Arduino reset pin)
const int OLED_ADDRESS = 0x3C; // Use I2cScanner to find the address value
const int BITMAP_HEIGHT = 14;  // Standard bitmap height
const int BITMAP_WIDTH = 14;   // Standard bitmap width
const unsigned char BITMAP_BLUETOOTH[] PROGMEM = {
    0x01, 0x00, 0x01, 0x80, 0x01, 0xc0, 0x19, 0x60, 0x0d, 0x60, 0x07, 0xc0, 0x03, 0x80, 0x03, 0x80,
    0x07, 0xc0, 0x0d, 0x60, 0x19, 0x60, 0x01, 0xc0, 0x01, 0x80, 0x01, 0x00};
const unsigned char BITMAP_BULB[] PROGMEM = {
    0x07, 0x00, 0x18, 0xc0, 0x32, 0x60, 0x24, 0x20, 0x28, 0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x40,
    0x08, 0x80, 0x0f, 0x80, 0x00, 0x00, 0x0f, 0x80, 0x0f, 0x80, 0x07, 0x00};

// variables used inside project
BLESerial bleSerial;                                               // BLE serial emulation
String problemMessage = "";                                        // BLE buffer message
String humanReadableProblemMessage = "";                           // Problem human readable message
bool problemMessageStarted = false;                                // Start indicator of problem message
bool problemMessageEnded = false;                                  // End indicator of problem message
String confMessage = "";                                           // BLE buffer conf message
bool confMessageStarted = false;                                   // Start indicator of conf message
bool confMessageEnded = false;                                     // End indicator of conf message
bool ledAboveHoldEnabled = false;                                  // Enable the LED above the hold if possible
int bitmapMoonState = 0;                                           // Used to set the Moon Logo
bool bitmapBleState = false;                                       // Used to set the BLE bitmap
bool bitmapNeoPixelState = false;                                  // Used to set the Bulb bitmap
bool bleConnected = false;                                         // Ble connected state
bool setupState = false;                                           // Setup in progress
String oledText[] = {"", "", "", "", "", ""};                      // Oled text buffer
int oledTextIndex = -1;                                            // Current index of oled text buffer
unsigned long previousMillisBle = 0;                               // Last time BLE bitmap was updated
unsigned long previousMillisMoon = 0;                              // Last time Moon logo was updated
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET); // Oled screen object
CRGB leds[ledsCount * NEOPIXEL_LED_OFFSET];                        // Neopixel leds use by FastLED

// colors definitions
CRGB red = CRGB(255, 0, 0);
CRGB green = CRGB(0, 255, 0);
CRGB blue = CRGB(0, 0, 255);
CRGB cyan = CRGB(0, 128, 128);
CRGB magenta = CRGB(128, 0, 128);
CRGB yellow = CRGB(128, 128, 0);
CRGB pink = CRGB(120, 50, 85);
CRGB purple = CRGB(105, 0, 150);
CRGB black = CRGB(0, 0, 0);
CRGB white = CRGB(255, 255, 255);

/**
 * @brief Return the string coordinates for a position as "X12" where X is the column letter and 12 the row number
 *
 * @param position
 * @return String
 */
String positionToCoordinates(int position)
{
    String res = "";
    int rows = boardRows;
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
void neoPixelShowHold(char holdType, int holdPosition)
{
    bitmapNeoPixelState = true;
    Serial.print("[SERIAL] Light hold: ");
    Serial.print(holdType);
    Serial.print(", ");
    Serial.print(holdPosition);

    String colorLabel = "BLACK";
    CRGB colorRgb = black;

    switch (holdType)
    {
    case 'E':
        colorLabel = "RED";
        colorRgb = red;
        break;
    case 'F':
        colorLabel = "CYAN";
        colorRgb = cyan;
        break;
    case 'L':
        colorLabel = "PURPLE";
        colorRgb = purple;
        break;
    case 'M':
        colorLabel = "PINK";
        colorRgb = pink;
        break;
    case 'P':
        colorLabel = "BLUE";
        colorRgb = blue;
        break;
    case 'R':
        colorLabel = "BLUE";
        colorRgb = blue;
        break;
    case 'S':
        colorLabel = "GREEN";
        colorRgb = green;
        break;
    }
    Serial.print(" color = ");
    Serial.print(colorLabel);
    Serial.print(", coordinates: ");
    String coordinates = positionToCoordinates(holdPosition);
    Serial.print(coordinates);
    humanReadableProblemMessage.concat(coordinates);
    humanReadableProblemMessage.concat(' ');

    // Ligth Hold
    leds[holdPosition * NEOPIXEL_LED_OFFSET] = colorRgb;

    // Find the LED position above the hold
    if (ledAboveHoldEnabled)
    {
        int ledAboveHoldPosition = holdPosition;
        int gapLedAbove = 0;
        int rows = boardRows;
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
            leds[ledAboveHoldPosition * NEOPIXEL_LED_OFFSET] = white;
            leds[ledAboveHoldPosition * NEOPIXEL_LED_OFFSET].subtractFromRGB((1 - NEOPIXEL_BRIGHTNESS_ABOVE_HOLD) * 255);
        }
    }

    Serial.println();
    FastLED.show();
}

/**
 * @brief Update the oled screen
 *
 */
void oledRefresh()
{
    if (!OLED_ENABLED)
        return;

    // Compute blink values
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillisBle >= 400)
    {
        previousMillisBle = currentMillis;
        bitmapBleState = !bitmapBleState;
    }
    if (currentMillis - previousMillisMoon >= 250)
    {
        previousMillisMoon = currentMillis;
        bitmapMoonState = bitmapMoonState == 0 ? 10 : bitmapMoonState - 1;
    }

    // Clear display
    oled.clearDisplay();

    //-----------------------------------------------------------------
    // Actionbar area
    //-----------------------------------------------------------------

    // Moon logo
    int moonX, moonY, moonR;
    int moonX1, moonX2;
    double phase = (double)bitmapMoonState / 10.0;
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
    oled.println(boardName);

    // Bluetooth bitmap
    if ((bitmapBleState || bleConnected) && setupState)
        oled.drawBitmap(128 - BITMAP_WIDTH, 0, BITMAP_BLUETOOTH, BITMAP_WIDTH, BITMAP_HEIGHT, 1);

    // Bulb bitmap
    if (bitmapNeoPixelState)
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
void oledPrintln(String str, bool useLastLine = false)
{
    Serial.println("[OLED] " + str);
    if (!OLED_ENABLED)
        return;

    if (!useLastLine)
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
    }
    oledText[oledTextIndex] = str.substring(0, 20);
    oledRefresh();
}

/**
 * @brief Turn off all LEDs
 *
 */
void neoPixelReset()
{
    FastLED.clear();
    FastLED.show();
    bitmapNeoPixelState = false;
}

/**
 * @brief Process the configuration message
 *
 */
void processConfMessage()
{
    Serial.println("[SERIAL] -----------------");
    Serial.print("[SERIAL] Configuration message: ");
    Serial.println(confMessage);

    if (confMessage.indexOf("~D") != -1)
    {
        Serial.println("[SERIAL] Display an additional led above each hold");
        ledAboveHoldEnabled = true;
    }

    if (confMessage.indexOf("~Z*") != -1)
    {
        Serial.println("[SERIAL] Reset leds");
        neoPixelReset();
    }
}

/*
 * Examples of received BLE messages:
 *    - "~Z*"                                     (reset leds)
 *    - "~D,M*l#S69,S4,P82,P8,P57,P49,P28,E54#"   (v2, display hold aboce = true)
 *    - "l#S69,S4,P93,P81,P49,P28,P10,E54#"       (v1)
 *    - "~M*l#S103,E161,L115,R134,F150,M133#"    (v2, display hold above = false)
 *
 * First message part (separator = '#') is the configuration part (v2)
 *    - "~D,M*1" : display light above and below holds = true
 *    - "~M*1" : display light above and below holds = falsetrue
 *    - "1" : light the the problem holds
 *
 * Second message part (separator = '#') is the problem string separated by ','
 *    - format: "S12,P34,...,E56"
 *    - where alpha char:
 *          - S = starting hold
 *          - P = intermediate hold
 *          - E = ending hold
 *          - L = left hold
 *          - R = right hold
 *          - M = match hold
 *          -F = foot hold
 *    - where the following numbers are the LED position on the strip
 */

/**
 * @brief Process the BLE message to light the matching LEDs
 *
 */
void processProblemMessage()
{
    Serial.println("[SERIAL] -----------------");
    Serial.print("[SERIAL] Problem message: ");
    Serial.println(problemMessage);
    oledPrintln("Coordinates:");

    humanReadableProblemMessage = ' ';
    int indexComma1 = 0;
    int indexComma2 = 0;
    while (indexComma2 != -1)
    {
        indexComma2 = problemMessage.indexOf(',', indexComma1);
        String holdMessage = problemMessage.substring(indexComma1, indexComma2);
        indexComma1 = indexComma2 + 1;

        char holdType = holdMessage[0];                      // holdType is the first char of the string
        int holdPosition = holdMessage.substring(1).toInt(); // holdPosition start at second char of the string
        neoPixelShowHold(holdType, holdPosition);            // light the hold on the board
    }
    ledAboveHoldEnabled = false;
    oledPrintln(humanReadableProblemMessage);
}

/**
 * @brief Check LEDs by cycling through the colors red, green, blue and then turning the LEDs off again
 *
 */
void neoPixelCheck()
{
    CRGB colors[] = {red, green, blue};
    int fadeDelay = 5;

    if (NEOPIXEL_CHECK1_AT_BOOT)
    {
        bitmapNeoPixelState = true;

        // light each leds one by one
        for (int indexColor = 0; indexColor < sizeof(colors) / sizeof(CRGB); indexColor++)
        {
            for (int i = 0; i < ledsCount; i++)
            {
                leds[i * NEOPIXEL_LED_OFFSET] = colors[indexColor];
                FastLED.show();
                delay(fadeDelay);
                oledRefresh();
            }
        }
        neoPixelReset();
        oledRefresh();
    }

    if (NEOPIXEL_CHECK2_AT_BOOT)
    {
        bitmapNeoPixelState = true;

        // blink each color
        for (int indexColor = 0; indexColor < sizeof(colors) / sizeof(CRGB); indexColor++)
        {
            delay(fadeDelay * 100);
            for (int indexLed = 0; indexLed < ledsCount; indexLed++)
                leds[indexLed * NEOPIXEL_LED_OFFSET] = colors[indexColor];
            FastLED.show();
            delay(fadeDelay * 100);
            neoPixelReset();
            oledRefresh();
        }
    }
}

int coordinatesToPosition(char column, int row)
{
    int rows = boardRows;
    char columns[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K'};
    int columnIndex = -1;
    for (int i = 0; i < sizeof(columns) / sizeof(char); i++)
    {
        if (columns[i] == column)
        {
            columnIndex = i;
            break;
        }
    }

    int position = 0;
    if (columnIndex % 2 == 0) // even column
        position = (columnIndex * rows) + row - 1;
    else if (columnIndex % 2 == 1) // odd column
        position = (columnIndex * rows) + (rows - row);

    // Serial.print("[COORDINATES 2 POSITION] -----------------");
    // Serial.print(column);
    // Serial.print(row);
    // Serial.print(" -> ");
    // Serial.println(position);
    // // sleep(1);

    return position;
}

/**
 * @brief Check LEDs by cycling through the colors red, green, blue and then turning the LEDs off again
 *
 */
void neoPixelChristmas()
{
    CRGB colors[] = {red, green, blue};
    int fadeDelay = 5;

    for (int j = 0; j < 2; j++)
    {

        for (int i = 0; i < ledsCount; i++)
        {
            leds[i * NEOPIXEL_LED_OFFSET] = j == 0 ? red : black;
        }

        leds[coordinatesToPosition('F', 12) * NEOPIXEL_LED_OFFSET] = green;

        leds[coordinatesToPosition('E', 11) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('F', 11) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('G', 11) * NEOPIXEL_LED_OFFSET] = green;

        leds[coordinatesToPosition('D', 10) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('E', 10) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('F', 10) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('G', 10) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('H', 10) * NEOPIXEL_LED_OFFSET] = green;

        leds[coordinatesToPosition('E', 9) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('F', 9) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('G', 9) * NEOPIXEL_LED_OFFSET] = green;

        leds[coordinatesToPosition('D', 8) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('E', 8) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('F', 8) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('G', 8) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('H', 8) * NEOPIXEL_LED_OFFSET] = green;

        leds[coordinatesToPosition('C', 7) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('D', 7) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('E', 7) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('F', 7) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('G', 7) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('H', 7) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('I', 7) * NEOPIXEL_LED_OFFSET] = green;

        leds[coordinatesToPosition('D', 6) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('E', 6) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('F', 6) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('G', 6) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('H', 6) * NEOPIXEL_LED_OFFSET] = green;

        leds[coordinatesToPosition('C', 5) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('D', 5) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('E', 5) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('F', 5) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('G', 5) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('H', 5) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('I', 5) * NEOPIXEL_LED_OFFSET] = green;

        leds[coordinatesToPosition('B', 4) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('C', 4) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('D', 4) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('E', 4) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('F', 4) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('G', 4) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('H', 4) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('I', 4) * NEOPIXEL_LED_OFFSET] = green;
        leds[coordinatesToPosition('J', 4) * NEOPIXEL_LED_OFFSET] = green;

        leds[coordinatesToPosition('E', 3) * NEOPIXEL_LED_OFFSET] = CRGB(86, 43, 5);
        leds[coordinatesToPosition('F', 3) * NEOPIXEL_LED_OFFSET] = CRGB(86, 43, 5);
        leds[coordinatesToPosition('G', 3) * NEOPIXEL_LED_OFFSET] = CRGB(86, 43, 5);

        leds[coordinatesToPosition('E', 2) * NEOPIXEL_LED_OFFSET] = CRGB(86, 43, 5);
        leds[coordinatesToPosition('F', 2) * NEOPIXEL_LED_OFFSET] = CRGB(86, 43, 5);
        leds[coordinatesToPosition('G', 2) * NEOPIXEL_LED_OFFSET] = CRGB(86, 43, 5);

        FastLED.show();
        delay(fadeDelay * 100);
    }

    // if (NEOPIXEL_CHECK1_AT_BOOT)
    // {
    //     bitmapNeoPixelState = true;

    //     // light each leds one by one
    //     for (int indexColor = 0; indexColor < sizeof(colors) / sizeof(CRGB); indexColor++)
    //     {
    //         for (int i = 0; i < ledsCount; i++)
    //         {
    //             leds[i * NEOPIXEL_LED_OFFSET] = colors[indexColor];
    //             FastLED.show();
    //             delay(fadeDelay);
    //             oledRefresh();
    //         }
    //     }
    //     neoPixelReset();
    //     oledRefresh();
    // }

    // if (NEOPIXEL_CHECK2_AT_BOOT)
    // {
    //     bitmapNeoPixelState = true;

    //     // blink each color
    //     for (int indexColor = 0; indexColor < sizeof(colors) / sizeof(CRGB); indexColor++)
    //     {
    //         delay(fadeDelay * 100);
    //         for (int indexLed = 0; indexLed < ledsCount; indexLed++)
    //             leds[indexLed * NEOPIXEL_LED_OFFSET] = colors[indexColor];
    //         FastLED.show();
    //         delay(fadeDelay * 100);
    //         neoPixelReset();
    //         oledRefresh();
    //     }
    // }
}

/**
 * @brief Initialization
 *
 */
void setup()
{
    Serial.begin(115200);

    // oled setup
    if (OLED_ENABLED)
    {
        oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
        oled.cp437(true); // Use full 256 char 'Code Page 437' font
        oled.clearDisplay();
        oledRefresh();
    }

    // ble setup
    oledPrintln("OK| BLE init");
    bleSerial.begin(bleName);

    // NeoPixel setup
    FastLED.addLeds<WS2811, NEOPIXEL_PIN>(leds, ledsCount * NEOPIXEL_LED_OFFSET);
    FastLED.setBrightness(NEOPIXEL_BRIGHTNESS * 255);
    FastLED.clear();
    FastLED.show();

    oledPrintln("..| LEDS check");
    // neoPixelCheck();
    oledPrintln("OK| LEDS check", true);
    oledPrintln("OK| Setup");
    oledPrintln("Waiting APP");

    setupState = true;
}

/**
 * @brief Infinite loop processed by the chip
 *
 */
void loop()
{
    bleConnected = bleSerial.connected();
    oledRefresh();

    if (bleConnected)
    {
        while (bleSerial.available())
        {
            char c = bleSerial.read();
            // Serial.println(c);

            // detect string delimiters
            switch (c)
            {
            case '~':
                confMessageStarted = true;
                break;
            case '*':
                confMessageEnded = true;
                break;
            case '#':
                if (!problemMessageStarted)
                    problemMessageStarted = true;
                else
                    problemMessageEnded = true;
                break;
            default:
                break;
            }

            // conf message
            if (confMessageStarted)
            {
                confMessage.concat(c);
            }
            if (confMessageEnded)
            {
                processConfMessage();
                confMessage = "";
                confMessageStarted = false;
                confMessageEnded = false;
            }

            // problem message
            if (problemMessageStarted && (c != '#'))
            {
                problemMessage.concat(c);
            }
            if (problemMessageEnded)
            {
                neoPixelReset();
                processProblemMessage();
                problemMessage = "";
                problemMessageStarted = false;
                problemMessageEnded = false;
            }
        }
    }

    neoPixelChristmas();
}
