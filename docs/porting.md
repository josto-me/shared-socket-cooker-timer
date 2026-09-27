# Porting notes

## Target boards

The sketch uses **Timer3**, which exists on the ATmega32U4 (Arduino Leonardo / Micro) and
the ATmega2560 (Arduino Mega 2560). Both run at 16 MHz, so the tick and the phase constants
are identical on either. Pins are driven with `pinMode`/`digitalWrite`, so the same source
compiles for both even though digital pins 4/5/6 map to different physical ports.

## Porting to an Arduino Uno (ATmega328P) — use Timer1

The Uno has **no Timer3**. The 16-bit timer on the ATmega328P is **Timer1**. Move the
timer setup and the ISR over:

| Leonardo/Mega (Timer3) | Uno (Timer1) |
|---|---|
| `TCCR3A = 0b00000000;` | `TCCR1A = 0b00000000;` |
| `TCCR3B = 0b00000100;` (prescaler 256) | `TCCR1B = 0b00000100;` (prescaler 256) |
| `TCCR3C = 0b00000000;` | *(no TCCR1C write needed)* |
| `TIMSK3 = 0b00000001;` (TOIE3) | `TIMSK1 = 0b00000001;` (TOIE1) |
| `ISR(TIMER3_OVF_vect)` | `ISR(TIMER1_OVF_vect)` |

The clock is the same (16 MHz), Timer1 is also 16-bit, so the prescaler/256 tick stays at
1.048576 s and `TIME_5min`/`TIME_15min` do not change. Pins 4/5/6 exist on the Uno as well.

If a board runs at 8 MHz, the tick doubles to ~2.097 s; then `TIME_1min`, `TIME_5min` and
`TIME_15min` have to be halved (29, 143, 429).

## Caveat: overriding `main()` on a Leonardo / Micro

This firmware defines its own `int main()` and never calls the Arduino core `init()` or
`USBDevice` setup. On an **ATmega32U4** board (Leonardo/Micro) this **disables USB-CDC**:
the board will not enumerate as a serial port, so the auto-reset-into-bootloader trick that
the IDE uses for uploading does not work. To flash a new sketch you have to **enter the
bootloader manually** (double-tap the reset button, then upload within the bootloader
window). On the Mega 2560 this does not apply — its USB is a separate ATmega16U2 and the
main MCU's reset/upload path is unaffected.

If USB serial is wanted on the 32U4 while keeping this structure, call the core init
explicitly (`init(); USBDevice.attach();`) at the top of `main()` before configuring
Timer3 — at the cost of pulling in the Arduino USB stack.
