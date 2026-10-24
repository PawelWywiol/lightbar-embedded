# Hardware selection

Recommendations are conclusions drawn from [facts.md](facts.md) and [opinions.md](opinions.md); each states why. Extra parts per build: [components.md](components.md).

## Summary

| Use | Recommended | Acceptable | Not recommended |
|---|---|---|---|
| Small gadget, online | Seeed XIAO ESP32C3 | Seeed XIAO ESP32S3, Espressif ESP32-C3-DevKitM-1 | ESP32-C3 / S3 Super Mini, Waveshare C3 / S3-Zero, Lolin S2 Mini |
| Small gadget, offline | ESP32-C3 board (XIAO, DevKitM-1) | ESP32-C3 Super Mini (already owned) | ESP32-C2 boards (no RMT) |
| Strip 3–5 m, online | ESP32-S3 board + level shifter | Gledopto GL-C-016WL-D (owned), QuinLED Dig-Uno / Dig-Next-2 (ESP32) | ESP32-C3 boards / controllers, Athom controllers |
| Strip 3–5 m, offline | ESP32-S3 or ESP32 (Gledopto) | ESP32-C3 (not measured) | — |

## Why

### Chips

- **ESP32-C3** (current target): single core, 2 RMT TX, no RMT DMA. Espressif: without DMA a delayed RMT interrupt (e.g. Wi-Fi on the same core) corrupts the LED data. WLED maintainers confirm C3 flicker under Wi-Fi / web load as a known C3-only issue. Fine where the transfer is short or Wi-Fi is off; weakest choice for long strips online.
- **ESP32-S3**: dual core, 4 RMT TX, the only checked chip with RMT DMA. Best technical fit for long strips with Wi-Fi. Build env exists (`esp32-s3-devkitc-1`), not tested on hardware.
- **ESP32** (Gledopto, QuinLED): dual core, 8 RMT channels, no RMT DMA. WLED developers report occasional RMT flicker on ESP32 under load too, less than on C3. Build env exists (`gledopto-gl-c-016wl-d`), not tested on hardware.
- **ESP32-C6 / H2**: 2 RMT TX, no DMA, single core; H2 has no Wi-Fi; no PlatformIO board file for H2. No advantage here.
- **ESP32-C2**: no RMT (LEDs only via SPI backend), no PlatformIO board file. Not for this firmware.
- **ESP32-S2**: single core, no RMT DMA, clone S2 Mini boards with Wi-Fi faults. No advantage over C3.
- ESP8266, RP2040, ATtiny85, ATmega328P: not ESP-IDF 5.x or no Wi-Fi.

### Boards

- **XIAO ESP32C3**: official docs, external U.FL antenna; users call it "excellent quality and well documented". One report of WS2812 corruption with Wi-Fi AP on (chip issue above).
- **ESP32-C3 Super Mini**: strongest negative pattern found: many independent Wi-Fi failures (one test 4 of 10 boards, AUTH_EXPIRED at 20 dBm, fixed by 8.5–11 dBm TX power or antenna mod), crystal too close to antenna on clone revisions, undersized LDO, GPIO2 boot problems with LED data. Wi-Fi faults do not matter offline, so usable there.
- **S3 Super Mini, Waveshare C3 / S3-Zero**: same ceramic-antenna clearance complaints (Elektor measured +10 dB after mod; soldering Zero flat kills Wi-Fi).
- **XIAO ESP32S3**: FPC antenna; several "works outside enclosure, fails inside" reports; counter-reports that it works in plastic enclosures.
- **Espressif ESP32-S3-DevKitC-1 / ESP32-C3-DevKitM-1**: official, larger; one S3 DevKitC Wi-Fi report (fixed with 8.5 dBm, board origin unverified).

### Controllers

