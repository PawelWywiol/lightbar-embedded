# Requirements

## Project

- Works with [Lightbar Web](https://github.com/PawelWywiol/lightbar-web): schemes are sent over HTTP on the local network, 2.4 GHz Wi-Fi, device runs AP + STA.
- Two device classes:
  - **A, USB powered**: small gadgets, few LEDs, the device and all LEDs powered from one USB port only.
  - **B, external power**: LED strips 3–5 m (30 / 60 / 144 LED/m) on a separate 5 V power supply (12 V only if decided later).
- Two control modes, in both classes:
  - **online**: Wi-Fi, controlled from lightbar.wywiol.eu (as now);
  - **offline**: schemes stored on the device, controlled by physical buttons or an IR remote.
- Several chip types: ESP32-C3 now; ESP32-S3 and Gledopto GL-C-016WL-D (ESP32) later, selected per PlatformIO environment.

## Firmware constraints (this repo)

- ESP-IDF, current target `esp32c3` (`sdkconfig.defaults`). Another chip family needs a new PlatformIO env / IDF target and a build + hardware test.
- Partition table needs 4 MB flash: 1.5 MB app + 2 MB LittleFS (`sdkconfig.esp32-c3-devkitm-1-partisions.csv`).
- LEDs driven by the RMT peripheral, one data GPIO (`APP_LIGHTS_GPIO`), WS2812-type 800 kHz protocol, GRB or RGB order.
- Renderer runs at 60 FPS; each tick ends with a blocking `rmt_tx_wait_all_done`, so the strip transfer must fit in one 16.7 ms tick together with color computation.
- No software current / brightness limiter. Maximum color is 100 % lightness on all three channels (full white).

## LED count limit (255) in code

Limit is 720 LEDs (5 m × 144 LED/m, the highest density found at BTF). It is set in both repos and must change together:

| Place | Limit |
|---|---|
| `src/Kconfig.projbuild` `APP_LIGHTS_COUNT` | `range 1 720` |
| `include/app_lights.h` | `LIGHTS_FRAME_COLORS_MAX 720` (`uint16_t` sizes and offsets) |
| protocol `frm2` | `pixelShift` is `int8` (−128..127 per frame) |
| lightbar-web `lib/schemes/schemeFormat.ts` | `SCHEME_MAX_COLORS = 720`, `SCHEME_MAX_FRAMES = 255` |
| lightbar-web editor | lights count 1–720 |
| lightbar-web API | scheme max 256 KiB (255 frames × 720 LEDs ≈ 187 KB) |

A scheme's colors are cached in RAM on the device; a large scheme (many frames × many LEDs) may not fit next to Wi-Fi on ESP32-C3 (not measured).

## Derived hardware requirements

Each line points to the data in [facts.md](facts.md).

1. Chip: ESP32 family with 2.4 GHz Wi-Fi and RMT; ≥ 4 MB flash. ESP8266, RP2040, ATtiny85, ATmega328P are out (not ESP-IDF 5.x / no Wi-Fi).
2. Data line timing: ≤ 576 RGB LEDs per data line at 60 FPS with this firmware's encoder (**CALC**). More LEDs need several data lines (several RMT TX channels) or a lower frame rate (720 LEDs ≈ 48 FPS).
3. Long strips on RMT: chips without RMT DMA (ESP32, ESP32-C3) refill RMT memory from interrupts; Espressif warns Wi-Fi interrupts can corrupt the transfer. ESP32-S3 has RMT DMA.
4. Data level: ESP32 guarantees only 2.64 V high level, below VIH of every 5 V WS2812-type chip (2.7 V to 3.5 V). Use a level shifter (SN74AHCT125 / SN74AHCT1G125 / SN74LV1T34) or a controller with one built in.
5. Class A power: USB port current minus controller current. USB-C chargers give 5 V only when the board has 5.1 kΩ Rd on CC1/CC2.
6. Class B power: PSU sized from LED current, fused per strip, power injected so no LED is far from a feed point; common ground between PSU, strip and controller.
7. Software current limiter (like WLED ABL) is needed for both classes, because the firmware can request full white on every LED.

## Open questions

- LED density of the future 5 m strip (voltage: 5 V decided).
- Whether class B must keep 60 FPS above ~576 LEDs (needs 2 data lines) or may drop frame rate.
- Which USB sources class A must support (computer USB 2.0 / 3.x, TV, phone charger, powerbank) and therefore the current budget.
