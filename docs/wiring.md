# Wiring

The firmware defines the pins and their levels. The relay driver, the LED resistors and the
contact assignment below are one way to wire it.

## Pins used

| Arduino pin | Direction | Function |
|---|---|---|
| 4 | output | Changeover relay (SPDT): HIGH = pot A, LOW = pot B |
| 5 | output | Status LED B (high while the relay selects pot B) |
| 6 | output | Status LED A (high while the relay selects pot A) |
| 7 | input, pull-up | Optional restart jumper/button to GND (read once at power-up) |

The two LED pins are always driven complementary, so exactly one LED is lit at a time,
showing which pot is currently powered. Pin 7 can stay unconnected: then the timer always
resumes after a power loss. Held to GND while switching on, it starts fresh at pot A.

## Wiring sketch

The MCU cannot switch the cooking load directly, so pin 4 drives a relay coil (through a
transistor/driver) and the relay's SPDT contact routes the one 230 V socket (about 2.5 kW)
to either pot A or pot B. At 2.5 kW the contact carries about 11 A at 230 V AC, so the
relay must be rated for at least that (a 16 A / 250 V AC changeover relay leaves margin).
Mains wiring belongs in a closed, earthed enclosure and may only be done by a qualified
person according to the local regulations.

```
                         +--- Common (C) ----< 230 V socket, max. ~2.5 kW (L; N goes straight to both cookers)
                         |
        SPDT relay       |        NO ----------> Heater Pot A
      +-----------+      |       /
      |    coil   |      +------o  (armature)
Pin 4 o--[driver]-+      |       \
      |           |      |        NC ----------> Heater Pot B
      +-----------+      |
            |            +--- flyback diode across coil
           GND

Pin 5 o---[220R]---|>|---GND      status LED B ("pot B")
Pin 6 o---[220R]---|>|---GND      status LED A ("pot A")
Pin 7 o---[button or jumper]---GND  optional: restart at pot A (hold while switching on)
```

- The relay coil is switched via a transistor (e.g. NPN/logic-level MOSFET) with a flyback
  diode, since an MCU pin cannot drive a relay coil on its own. The driver stage is not
  part of the firmware.
- The LEDs need series resistors (≈ 220 Ω at 5 V).
- Pin 4 HIGH selects pot A (`POT_A = 1`). With a non-inverting driver the coil is then
  energized, so pot A goes on the NO contact and pot B on NC. With an inverting driver,
  swap them. The firmware only toggles pin 4 and mirrors its state on pins 5/6.
