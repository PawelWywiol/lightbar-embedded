# Verified data

Markers: see [README.md](README.md). Source ids link to the list at the bottom.

## LED chips

| Chip | Supply | Current | Bit / reset | Data VIH | Order |
|---|---|---|---|---|---|
| WS2812B (old) | 3.5–5.3 V | not stated | 1.25 µs ±600 ns / > 50 µs | 0.7·VDD | GRB [L1] |
| WS2812B-V5 | 3.7–5.3 V | tested at 12 mA/ch, max not stated | T0H 220–380 ns, T1H 580 ns–1 µs / > 280 µs | 2.7 V min | GRB [L2] |
| WS2811 (IC) | specs at 4.5–5.5 V, 12/24 V strips feed IC via resistor | 18.5 mA/ch constant | 400 / 800 kbps / > 50 µs | 0.7·VDD | RGB [L3] |
| SK6812 RGB | 5.2 V typ | not stated (**UNVERIFIED**) | 1.25 µs / 80 µs | 3.4 V at VDD 5 V | GRB [L4] |
| SK6812 RGBW | 3.5–5.5 V | not stated | 1.25 µs / 80 µs | 3.4 V | RGBW, some strips GRBW [L5][A3] |
| WS2813 | 3.5–5.3 V | 15 mA (A/B), 5 mA (C/D) | 1.1 µs ±300 ns / ≥ 300 µs | 0.7·VDD | GRB, diagram contradicts [L6] |
| WS2815B | 9.5–13.5 V (12 V) | 10 mA/ch constant | / > 280 µs | 0.7·VDD vs VCC pin unclear (**UNVERIFIED**) | GRB [L7] |
| APA106-F5/F8 | 4.5–6.5 V | 14 mA/ch | T0H 0.35 µs, T1H 1.36 µs / 50 µs | not stated | RGB [L8] |

- WS2813 / WS2815: backup data input, "if DIN do NOT receive the signal, then switching to BIN" [L6][L7].
- WS2815: "12V DC power supply, can effectively reduce the operating current of the pixel LED, and decrease the voltage-drop" [L7].
- 3.3 V data at 5 V supply: 0.7 × 5.0 = 3.5 V > 3.3 V, out of spec for WS2812B (old), WS2811, WS2813; SK6812 needs 3.4 V; WS2812B-V5 needs 2.7 V (**CALC**).

### Data line timing at 60 FPS (CALC)

Frame 16.67 ms; RGB LED = 24 bits × 1.25 µs = 30 µs; reset 280 µs (WS2812B-V5). Max ≈ (16 667 − 280) / 30 = **546 LEDs per data line**.

| LEDs | Transfer | 60 FPS |
|---|---|---|
| 150 | 4.8 ms | yes |
| 255 | 7.9 ms | yes |
| 300 | 9.3 ms | yes |
| 480 | 14.7 ms | yes, little time left for computing |
| 720 | 21.9 ms | no, max ≈ 46 FPS |

RGBW (32 bits) ≈ 40 µs per LED → ≈ 410 per line.

## LED current

- Adafruit: "up to 60 milliamps at maximum brightness white", "20 mA per pixel" as rule of thumb for mixed content [A1].
- Pololu: "Each RGB LED draws approximately 50 mA at 5 V with red, green, and blue at full brightness" [A4]. Measured full white, fed at one end:

| Strip | Current | Voltage drop |
|---|---|---|
| 30 LEDs, 1 m | 1.5 A | 0.2 V |
| 60 LEDs, 2 m | 2.9 A | 0.8 V |
| 150 LEDs, 5 m | 4.1 A | 2.0 V |
| 60 LEDs, 1 m | 3.0 A | 0.6 V |
| 120 LEDs, 2 m | 4.7 A | 1.4 V |

- Pololu: "As the voltage drops, RGB LEDs tend to look redder and draw less current" [A4].
- Fab Academy: 10 × WS2812B full white 0.481 A at 5 V; whether the meter included the MCU is not stated [A5].
- WLED limiter default: "55mA (typ. 5V WS281x)" per LED at full white; "Keep below 1 A if powering LEDs from the ESP's 5V pin" [W1].

## 5 m strips (BTF-Lighting, WS2812B 5 V)

| LEDs/m | LEDs per 5 m (CALC) | BTF max W/m | 5 m reel sold |
|---|---|---|---|
| 30 | 150 | 6 | yes |
| 60 | 300 | 11 | yes |
| 96 | 480 | 17 | no (1 m, 2 m) |
| 100 | 500 | 18 | no (1 m, 2 m) |
| 144 | 720 | 25 | no (1 m, 2 m) |

Variants sold: 1M/2M at 30/60/96/100/144, 4M at 60, 5M at 30/60 [B1]. WS2812B-ECO: same 5 m options [B2]. WS2815 12 V: 5 m at 30/60, W/m not stated (**UNVERIFIED**) [B3].

