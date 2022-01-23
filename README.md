# BikeGuard
Arduino-based bike security system.

## Research
When I came to Winnipeg, I saw multiple bikes without one or two wheels, no seats.
![](img/IMG_20170828_1257027.jpg)

I laughed as hard as I shouldn't. Back at home bikes are stolen as a whole, not piece by piece.

Then I bought myself a new bike:

![](img/IMG_20170828_0922434.jpg)

A few years later, I have been a victim of bike theft and vandalism twice. 
 
![](img/IMG_20170926_0922414.jpg)
![](img/IMG_20190422_2007566.jpg)

Unfortunately, there was nothing I can do to stop theft from happening and the police cannot do anything either.

I started research on modern security systems, like the anti-theft system in cars and the security system installed in public and private properties. They all share the same functionality: actively looking for anomalies. 

In the case of a car anti-theft system, the system is actively looking for the presence of all doors and latches that can gain access to the car, including doors, windows, engine lid, and fuel panel. When the door has been unlocked and opened unexpectedly, the alarm will sound. If the car can communicate with wireless towers, the car will send it's location and the detail including what went wrong to the owner. This applies to the security system installed in properties, too.

Back to the bike side, there is a company called [Sherlock](https://www.sherlock.bike/en/) developed a stealth GPS tracker. It is almost perfect until someone takes wheels and the seat off from the bike, or if the thief can't take parts off your bike, they will ruin it. 

Sometimes vandalism is worse than getting your whole bike stolen. In Canada, most of the insurance has a deductible of $500. Usually, the value of a bike wheel should be well below that. But with my own experience, in case of only one wheel was stolen, it's very hard to find a replacement one, ultimately that bike will become a write-off.

Having an effective vandalism alarm can help to prevent vandalism and theft from success.

## Development

### Hardware Platform
The system is based on the Arduino platform. It is both easy and familiar to program and has multiple form factors of boards. In this case, Arduino Nano or MKR would be ideal. Arduino as a programming language also supports STM32 as an alternative backend for further development.

### Components
The system should have a battery, an Arduino board, a few probe wires with fuses to protect the Arduino board, a sounding alarm, and preferably a BLE, GPS and 4G shield (not implemented yet due to lack of hardware). These probe wires can run through anywhere on the bike, e.g. front and rear wheel, bike seat, etc. The battery, board, and fuses are designed to be installed on or inside the bike frame in a weather and tamper-proof box. 

### Software Logic 
In normal mode, the board actively pooling for the presence of the probe pins. If one or more probe wires are out for over 2 seconds, the alarm will be activated, and the state of alarm will be written to non-volatile memory in case of loss of power.

In alarm mode, an auditory alarm will sound, and detailed alarm information will be printed to the LCD, including when and which part of the bike has lost. If GPS and 4G module is present, the location of the bike and the alarm will be sent via Internet or text message.

### Implementation details
The prototype program consists following parts: compile-time configuration, setup, main loop, state management, state retention and alarm.

The compile-time configuration tells the program which pin is used and where it connects to. 

Arduino Uno has 1024 bytes of EEPROM available, and it can hold its data between power cycles. The first byte of the EEPROM is used to store the state of the probing wires, and updated to the EEPROM when the alarm is tripped. The next 4 bytes were to record the last conscious time in microseconds reported by `millis()`, and updated once in a while to reduce the wear on the EEPROM. This was implemented to replace the lack of RTC chip on Arduino Uno. Every 4 bytes after represents the tripped time for certain probe wire, and it's updated when the alarm is tripped.

The setup function will read the last known state from EEPROM or load the default state to reinitiate the system as required.

Throughout the program, a few operations needs to be applied to every enabled probe pins. This includes setting up the correct pinmode, restoring states from EEPROM, probing pins, starting timers. The routine for looping the enabled pins is the same for every operation, so a function called `forEachEnabledProbePin()` is here to abstract the loop routine, and the actions to be looped can be passed in as a lambda with 2 parameters, the index of the pin and the pin number.

An I2C LCD screen is used instead of normal LCD. I2C only needs 2 wires on the analog pin socket while the oridinary LCD would use up almost all digital pin socket on the board. 


## Prototype and Reality
Thanks to the never-ending pandemic, sourcing suitable and reasonably priced parts has never been harder than before. So the prototype here can't do much other than it's base functionality.

### Prototype Usage 
- Modify compile-time configuration at the beginning of `BikeGuard.ino`. 

    The `probePinouts` and the `probePinsDescription` array have the same length and one-to-one correspondence. For example, `probePinouts[0] = 5` and `probePinsDescription[0] = "Seat"` means the probe wire for the bike seat is assigned to pin 5. The alarm will be activated once it's been removed from the bike. 

- Assemble the board with an LCD screen, buzzer, reinitiate button, and probe wires.
    ![](img/Breadboard.jpg)

    Or if you put it on a real bike, it will roughly be like this:
    ![](img/mockup.jpg)

- Upload the program to Arduino.

### Prototype Demo
Video: https://1drv.ms/v/s!AsQIPTVMm5jyrYJxPdakP_ZY_XlX1w

Before the first boot, make sure the probe wires (orange wires above) are properly connected, then boot the Arduino up by plugging in power. The LCD screen should be showing "System normal." after a few seconds.

Try to pull one of the probe wires and quickly put it back, nothing should happen because the prototype has a poor connection tolerance of 2 seconds.

Try to pull one of the probe wires and wait for a few seconds, the buzzer should start buzzing non-stop and the LCD screen should say "Something has been stolen sometime ago!". The time should be incrementing and updating by itself every second. 

Try the above with other probe wires. The LCD screen should be alternating between different probe wires and their respective time of the accident.

Try plugging the wires back, the alarm won't be stopped in this way because the alarm state is designed to be remembered.

Try pulling the power of the Arduino board, obviously the alarm should stop because there is no power. But as soon as the power is back, the alarm (including the time counter) will continue, thanks to EEPROM that recorded the information on the activation of alarm. 

To completely "reset" the prototype, hold the reinitialization switch, then press the reset button on the Arduino board. Release the reinitialization switch once "System Normal" is displayed on the LCD.

## Testing
The program went through several improvements, from being able to remember the state of alarm before power loss, to being able to display the time when and which part of the bike has been stolen.

The program was fairly simple. However, the LCD screen I used wasn't so cooperating. The I2C LCD library I used have a function called `println()`, and I used it just like any other `println()` function in C# or Java. But it didn't put the cursor into the next line, instead it printed 2 garbage characters that suppose to be CRLF. I digged into the source code of the library and found out the `println()` function was simple `print()` with CRLF in the end. And the LCD screen doesn't understand CRLF, so it printed 2 garbage characters. 

I also tried quite a few ways to implements how to display things in a good format, and avoid refreshing content with the same content, I ended up using another state indicator that indicates does the screen needs to be refreshed. 

The LCD screen also has a strange behaviour when something longer than it's column is passed in. I did quite a lot experiment, and ended up using the first row to display the part name, second row to indicate the stolen state, and the third row to display time.


## Personal Refelctions
I love cycling and I love my bike. I can still remember how hard I cried at the night which my bike got stolen. It's the same feeling that if my brother got kidnapped and the police won't help.

After finishing this prototype, I found that no alarm system can completely stop theft and vandalism. The system will have no idea if the criminal cut most of the spoke on the wheel, and the system can be easily destroyed by applying an arbitrarily high voltage on any probe wire. It is also possible that the box holding the board could be destroyed altogether. Surely we can improve it by running more probe wires around everywhere in the bike, making more durable boxes. But just like a property or a vehicle can be victims from time to time, no security system is perfect and invincible either. 

I asked the police where did the stolen bikes go, they said they are sold in black markets for as low as $10, whereas the financial loss incurred on me was magnitude more than that. They also said criminals who did this are also likely drug abusers, and they are stealing anything they can to source money to buy more drugs.

Ultimately, this is a human and society problem, a big problem that technology cannot solve and a big problem that everyone should take their part to solve. It isn't a one-off psycho trying to be exciting about committing petty crime, but a soul ruined by drugs ruining other's life.

Theft and vandalism on bikes are no different from theft and vandalism on vehicles or properties. But what's different is vehicles and properties have insurance as a financial backup when such incident happens, while bike don't. If a bike alarm system like this wants to be successful on the market, a bundled insurance plan would be necessary. 

It is also important that there should be more guarded bike shelters around the city so cyclists can stop worrying about their bikes got stolen all the time when they need to leave their bikes temporarily.