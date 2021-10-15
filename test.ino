#include <EEPROM.h>
inline int high(int pin) { return digitalRead(pin) == HIGH; }
inline int low(int pin) { return digitalRead(pin) == LOW; }
// #define p(x) Serial.print(x)
// #define pl(x) Serial.println(x)
// #define pv(x) Serial.print(#x); Serial.print(" = "); Serial.print(x);
// #define pvl(x) Serial.print(#x); Serial.print(" = "); Serial.println(x);
// #define pee() Serial.print("EEPROM[0] = "); Serial.println(EEPROM.read(0), BIN);

///////////// Configuration /////////////
const int BUZZER_PIN = 13;
const int REINIT_PIN = 12;

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

// Synced with
volatile uint8_t state;
unsigned long probePinsTimeout[8] = {};


void setup()
{
    // pl("############################################");

    // Prepare serial connection
    Serial.begin(9600);

    // Prepare pin modes
    pinMode(REINIT_PIN, INPUT_PULLUP);
    forEachEnabledProbePin([](int _, int pin)
    {
        pinMode(pin, INPUT_PULLUP);
    });

    // Read EEPROM 
	state = EEPROM.read(0);
    // pee();
    // pvl(state);
}

void loop()
{
    // pl("////////////////////////////////////////////");
    delay(1000);
    // if tripped
    if (state != 0)
    {
        // p("tripped");
        if (low(REINIT_PIN))  // reinit
        {
            // p("reinit eeprom update");
            EEPROM.update(0, 0);
            state = EEPROM.read(0);
            // pee();
            // pv(state);
        }
        else  // report which pin was tripped throug serial
        {
            // p("alarming forever");
            EEPROM.update(0, state);
            tone(BUZZER_PIN, 523);
            for(;;)
            {
                forEachEnabledProbePin([](int i, int pin)
                {
                    if (1<<i & state)
                    {
                        Serial.println(probePinsDescription[i] + " has been stolen!");
                    }
                });
                Serial.println();
            }
        }
    }

    // start timer on disconnect, reset timer in case of poor pin connection
    forEachEnabledProbePin([](int i, int pin)
    {
        if (high(pin))
        {
            if (probePinsTimeout[i] == 0)
            {
                probePinsTimeout[i] = millis();
            }
        }
        else
        {
            probePinsTimeout[i] = 0;
        }
    });
    

    forEachEnabledProbePin([](int i, int pin)
    {
        if (millis() - probePinsTimeout[i] > 2000 && probePinsTimeout[i] != 0)
        {
            // p("pin ");
            // p(i);
            // pl(" time's up");
            // pvl(state);
            // pee();
            state |= 1<<i;
            // pvl(state);
            // pee();
        }
    });
    

    delay(1000);
}

void forEachEnabledProbePin(void (*action)(int, int))
{
    for (size_t i = 0; i < 8; i++)
    {
        int pin = probePinouts[i];

        if (pin != 0)
        {
            // p("For ");
            // pv(i)
            // p(", ");
            // pvl(pin);

            action(i, pin);
        }
    }
}