Per 5 m at full white (**CALC**):

| LEDs/m | BTF rating | 50 mA (Pololu) | 55 mA (WLED) | 60 mA (Adafruit) |
|---|---|---|---|---|
| 30 | 30 W = 6 A | 7.5 A | 8.3 A | 9 A |
| 60 | 55 W = 11 A | 15 A | 16.5 A | 18 A |

## Power distribution

- Adafruit: "aim for 1 meter or less distance from any pixel to a power connection" [A1].
- QuinLED: "An edge injection can deliver a max of 4Amps into an LED strip and a middle injection about 8Amps"; injection wire ≤ 10 % voltage drop [Q4].
- WLED: fuse every strip "just above their max current", fuses close to the PSU; injection wire not thinner than the fuse wire [W2].
- WLED: "never mix two power supplies on the same LED strip. You can connect grounds together, just never the positive rails"; connect ESP and strip grounds [W2][W3].
- WLED: power strip directly from the PSU, board in parallel; "Never try to draw more than 1A through a board's USB port" [W2].
- Adafruit: 500–1000 µF ≥ 6.3 V capacitor across +/−; 300–500 Ω data resistor at the strip end; connect ground first; power pixels before the MCU [A2].
- Adafruit: "If using a 3.3V microcontroller you must use a logic level shifter such as a 74AHCT125 or 74HCT245" [A2].
- WLED: data over long distances (> 5 m) is hard to keep clean; RS485 for long runs [W4].
- Espressif DevKitC: power from "one and only one of the options", else damage [E6].

### Copper resistance

| AWG | mΩ/m |
|---|---|
| 18 | 20.95 |
| 20 | 33.31 |
| 22 | 52.96 |
| 26 | 133.9 |

[A6][A7]. Drop for a feed at one end with load spread evenly (**CALC**): ΔV ≈ R·L·I (both wires, average current I/2). Contact and strip PCB resistance not included.

## Power supplies (Mean Well LRS, datasheets)

| Model | Output | Rated current |
|---|---|---|
| LRS-50-5 | 5 V | 10 A |
| LRS-100-5 | 5 V | 18 A |
| LRS-150F-5 | 5 V | 22 A |
| LRS-200-5 | 5 V | 40 A |
| LRS-100-12 | 12 V | 8.5 A |
| LRS-150-12 | 12 V | 12.5 A |

No LRS-150-5 exists (LRS-150 is 12–48 V only). Output must be derated with temperature (datasheet derating curve) [M1]–[M5]. Headroom: Adafruit gives no percentage ("extra Amps are OK") [A1]; 20 % reserve only from a vendor blog [M6].

## USB power

| Source | 5 V current |
|---|---|
| USB 2.0 | 500 mA |
| USB 3.0 / 3.1 | 900 mA |
| USB BC 1.2 | 1.5 A |
| USB Type-C (no PD) | 3 A |
| USB PD 3.0 | up to 20 V / 5 A |

[U1]. Type-C current is advertised by the source's Rp; the sink needs Rd on CC1/CC2; when no device is attached VBUS should be 0 V [U1]. Rd = 5.1 kΩ (±10 % to detect 1.5 A / 3 A) [U2][U3].

- TV USB ports: rating printed per port, varies by model; no manufacturer manual with a value found (**UNVERIFIED**).
- Powerbank: 10 000 mAh cell at 3.7 V = 37 Wh; 37 Wh / 5 V = 7400 mAh; boost converters 85–90 % efficient [U4] → 6290–6660 mAh at 5 V (**CALC**).
- Powerbanks shut off at low current: "Many USB power banks shut off automatically when current draw is low" [A1]; Anker trickle mode for low-current devices [U5].
- Quick Charge only when both sides support it, otherwise regular 5 V [U6].

## Chips

| Chip | CPU | SRAM | Radio | RMT TX ch. | RMT words/ch | RMT DMA |
|---|---|---|---|---|---|---|
| ESP32-C3 | 1× RISC-V 160 MHz | 400 KB (16 KB cache) | 802.11b/g/n, BLE 5 | 2 | 48 | no |
| ESP32-S3 | 2× LX7 240 MHz | 512 KB | 802.11b/g/n, BLE (version **UNVERIFIED**) | 4 | 48 | yes |
| ESP32 | 2 (or 1)× LX6 | 520 KB | 802.11b/g/n, BT 4.2 BR/EDR + LE | 8 | 64 | no |
| ESP32-S2 | 1× LX7 | 320 KB | 802.11b/g/n, no Bluetooth | 4 | 64 | no |

