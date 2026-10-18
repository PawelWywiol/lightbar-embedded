# Hardware selection

All numbers come from [facts.md](facts.md). **CALC** results use the formula shown; they are estimates for sizing, not measurements.

## Sizing formulas

- LED current at full white: `N × I_led`, with I_led between 50 mA (Pololu measured) and 60 mA (Adafruit max); WLED uses 55 mA.
- USB LEDs: `N = (I_port − I_board) / I_led`.
- Data line time: `N × 30 µs + 0.28 ms` must fit 16.7 ms (60 FPS) → ≤ 546 LEDs per line.
- Feed points: ≤ 4 A per strip end, ≤ 8 A per middle feed (QuinLED); ≤ 1 m from any LED to a feed (Adafruit, stricter).
- Voltage drop of a feed wire with load spread evenly: `ΔV ≈ R_per_m × L × I`.

## Class A: USB powered

### Controller

ESP32-C3 board, so the firmware runs without porting. Candidates (≥ 4 MB flash, all ESP32-C3):

| Board | For | Against |
|---|---|---|
| Seeed XIAO ESP32C3 | official docs, external U.FL antenna, 5V pin = VBUS | 3V3 current conflict in docs (700 / 500 mA), CC resistors **UNVERIFIED** |
| ESP32-C3 Super Mini | small, cheap, already owned (see inventory) | no official docs, Wi-Fi problems reported, LDO and CC resistors **UNVERIFIED** |
| Waveshare ESP32-C3-Zero | official wiki | onboard WS2812 on GPIO10 = current `APP_LIGHTS_GPIO` default, change the GPIO |
| Espressif ESP32-C3-DevKitM-1 | official, matches PlatformIO board | larger, onboard RGB LED on GPIO8 |

### LEDs

WS2812B-V5 at 5 V straight from VBUS: VIH 2.7 V, so 3.3 V GPIO data is in spec without a level shifter. Other 5 V chips (WS2812B old, SK6812, WS2811, WS2813) are out of spec at 3.3 V data.

### LED count (CALC, full white, 55 mA/LED)

| USB source | Port | Board at RX 84 mA | Board at TX peak 335 mA |
|---|---|---|---|
| USB 2.0 | 500 mA | 7 | 3 |
| USB 3.x | 900 mA | 14 | 10 |
| WLED limit through a board's USB | 1 A | 16 | 12 |
| BC 1.2 charger | 1.5 A | 25 | 21 |

- Board current is the ESP32-C3 chip figure at 3.3 V (LDO passes the same current); LDO and USB-serial overhead not included.
- More LEDs work only below full white, which needs a software current limiter (not in firmware yet).
- USB-C chargers (C-to-C cable) give 5 V only if the board has 5.1 kΩ on CC1/CC2; a USB-A to USB-C cable avoids this.
- Powerbanks may shut off at low current (dark schemes).

## Class B: external power

### LED density for a 5 m strip

| LEDs/m | LEDs | 5 m reel (BTF) | Firmware today | 1 data line at 60 FPS | Current (BTF rating → 60 mA) |
|---|---|---|---|---|---|
| 30 | 150 | yes | yes | yes | 6 → 9 A |
| 60 | 300 | yes | no (> 255) | yes | 11 → 18 A |
| 96 | 480 | no (1/2 m pieces) | no | yes, tight | 17 → 28.8 A |
| 144 | 720 | no (1/2 m pieces) | no | no, 2 lines | 25 → 43.2 A |

Current column (CALC): BTF W/m × 5 m / 5 V → N × 60 mA.

### Controller

| Option | For | Against |
|---|---|---|
| ESP32-C3 (as today) | no porting | 2 RMT TX channels, no RMT DMA: Espressif warns Wi-Fi interrupts corrupt long RMT transfers; long-strip behavior untested |
| ESP32-S3 (e.g. Waveshare ESP32-S3-RS485-WLED) | RMT DMA, 4 TX channels, 2 cores; board has level shifting and a fuse | needs IDF target port; board max 8 A, so the strip must be fed from the PSU directly |
| ESP32 (Gledopto GL-C-016WL-D, already owned; QuinLED Dig-Uno) | 8 TX channels, 2 cores, ready enclosures; QuinLED has AHCT level shifter and fuse | needs IDF target port; no RMT DMA; Gledopto level shifter and fuse **UNVERIFIED** |

Port to another chip = new PlatformIO env + IDF target, then build and test on hardware (effort **UNVERIFIED**).

### Power

- 5 V, 150 LEDs: 6–9 A → LRS-50-5 (10 A). Feed both ends (Pololu measured 2.0 V drop at 4.1 A when 150 LEDs were fed from one end).
- 5 V, 300 LEDs: 11–18 A → LRS-100-5 (18 A) leaves no reserve at 60 mA; use a software limit. Feed points (CALC): 2 ends (2 × 4 A) + 1 middle (8 A) = 16 A, 2 middles for 18 A; Adafruit's 1 m rule needs a feed every 2 m.
- 12 V WS2815: datasheet says lower current and voltage drop; strip W/m not published by BTF (**UNVERIFIED**); 12 V PSU e.g. LRS-100-12 (8.5 A), LRS-150-12 (12.5 A).
- Fuse every feed just above its max current, close to the PSU; injection wire not thinner than the fuse wire.
- Strip and controller in parallel from the PSU; controller output limit (Gledopto 15 A total) does not limit injected current.
- One PSU per strip, or several with grounds joined and + rails separate.
- 1000 µF across + / − at the strip, 300–500 Ω in the data line at the strip end, level shifter for 5 V chips.

## Inventory (from earlier notes)

Identified from photos in an earlier session, not re-checked here.

| Board | Chip | Usable here |
|---|---|---|
| ESP32-C3 Super Mini | ESP32-C3 FN4 | class A |
| Gledopto GL-C-016WL-D | ESP32 (variant from seller text, not read from chip) | class B after port |
| Wemos/Lolin S2 Mini | ESP32-S2FN4R2 | only after port; single core, no RMT DMA |
| Wemos D1 mini | ESP8266EX | no (not ESP-IDF 5.x) |
| ProMicro RP2040, Pro Mini, Digispark, LilyTiny | RP2040, ATmega328P, ATtiny85 | no (no Wi-Fi) |

## Before buying

1. Decide class B LED density and voltage (see [requirements.md](requirements.md) open questions).
2. Flash current firmware on the chosen board, run the target LED count with Wi-Fi traffic, watch for wrong colors.
3. Measure real current at full white and at a typical scheme; size PSU / USB source from the measurement.
4. Read the ESP32 variant from esptool on first flash of the Gledopto (`Chip is ...`).
