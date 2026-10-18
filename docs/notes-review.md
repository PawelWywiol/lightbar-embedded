# Earlier notes review

Two earlier notes ("Projekt LED na ESP32 – podsumowanie", "Porównanie płytek mikrokontrolerowych", 2026-10-18) checked against sources in [facts.md](facts.md).

Status: **OK** confirmed, **FIX** wrong or misquoted, **?** not verifiable, **N/A** does not apply to this project.

## Not applicable to this project

| Claim | Status | Why |
|---|---|---|
| Own firmware with PlatformIO + Arduino + FastLED, `board = esp32dev` | N/A | this firmware is ESP-IDF (C, RMT), target `esp32c3`; FastLED is not used |
| FastLED `APA106` class, RGB default, `setMaxPowerInVoltsAndMilliamps` | OK, N/A | exists in FastLED source; irrelevant for ESP-IDF |
| I2S mic API differences Arduino-ESP32 2.x / 3.x | N/A | no Arduino here |
| Rust crate `gledopto` | N/A | not checked |

## LEDs and current

| Claim | Status | Note |
|---|---|---|
| WS2812B 60 mA at full white (datasheet) | FIX | old WS2812B datasheet states no current; 60 mA is Adafruit's figure |
| Measured ~50 mA/LED (Pololu 30 LEDs 1.5 A) | OK | Pololu product page |
| 10 LEDs 0.481 A | OK | Fab Academy; MCU in reading unknown |
| Single color / rainbow 20–24 mA/LED | ? | Adafruit gives 20 mA as rule of thumb only |
| 9 / 18 / 43.2 W/m for 30 / 60 / 144 LEDs/m | FIX | that is 60 mA × 5 V × LEDs/m; BTF rates 6 / 11 / 25 W/m |
| Pololu 0.8 V drop on 2 m of 60 LEDs/m | FIX | 0.8 V was 60 LEDs on 2 m (30/m); 120 LEDs on 2 m dropped 1.4 V |
| 144 LEDs/m strip 2 m, PSU 5 V 20 A | OK (CALC) | 288 × 60 mA = 17.3 A; BTF sells 2 m at 144/m, no 5 m reel |
| APA106: 4.5–6.5 V, 14 mA/ch (~42 mA), RGB order | OK | datasheet |
| APA106 timing differs from WS2812 | OK | T1H 1.36 µs vs WS2812B 0.8 µs |
| APA106 IP33, seller spacing 3 / 5 cm, 0.24 W | ? | seller data not found |

## WS2811 5 m strip (seller listing)

| Claim | Status | Note |
|---|---|---|
| 5 V, GRB, SMD3240, IP67, 0.1 W/LED, 5.5 mA/ch, 26 AWG, extra power every 300 LEDs | ? SELLER | listing not found |
| 1.5 cm spacing → 330 LEDs on 5 m | OK (CALC) | 500 / 1.5 = 333 |
| 330 × 0.1 W = 33 W = 6.6 A | OK (CALC) | from seller data |
| Power every 300 LEDs is enough | FIX | 6.6 A > 4 A max per strip end (QuinLED); feed both ends or more |
| WS2811 IC color order | note | WS2811 IC datasheet: RGB; strip wiring may differ, test |

## USB and powerbank

| Claim | Status | Note |
|---|---|---|
| USB 2.0 500 mA, USB 3.0 900 mA, BC 1.2 1.5 A | OK | TI USB Type-C primer |
| Some PCs limit USB 3.0 to 500 mA for USB 2.0 devices | ? | forum source only |
| TV ports printed per port (Sony 500 / 900 mA) | ? | no manufacturer manual found |
| 8–10 LEDs white / 20–25 animation from 500 mA | FIX | counts LEDs only (note mentions board current but does not subtract it); with ESP32-C3 (84–335 mA) it is 3–7 LEDs at full white (CALC) |
| 10 000 mAh = 37 Wh, ~6000–6700 mAh at 5 V | OK | Anker: 85–90 % efficiency → 6290–6660 mAh (CALC) |
| Quick Charge gives nothing to a 5 V load | OK | QC needs both sides |
| Powerbank ~3 A output | ? | model dependent |
| Runtime table (4.8 h, 2.5 h, 6 h) | OK (CALC) | 6300 mAh / current, controller not included |

