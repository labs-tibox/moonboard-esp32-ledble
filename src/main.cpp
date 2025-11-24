#include <config.h>

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
unsigned long previousMillisBle = 0;                               // Last time BLE bitmap was updated
unsigned long previousMillisMoon = 0;                              // Last time Moon logo was updated
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
    Serial.print(", position: ");
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
    Serial.print(" color: ");
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
            }
        }
        neoPixelReset();
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

    // ble setup
    Serial.println("[SERIAL] OK| BLE init");
    bleSerial.begin(bleName);

    // NeoPixel setup
    FastLED.addLeds<WS2811, NEOPIXEL_PIN>(leds, ledsCount * NEOPIXEL_LED_OFFSET);
    FastLED.setBrightness(NEOPIXEL_BRIGHTNESS * 255);
    FastLED.clear();
    FastLED.show();

    Serial.println("[SERIAL] ..| LEDS check");
    neoPixelCheck();
    Serial.println("[SERIAL] OK| LEDS check");
    Serial.println("[SERIAL] OK| Setup");
    Serial.println("[SERIAL] Waiting APP");

    setupState = true;
}

/**
 * @brief Infinite loop processed by the chip
 *
 */
void loop()
{
    bleConnected = bleSerial.connected();

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
}
