#include <EEPROM.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
inline int high(int pin) { return digitalRead(pin) == HIGH; }
inline int low(int pin) { return digitalRead(pin) == LOW; }
inline int EEPROM_PROBE_PIN_TIMEOUT_OFFSET(int i) { return 5 + 4 * i; }

///////////// Compile-time Configuration /////////////
const int BUZZER_PIN = 11;  // Pin to buzzer
const int REINIT_PIN = 12;  // Pin to re-initiate to untripped state

// 0 = disabled, number = pinout 
const uint8_t probePinouts[8] = 
{
    5,
    6,
    7,
    0, 
    0,
    0,
    0,
    0
};

String probePinsDescription[8] = 
{
    "Seat",
    "Front Wheel",
    "Rear Wheel",
    "",
    "",
    "",
    "",
    ""
};
/////////////////////////////////////////


volatile uint8_t state;  // state of alarm, one bit represents one probe
unsigned long probePinsTimeout[8] = {};  // 
LiquidCrystal_I2C lcd(0x27, 20, 4);  // I2C 20x4 LCD
bool lcdContentChanged = true;
inline void refreshDisplay() { lcd.clear(); lcd.setCursor(0, 0); }
unsigned long previousMillis = 0;
inline unsigned long realMillis() { return previousMillis + millis(); }


void setup()
{
    // Prepare serial connection
    Serial.begin(9600);

    // Prepare LCD
    lcd.init();
    lcd.backlight();
    lcd.noCursor();

    // Prepare pin modes
    pinMode(REINIT_PIN, INPUT_PULLUP);
    forEachEnabledProbePin([](int _, int pin)
    {
        pinMode(pin, INPUT_PULLUP);
    });

    // EEPROM Layout
    //       [ 0 ] : probePinouts
    //       [ 1 ] : previousMillis
    // [5 + 4 * i] : probePinsTimeout[i]

    // Read EEPROM 
	state = EEPROM.read(0);
    // Serial.print("state = ");
    // Serial.println(state);

    // restore previous state
    if (state)
    {
        if (low(REINIT_PIN)) // reinit
        {
            for (int i = 0; i < 37; i++)
            {
                EEPROM.update(i, 0);
            }
            state = EEPROM.read(0);
            refreshDisplay();
            lcd.print("Reset.");
            delay(1000);
        }
        else
        {
            EEPROM_readAnything(1, previousMillis);
            // Serial.print("previousMillis = ");
            // Serial.println(previousMillis);
            forEachEnabledProbePin([](int i, int pin)
            {
                EEPROM_readAnything(EEPROM_PROBE_PIN_TIMEOUT_OFFSET(i), probePinsTimeout[i]);
                // Serial.print("probePinsTimeout[" + i);
                // Serial.print(i);
                // Serial.print("] = ");
                // Serial.println(probePinsTimeout[i]);
            });
        }
    }
}

void loop()
{
    // if tripped
    if (state)
    {
        // alarm sound forever
        tone(BUZZER_PIN, 523);

        if (realMillis() - previousMillis > 3000)
        {
            EEPROM_writeAnything(1, realMillis());
        }

        forEachEnabledProbePin([](int i, int pin)
        {
            // print details to serial and LCD
            if (1<<i & state)
            {
                String text = probePinsDescription[i];
                String stolen = "has been stolen ";
                auto timeout = realMillis() - probePinsTimeout[i];

                Serial.println(text + " " + stolen + getReadableTime(timeout) + " ago!");

                refreshDisplay();
                lcd.print(text);
                lcd.setCursor(0, 1);
                lcd.print(stolen);
                lcd.setCursor(0, 2);
                lcd.print(getReadableTime(timeout));
                lcd.print(" ago!");

                delay(1000);
            }
        });
    }
    else if (lcdContentChanged)
    {
        refreshDisplay();
        lcd.print("System normal.");
        lcdContentChanged = false;
    }
    else 
    {
        delay(500);
    }

    // record time on disconnect, reset time in case of poor pin connection
    forEachEnabledProbePin([](int i, int pin)
    {
        if (high(pin))
        {
            if (probePinsTimeout[i] == 0)
            {
                probePinsTimeout[i] = realMillis();
            }
        }
        else if ((realMillis() - probePinsTimeout[i]) < 2000)
        {
            probePinsTimeout[i] = 0;
        }
    });

    // trip alarm according to timeout
    forEachEnabledProbePin([](int i, int pin)
    {
        if (!(state & 1<<i) && realMillis() - probePinsTimeout[i] >= 2000 && probePinsTimeout[i] != 0)
        {
            state |= 1<<i;
            EEPROM_writeAnything(EEPROM_PROBE_PIN_TIMEOUT_OFFSET(i), probePinsTimeout[i]);
            EEPROM_writeAnything(1, realMillis());
        }
    });
    
    EEPROM.update(0, state);
}

// param for the lambda: (index, pinout)
void forEachEnabledProbePin(void (*action)(int, int))
{
    for (size_t i = 0; i < 8; i++)
    {
        int pin = probePinouts[i];

        if (pin != 0)
        {
            action(i, pin);
        }
    }
}

// converts miliseconds to readable time string
String getReadableTime(unsigned long ms) 
{
    unsigned long currentMillis;
    unsigned long seconds;
    unsigned long minutes;
    unsigned long hours;
    unsigned long days;

    currentMillis = ms;
    seconds = currentMillis / 1000;
    minutes = seconds / 60;
    hours = minutes / 60;
    days = hours / 24;
    currentMillis %= 1000;
    seconds %= 60;
    minutes %= 60;
    hours %= 24;

    String readableTime = "";

    if (days > 0) 
    {
        readableTime = String(days) + " ";
    }

    if (hours > 0) 
    {
        readableTime += String(hours) + ":";
    }

    if (minutes < 10) 
    {
        readableTime += "0";
    }
    readableTime += String(minutes) + ":";

    if (seconds < 10) 
    {
        readableTime += "0";
    }

    readableTime += String(seconds);
    return readableTime;
}

// credit: https://forum.arduino.cc/t/writing-numbers-bigger-than-1-byte-to-eeprom/56631/12
template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
    const byte* p = (const byte*)(const void*)&value;
    int i;
    for (i = 0; i < sizeof(value); i++)
	  EEPROM.update(ee++, *p++);
    return i;
}

// credit: https://forum.arduino.cc/t/writing-numbers-bigger-than-1-byte-to-eeprom/56631/12
template <class T> int EEPROM_readAnything(int ee, T& value)
{
    byte* p = (byte*)(void*)&value;
    int i;
    for (i = 0; i < sizeof(value); i++)
	  *p++ = EEPROM.read(ee++);
    return i;
}