[E1][E2][E3][E4][E5]. Espressif `led_strip`: without RMT DMA "if the RMT interrupt is delayed or not serviced in time (e.g. if Wi-Fi interrupt happens on the same CPU core), the RMT transaction will be corrupted"; SPI backend with DMA is the alternative but takes the whole SPI bus [E7].

- Wi-Fi TX current at 3.3 V: ESP32-C3 335 mA peak (802.11b 21 dBm, 100 % duty) [E1]; ESP32 240 mA (802.11b 19.5 dBm, 50 % duty) [E3]. RX: C3 84–87 mA, ESP32 95–100 mA.
- ESP32-C3 supply: 3.3 V, ≥ 500 mA [E8].
- ESP8266: not in ESP-IDF, uses ESP8266 RTOS SDK [E9]. RP2040, ATtiny85, ATmega328P: no Wi-Fi in datasheets.

## Boards (ESP32-C3, class A candidates)

| Board | Docs | 5V pin | 3V3 regulator | Onboard LED | Antenna |
|---|---|---|---|---|---|
| Seeed XIAO ESP32C3 | official wiki | VBUS; external 5 V only through a diode | wiki says 700 mA and 500 mA (**conflict**) | none | external U.FL |
| Waveshare ESP32-C3-Zero | official wiki | 3.7–6 V input | — | WS2812 on GPIO10 | ceramic |
| Espressif ESP32-C3-DevKitM-1 | official | — | — | addressable RGB on GPIO8 | module |
| ESP32-C3 Super Mini | third party only | VBUS wired directly to 5V pin (schematic, per review) | ME6211 500 mA per schematic; some boards 250 mA LDO (**UNVERIFIED**) | GPIO8 | ceramic |

[D1][D2][D3][D4][D5]. Super Mini: Wi-Fi problems reported on some boards; WLED: "Some stores sell a version that has crippling Wifi strength issues"; workaround in community: lower TX power to 8.5 dBm [D4][W5][D7]. Super Mini USB-serial: sources conflict (CH340 vs native USB) [D5][D6]. CC resistors on Super Mini: **UNVERIFIED**.

GPIO2, 8, 9 are strapping pins on ESP32-C3 [D3].

## Controllers (class B candidates)

| Controller | Chip | Input | Outputs | Level shifter | Fuse | Max current |
|---|---|---|---|---|---|---|
| Gledopto GL-C-016WL-D | ESP32 | 5/12/24 V | 2 (GPIO16, GPIO2) | **UNVERIFIED** | **UNVERIFIED** | 10 A/ch, 15 A total |
| Gledopto GL-C-017WL-D | ESP32 | 5/12/24 V | 4 (GPIO16, 12, 4, 2) | **UNVERIFIED** | **UNVERIFIED** | 15 A total |
| QuinLED Dig-Uno v3.1 | ESP32 | 5–24 V | 2 | AHCT | ATO fuse | — |
| QuinLED Dig-Quad v3.1 | ESP32 | 5–24 V | 4 (+1) | AHCT | 5 fuses | — |
| QuinLED Dig-Next-2 | ESP32, 8 MB flash, 2 MB PSRAM | 5–48 V | 2 | yes | 2× 5 A + 1× 10 A | 15 A suggested |
| cod.m v0.10 | ESP32 4 MB | 5/12/24 V | up to 4 | 74AHCT125D | **UNVERIFIED** | — |
| Waveshare ESP32-S3-RS485-WLED | ESP32-S3 N16R8 | 5–24 V | 2 (GPIO2, GPIO16) | 5 V level shifting | blade fuse | 8 A |
| Athom ESP32-C3 WLED controllers | ESP32-C3 | 5–24 V | 1 digital | **UNVERIFIED** | **UNVERIFIED** | — |

[G1]–[G4][Q1]–[Q3][C1][W6][T1]. GL-C-016WL-D details [G1]: relay GPIO12 cuts only the controller output ("cannot switch off the additional added power input"), I2S mic SD 26 / WS 5 / SCK 21, spare IO18/19/25/33, firmware upload by UART over USB-C, −20–45 °C, max 800 ICs [G2]. GL-C-017WL-D [G3]: relay GPIO18 inverted, PDM mic SD 32 / WS 15. Gledopto ESP32-C3 models: none found.

## Sources