- **QuinLED Dig-Uno / Dig-Quad / Dig-Next-2**: AHCT level shifters, fuses per output, strongest user loyalty. Known 24 V DC-DC failure on v3.5r3 Uno/Quad (maker: "10 to 15 cases"), not relevant at 5 V per maker's description (only at 24 V). No independent Dig-Next-2 review.
- **Gledopto GL-C-016WL-D**: 5–24 V, 2 outputs, relay, mic, enclosure; users praise relay and support. Data output called weak ("weak level shifter which limits cable distance", one 016-specific opinion); other Gledopto models had 1 kΩ data resistor / weak shifter (vendor: fixed in 2024). Keep the controller-to-strip data wire short.
- **Athom**: polarised; one user 22 controllers, 59 % success; others report 5–20 units fine; units arriving bricked.
- **Waveshare ESP32-S3-RS485-WLED**: S3, level shifting, fuse, 8 A max; no user opinions found.
- **DOMRAEM DOM-ETH-806DM ("WLED Pro MAX")**: ESP32 (classic; vendor firmware image header chip id 0, WLED 16.0.1 `ESP32_Ethernet` build, [jacker-25/DOM-ETH-806DM](https://github.com/jacker-25/DOM-ETH-806DM)). **SELLER**: 6 outputs on GPIO 18/16/14/13/4/2, Ethernet + Wi-Fi, PDM mic, relay, 5–24 V, 15 A, 20 A fuse, USB-C UART. **UNVERIFIED**: level shifter, relay / mic GPIOs, Ethernet PHY, no schematic. No user opinions found. Here: one output used (firmware drives one line), Ethernet unused (firmware is Wi-Fi only); new env needed once relay GPIO is known.

## Sizing formulas

- LED current at full white: `N × I_led`, I_led 50 mA (Pololu measured) to 60 mA (Adafruit max); WLED uses 55 mA.
- USB LEDs: `N = (I_port − I_board) / I_led`.
- Data line time with this firmware: `N × 36 µs + 0.05 ms` (full white) ≤ 16.7 ms → ≤ 461 LEDs per line at 60 FPS.
- Feed points: ≤ 4 A per strip end, ≤ 8 A per middle feed (QuinLED); ≤ 1 m from any LED to a feed (Adafruit, stricter).

## Class A: USB powered

LED count at full white, 55 mA/LED (**CALC**):

| USB source | Port | Board at RX 84 mA | Board at TX peak 335 mA |
|---|---|---|---|
| USB 2.0 | 500 mA | 7 | 3 |
| USB 3.x | 900 mA | 14 | 10 |
| WLED limit through a board's USB | 1 A | 16 | 12 |
| BC 1.2 charger | 1.5 A | 25 | 21 |

- Board current is the ESP32-C3 chip figure; offline (no radio) the chip draws 23–28 mA, leaving more for LEDs.
- More LEDs only below full white, with the software current limiter.
- USB-C chargers (C-to-C cable) need 5.1 kΩ on CC1/CC2 on the board; USB-A to USB-C cable avoids this.
- Powerbanks may shut off at low current (dark schemes).

## Class B: external 5 V power

| Strip | LEDs | 5 m reel (BTF) | 60 FPS on 1 line | Current: BTF rating → 60 mA (CALC) | PSU (Mean Well LRS) |
|---|---|---|---|---|---|
| 3 m × 30 | 90 | — | yes | 3.6 → 5.4 A | LRS-50-5 (10 A) |
| 3 m × 60 | 180 | — | yes | 6.6 → 10.8 A | LRS-100-5 (18 A) |
| 3 m × 144 | 432 | — | only just | 15 → 25.9 A | LRS-200-5 (40 A), or LRS-150F-5 (22 A) + limiter |
| 5 m × 30 | 150 | yes | yes | 6 → 9 A | LRS-50-5 (10 A) |
| 5 m × 60 | 300 | yes | yes | 11 → 18 A (QuinLED measured 13 A) | LRS-100-5 (18 A) + limiter |
| 5 m × 144 | 720 | no (1 m / 2 m pieces) | no, ≈ 38 FPS | 25 → 43.2 A | LRS-200-5 (40 A) + limiter |

- BTF max W/m figures are below measurements (QuinLED 65 W vs BTF 55 W for 300 LEDs); size from measured or 55–60 mA values.
- Feed points (CALC): 5 m × 60 at ~13–18 A needs both ends + 1–2 middle feeds; 5 m × 144 needs feeds about every 1 m.
- 144 LED/m over 5 m is above 100 W, where WLED says to add fuses.
- 720 LEDs at 60 FPS needs 2 data lines (2 RMT TX channels); firmware drives one line today.

## Test strip: WS2811 string (connected now)

Seller data (**SELLER**): DC 5 V, WS2811 protocol, color order GRB, SMD3240, 0.1 W per LED, 5.5 mA per R/G/B output, 26 AWG 3-wire, 1.5 cm spacing = 66 LED/m, IP67, "add voltage at every 300 LED".

| Item | Value | Note |
|---|---|---|
| LEDs on 5 m | 330 (**CALC**) | 66 × 5 |
| Current per LED, full white | **measured** ≈ 15.7 mA for the first 30 LEDs, falling to ≈ 7 mA per added LED at 70–86 LEDs | seller: 16.5 mA (3 × 5.5) or 20 mA (0.1 W / 5 V); cause of the fall **UNVERIFIED** (voltage drop along the string suspected) |
| 86 LEDs, full white, USB powered | **measured** 0.99 A at 4.85 V (board idle 0.02 A) | powerbank; linear 15.7 mA model gives 1.37 A |
| 330 LEDs, full white | ≤ 5.2 A (**CALC**, 15.7 mA) | LRS-50-5 (10 A) |
| Limiter | set `APP_LIGHTS_LED_CURRENT_MA` to 15 | default 55 mA is for WS2812B and would dim this strip ~3.5× too early; linear model overestimates long runs (safe side) |
| LEDs per feed, ≤ 0.5 V drop, one end | ≈ 110–120 (**CALC**) | `N = √(0.5 / (R·s·i))`, 26 AWG R = 0.1339 Ω/m, s = 0.015 m; real wire between LEDs is longer, so fewer |
| Seller's 300 LEDs per feed | ≈ 3 V drop (**CALC**) | `R·s·i·N²` at 16.5 mA; QuinLED: ≤ 4 A per strip end |
| Data level | level shifter | WS2811 IC VIH 0.7·VDD = 3.5 V |
| Timing | **UNVERIFIED**, test | WS2811 IC: 400 kHz mode T0H 0.5 / T1H 1.2 / T0L 2.0 / T1L 1.3 µs ±150 ns, 800 kHz mode "one half"; firmware sends WS2812 timing (T1H 0.9 µs, outside 800 kHz 0.6 ± 0.15 µs). Which IC sits in the SMD3240 package is unknown |
| Color order | test | seller GRB (firmware default); WS2811 IC datasheet sends RGB |

Measurement: USB meter between source and board, LEDs lit one by one at 255/255/255, Wi-Fi off, 1–50 LEDs from a MacBook (via USB hub), 0–86 LEDs from a 10000 mAh powerbank. Neither source cut out (MacBook 0.76 A, powerbank 0.99 A); the USB 2.0 500 mA figure is what a device may draw by spec, not where a port cuts off. Supply voltage at the meter fell 5.21 → 5.04 V (MacBook, 0.74 A) and 5.07 → 4.85 V (powerbank, 0.97 A).

At 400 kHz, transfer doubles: 60 µs per LED → 330 LEDs ≈ 20 ms ≈ 50 FPS (**CALC**). Espressif `led_strip` uses the 400 kHz timings for its `LED_MODEL_WS2811`.

## Inventory (from earlier notes)

Identified from photos in an earlier session, not re-checked here.

| Board | Chip | Usable here |
|---|---|---|
| ESP32-C3 Super Mini | ESP32-C3 FN4 | class A offline; online only if its Wi-Fi works (test, lower TX power) |
| Gledopto GL-C-016WL-D | ESP32 | class B (env `gledopto-gl-c-016wl-d`, untested) |
| Wemos/Lolin S2 Mini | ESP32-S2FN4R2 | not recommended |
| Wemos D1 mini, ProMicro RP2040, Pro Mini, Digispark, LilyTiny | ESP8266, RP2040, ATmega328P, ATtiny85 | no |

## Before buying

1. Decide 5 m strip density.
2. Flash the firmware on the chosen board, run the target LED count with Wi-Fi traffic (open the device page repeatedly), watch for wrong colors.
3. Measure current at full white and at a typical scheme; size PSU / USB source from the measurement.
4. On the Gledopto, read the ESP32 variant from esptool on first flash (`Chip is ...`).
