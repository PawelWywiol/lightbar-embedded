# Extra components

What to add besides board, LEDs and power source. Sources in [facts.md](facts.md).

| Part | Small USB gadget | Strip 3–5 m, own PSU | Why |
|---|---|---|---|
| Level shifter: SN74AHCT1G125 / SN74LV1T34 (1 channel), SN74AHCT125 / 74HCT245 (more) | yes | yes (unless the controller has one) | ESP32 VOH ≥ 2.64 V is below every WS2812-type VIH (2.7–3.5 V); AHCT/HCT/LV1T have VIH 2 V at 5 V |
| 100 nF next to the level shifter | yes | yes | TI recommends 0.1 µF bypass |
| Series resistor in data line | yes | yes | ringing / reflections; value and place disputed: Adafruit 300–500 Ω at the strip, forums 33–50 Ω at the driver |
| Bulk capacitor across strip + / − | ≤ 10 µF on USB VBUS | 500–1000 µF, ≥ 6.3 V, at strip input | buffers current steps (Adafruit); USB 2.0 allows max 10 µF without inrush limiting |
| Fuse (ATO blade) near PSU, + lead, just above max current | no (USB port is limited) | yes, required above 100 W (WLED) | wire / strip fire protection |
| Power injection wires ≥ 22 AWG, ferrules in screw terminals | no | yes | voltage drop; splayed strands loosen and spark (WLED) |
| RS485 transceivers | no | only if data wire > ~5 m | single-ended data degrades over long runs (WLED) |
| IR receiver Vishay TSOP38438 (or TSOP4438), optional 100 Ω + 4.7 µF supply filter | offline devices | offline devices | 2.0–5.5 V, output straight to GPIO; TSOP38238 / 4838 are legacy, not for NEC |
| Buttons on non-strapping GPIOs with internal pull-up | offline devices | offline devices | strapping pins change boot mode; `espressif/button` handles debounce and long press |
| USB-C with 5.1 kΩ Rd on CC1 / CC2 | check board | — | C-to-C chargers give no 5 V without it |

Notes:

- Do not use TXS0102 / TXS0108 (only for < 50 cm) or I2C level shifters (too slow) for LED data (WLED).
- "Sacrificial pixel" (first LED powered through a diode, acting as level shifter) works but forums recommend a real level shifter.
- Never join + outputs of two PSUs; join grounds (WLED).
- Power the strip directly from the PSU, controller in parallel; not through the board's USB / 5 V pin.