- [L1] https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
- [L2] https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/8880/WS2812B.pdf
- [L3] https://cdn-shop.adafruit.com/datasheets/WS2811.pdf
- [L4] https://cdn-shop.adafruit.com/product-files/1138/SK6812+LED+datasheet+.pdf
- [L5] https://cdn-shop.adafruit.com/product-files/2757/p2757_SK6812RGBW_REV01.pdf
- [L6] https://shop-archive.boxtec.ch/pub/diverse/WS2813.pdf
- [L7] https://akizukidenshi.com/goodsaffix/ws2815b.pdf
- [L8] https://kyohritsu.com/eclib/PDF/L/LEDX226-APA-Z.pdf
- [A1] https://learn.adafruit.com/adafruit-neopixel-uberguide/powering-neopixels
- [A2] https://learn.adafruit.com/adafruit-neopixel-uberguide/best-practices
- [A3] https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use
- [A4] https://www.pololu.com/product/2546
- [A5] https://fabacademy.org/2025/labs/bottrop/Assignments/week10/
- [A6] https://en.wikipedia.org/wiki/American_wire_gauge
- [A7] https://www.powerstream.com/Wire_Size.htm
- [B1] https://www.btf-lighting.com/products/ws2812b-rgb-addressable-led-strip-dc5v
- [B2] https://www.btf-lighting.com/products/ws2812b-eco-led-pixel-strip
- [B3] https://www.btf-lighting.com/products/ws2815-rgb-addressable-led-strip-dc12v
- [W1] https://kno.wled.ge/features/settings/
- [W2] https://kno.wled.ge/advanced/wiring/
- [W3] https://kno.wled.ge/basics/getting-started/
- [W4] https://kno.wled.ge/advanced/longdata/
- [W5] https://mm.kno.wled.ge/basics/compatible-controllers/
- [W6] https://www.waveshare.com/esp32-s3-rs485-wled.htm
- [Q1] https://quinled.info/quinled-dig-uno-pre-assembled-v3-1-specifications/
- [Q2] https://quinled.info/quinled-dig-quad-pre-assembled-v3-1-specifications
- [Q3] https://quinled.info/dig-next-2-specifications/
- [Q4] https://quinled.info/the-ultimate-led-strip-power-injection-guide/
- [C1] https://docs.codm.de/en/wled/controller/
- [T1] https://www.athom.tech/blank-1/wled-2812b-led-strip-controller
- [G1] https://www.gledopto.eu/mediafiles/anleitungen/7002-gl-c-016wl-d-eng.pdf
- [G2] https://www.gledopto.eu/gledopto-esp32-wled-uart_1
- [G3] https://www.gledopto.eu/mediafiles/anleitungen/7016-GL-C-I-017WL.pdf
- [G4] https://www.gledopto.eu/gledopto-esp32-wled-uart_5
- [M1] https://www.meanwell.com/Upload/PDF/LRS-50/LRS-50-SPEC.PDF
- [M2] https://www.meanwell.com/Upload/PDF/LRS-100/LRS-100-SPEC.PDF
- [M3] https://www.meanwell.com/Upload/PDF/LRS-150/LRS-150-SPEC.PDF
- [M4] https://www.meanwell.com/Upload/PDF/LRS-150F/LRS-150F-SPEC.PDF
- [M5] https://www.meanwell.com/Upload/PDF/LRS-200/LRS-200-SPEC.PDF
- [M6] https://flexfireleds.com/blog/LED-power-supply-derating
- [U1] https://www.ti.com/lit/wp/slyy109b/slyy109b.pdf
- [U2] https://www.st.com/resource/en/application_note/an5225-introduction-to-usb-typec-power-delivery-for-stm32-mcus-and-mpus-stmicroelectronics.pdf
- [U3] https://ww1.microchip.com/downloads/en/Appnotes/00001914B.pdf
- [U4] https://www.anker.com/blogs/power-banks/why-10000mah-power-bank-not-actually-10000mah
- [U5] https://service.anker.com/article-description/What-is-Trickle-Charging-Mode
- [U6] https://en.wikipedia.org/wiki/Quick_Charge
- [E1] https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf (v2.4)
- [E2] https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- [E3] https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf (v5.3)
- [E4] https://www.espressif.com/sites/default/files/documentation/esp32-s2_datasheet_en.pdf
- [E5] https://github.com/espressif/esp-idf/tree/release/v5.5/components/soc (`<chip>/include/soc/soc_caps.h`)
- [E6] https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html
- [E7] https://github.com/espressif/idf-extra-components/blob/master/led_strip/README.md
- [E8] https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32c3/schematic-checklist.html
- [E9] https://github.com/espressif/esp-idf/blob/release/v5.5/README.md
- [D1] https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/
- [D2] https://www.waveshare.com/wiki/ESP32-C3-Zero
- [D3] https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c3/esp32-c3-devkitm-1/user_guide.html
- [D4] https://sigmdel.ca/michel/ha/esp8266/super_mini_esp32c3_en.html
- [D5] https://mischianti.org/esp32-c3-supermini-plus-v2-high-resolution-pinout-datasheet-and-specs/
- [D6] https://mischianti.org/esp32-c3-super-mini-high-resolution-pinout-datasheet-and-specs/
- [D7] https://hackaday.io/project/205906-esp32-c3-super-mini-wifi-fix-3-methods-that-work/details
