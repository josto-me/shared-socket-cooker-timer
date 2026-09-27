# Timer math and phase timing

The firmware runs its own `main()` and uses **Timer3** (16-bit) as the only time base.
There are no `delay()` calls; a single overflow ISR decrements a software timer
(`timer_sec`) and the state machine polls it.

## From prescaler to tick

`TCCR3B = 0b00000100` selects clock source `CS3 = 0b100`, i.e. **prescaler 256**.

| Quantity | Formula | Value |
|---|---|---|
| CPU clock | given | 16 000 000 Hz |
| Timer prescaler | `CS3 = 0b100` | 256 |
| Timer clock | 16 MHz / 256 | 62 500 Hz |
| Counter width | 16-bit | 65 536 counts / overflow |
| **Overflow period (1 tick)** | 65 536 / 62 500 | **1.048576 s** |

So one ISR call ≈ **1.049 s**. `timer_sec` therefore counts in units of ~1.049 s,
not exact seconds.

## The 5 / 15-minute constants

Ticks needed for a target time `t`:  `ticks = round(t / 1.048576 s)`.

| Constant | Target | Exact ticks | Rounded | Real time back-calculated |
|---|---|---|---|---|
| `TIME_5min`  | 300 s (5 min)  | 286.11 | **286** | 286 × 1.048576 = 299.89 s (≈ 5:00) |
| `TIME_15min` | 900 s (15 min) | 858.34 | **858** | 858 × 1.048576 = 899.68 s (≈ 15:00) |

In the code:

```c
#define TIME_1min 57     //(60s / 1,048576s) = 57
#define TIME_5min 286    //(300s / 1,048576s) = 286
#define TIME_15min 858   //(900s / 1,048576s) = 858
```

The constants apply to a 16 MHz clock. At another clock the tick changes and the
constants have to be recalculated.

## Phase sequence

With `HEATUP_ENABLED = 1` the timer runs:

```
Phase 1: Pot A,  15 min   (heat up A)
Phase 2: Pot B,  15 min   (heat up B)
Phase 3: Pot A,  5 min    (keep warm)
Phase 4: Pot B,  5 min
Phase 5: Pot A,  5 min
...alternating every 5 min forever
```

With `HEATUP_ENABLED = 0` (default) it alternates every 5 min from phase 1.

### ASCII timing diagram (heat-up sequence)

Signal levels as set by the code (phase 1 starts with the relay on pot A):

```
time ->     |<-- 15 min -->|<-- 15 min -->|<- 5 ->|<- 5 ->|<- 5 ->|<- 5 ->|
Relay(4)    |    Pot A     |    Pot B     |  A    |  B    |  A    |  B    |
Pin 6 LED A ############### _______________ ####### _______ ####### _______
Pin 5 LED B _______________ ############### _______ ####### _______ #######
                                          (keep both warm, 5-min alternation)
```

`#######` = pin high (LED on), `_______` = pin low (LED off). LED A (pin 6) is lit while
the relay selects pot A, LED B (pin 5) while it selects pot B. Which relay contact
carries which pot depends on the relay driver (see `wiring.md`).

## EEPROM save interval

`TIME_SAVE` = `TIME_1min` = 57 ticks = 59.77 s. The state is saved on every phase
start and then every 57 ticks: about 60 + 12 = 72 writes per hour with 5-min phases.
Endurance and the ring buffer are described in the README.