## Wiring

| Claim | Status | Note |
|---|---|---|
| One PSU for strip and ESP32, joined grounds with two PSUs | OK | WLED: grounds together, never + rails |
| Do not power strip through board 5V pin | OK | WLED: strip directly from PSU, max 1 A through board USB |
| Do not connect USB and external 5 V at the same time | OK | Espressif DevKitC: one source only; XIAO allows external 5 V through a diode |
| 1000 µF capacitor, fuse on +, level shifter | OK | Adafruit (500–1000 µF, 74AHCT125), WLED (fuse just above max current) |
| Voltage drop table (26–18 AWG, 3/5 m, 4.2/6 A) | OK (CALC) | matches ΔV = R·L·I (evenly spread load, both wires); copper only |
| Feeding both ends cuts drop ~4× | ? | not checked |
| ESP32 ~240 mA Wi-Fi TX | OK | ESP32 datasheet, 802.11b 19.5 dBm, 50 % duty |
| ESP32 peaks 300 mA measured | ? | SparkFun page not checked |
| PSU 5 V 10 A for 330 LEDs + ESP32 | OK (CALC) | 6.6 + 0.24 = 6.84 A; Mean Well LRS-50-5 is 10 A |
| ~20–30 % PSU reserve | ? | only a vendor blog (20 %); Adafruit gives no number |

## Gledopto

| Claim | Status | Note |
|---|---|---|
| GL-C-016WL-D: 5–24 V, 10 A/ch, 15 A total | OK | manual |
| Data GPIO16 default, GPIO2 standby, relay GPIO12, mic SD 26 / WS 5 / SCK 21, spare 18/19/25/33 | OK | manual |
| Relay does not cut extra injected power | OK | manual |
| UART upload over USB-C | OK | manual |
| USB-UART chip CH341 | ? | not in manual |
| Output voltage = input voltage | ? | implied by 5/12/24 V support, not stated |
| Level shifter, fuse | ? | not in manual |
| Max 800 ICs | OK | gledopto.eu shop |
| −20–45 °C | OK | manual |
| 108 × 45 × 18 mm | OK | manual |
| GL-C-015WL-D discontinued | ? | no source |
| GL-C-017WL-D "5–14 V" is a typo, real 5–24 V | OK | official manual: DC 5-12-24V |
| GL-C-017WL-D GPIOs unknown | FIX | manual: data GPIO16/12/4/2, relay GPIO18 inverted, PDM mic SD 32 / WS 15 |
| DC barrel 5.5 × 2.1 mm only ~3 A | ? | not checked |
| Weight 0.069 vs 0.57 kg | ? | not checked, irrelevant |
| Classic ESP32 variant | ? | read with esptool on first flash |

## Boards

| Claim | Status | Note |
|---|---|---|
| Board identification from photos | ? | photos not available here |
| ESP32-C3: 400 KB SRAM, RISC-V 160 MHz, BLE 5, 4 MB in FN4 | OK | Espressif datasheet v2.4 |
| ESP32-C3 Super Mini 22.5 × 18 mm, 13 GPIO | OK | sigmdel review |
| ESP32-S2: single LX7, 320 KB, Wi-Fi only | OK | datasheet |
| ESP32: 520 KB, Wi-Fi + BT classic/BLE | OK | datasheet |
| RP2040: 2× M0+ 133 MHz, 264 KB, no Wi-Fi | OK | datasheet |
| ATtiny85 512 B SRAM, ATmega328P 2 KB, no Wi-Fi | OK | datasheets |
| D1 mini (ESP8266) in the board set | N/A | ESP8266 is not supported by ESP-IDF, cannot run this firmware |
| Other board details (regulators, dimensions from photos, CH340C) | ? | not re-checked |
