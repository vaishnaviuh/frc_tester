# FRC 10-Pin General-Purpose Pinout Tester

ESP32-based rig for checking 10-conductor FRC harnesses/cables/adapters (2x5,
0.1" pitch IDC-style connectors, as used for RoboRIO DIO/PWM/relay headers
etc.) against an arbitrary expected pinout — not just straight-through
cables, but crossed, remapped, split (fan-out), or intentionally-open pins.

## How it works

- Two female 10-pin headers (**Header A** = drive side, **Header B** = sense
  side) are mounted on the protoboard and wired to the ESP32.
- The cable/harness/adapter under test is plugged in between Header A and
  Header B.
- Firmware drives one Header-A pin HIGH at a time (all other drive pins set
  to high-impedance input, so a shorted cable never causes contention), and
  reads all 10 Header-B pins (each pulled to GND).
- Each drive pin has a configurable **expected mapping**: which Header-B
  pin(s) should read HIGH when it's driven (a bitmask, so a single drive pin
  can legitimately expect to fan out to multiple sense pins). Default is
  identity (pin *i* -> pin *i*), i.e. a straight-through cable.
  - Actual result matches expected -> PASS.
  - Expected pin(s) missing from actual -> reported as **missing** (open).
  - Actual pin(s) not in expected -> reported as **unexpected** (short or
    miswire).
- Results print over USB serial (115200 baud) as a pass/fail table, repeating
  every 2 seconds so you can wiggle the cable and watch for intermittent
  faults.

### Configuring the expected mapping (no reflash needed)

Send commands over the serial monitor at 115200 baud:

| Command | Effect |
|---|---|
| `MAP` | Print the current expected mapping |
| `IDENTITY` | Reset to straight-through (pin *i* -> pin *i*) |
| `SET <pin> <mask>` | Set expected sense pin(s) for a drive pin |
| `RUN` | Run one test pass immediately |

`<pin>` is 1-10 (the drive-side pin number). `<mask>` is a comma-separated
list of expected sense pin numbers (1-10), or `0` for "expected open" (pin
intentionally not connected through, e.g. a key pin).

Examples:
```
SET 3 7        # pin 3 is expected to be crossed to pin 7
SET 1 1,2      # pin 1 is expected to fan out (split) to pins 1 and 2
SET 9 0        # pin 9 is expected to be unconnected
IDENTITY       # back to straight-through for all 10 pins
```

The mapping lives in RAM only (not persisted across power cycles) — script
it via serial at startup, or hardcode a different default in
`setIdentityMap()`/`setup()` in the firmware if you always test the same
non-standard harness.

## BOM

- 1x ESP32 dev board (WROOM-32 or similar, 30/38-pin)
- 2x female 2x5 (10-pin) 0.1" IDC box headers (or the specific FRC connector
  you're testing — shrouded box header recommended so the cable is keyed)
- 10x 10k resistors (pulldowns for Header B sense lines)
- General-purpose protoboard, headers/sockets for the ESP32, hookup wire

## Pinout

| Cable pin | Header A (drive) — ESP32 GPIO | Header B (sense) — ESP32 GPIO | Notes |
|-----------|-------------------------------|-------------------------------|-------|
| 1         | 4                              | 23                             | |
| 2         | 5                              | 25                             | |
| 3         | 13                             | 26                             | |
| 4         | 14                             | 27                             | |
| 5         | 16                             | 32                             | |
| 6         | 17                             | 33                             | |
| 7         | 18                             | 34                             | input-only, needs external 10k pulldown |
| 8         | 19                             | 35                             | input-only, needs external 10k pulldown |
| 9         | 21                             | 36                             | input-only, needs external 10k pulldown |
| 10        | 22                             | 39                             | input-only, needs external 10k pulldown |

Pins chosen to avoid strapping pins (0, 2, 12, 15), flash SPI pins (6-11),
and UART0 (1, 3). GPIOs 34/35/36/39 are input-only on the ESP32 and have no
internal pull resistors, so all 10 sense lines use an external 10k pulldown
to GND for consistency (even though 23/25/26/27/32/33 could use the internal
one).

Wire Header A pin *N* directly to its listed GPIO. Wire Header B pin *N* to
its listed GPIO **and** to GND through a 10k resistor at that pin.

If your FRC connector pinout reserves any of the 10 pins for power/ground
(check the actual connector you're testing — some 10-pin FRC headers carry
+5V/+12V and GND on specific pins rather than 10 signal lines), do **not**
wire the ESP32 GPIO directly to those pins. Leave power/ground pins
unconnected on the tester or add series resistors + a simple voltage divider
so you're only sensing presence, not backfeeding the ESP32.

## Firmware

See `firmware/frc_continuity_tester/frc_continuity_tester.ino`. Flash with
Arduino IDE or `arduino-cli` targeting an ESP32 board, open serial monitor
at 115200 baud.
