# Lightbar Embedded

ESP-IDF firmware for an ESP32-C3 driving an addressable RGB LED strip. Schemes are created in [Lightbar Web](https://github.com/PawelWywiol/lightbar-web) (https://lightbar.wywiol.eu) and sent to the device over HTTP.

## Features

- WiFi AP + STA at the same time: own access point for setup, home network for daily use
- HTTP API `/api/lightbar` (info, WiFi credentials, schemes) with CORS
- Device page: WiFi form (when opened over the AP) and `postMessage` bridge for the HTTPS web app
- 60 FPS frame renderer: fade, shuffle, hue shift, pixel shift, tempo per frame
- Last scheme saved in LittleFS and restored on boot, WiFi and AP credentials in NVS

## Hardware

- ESP32-C3 DevKitM-1 (4 MB flash)
- WS2812B-compatible strip on GPIO 10 (RMT, GRB order by default)
- 5 V supply sized for the strip

## Layout

```
src/
  main.c             init, POST body parsing (wifi / head / frame chunks)
  app_server.c       HTTP server, /api/lightbar, static files, CORS
  app_network.c      AP + STA WiFi
  app_lights.c       scheme cache, frame timing, colors, strip output
  app_nvs.c          NVS read/write
  app_vfs.c          LittleFS mount and helpers
  app_utils.c        credentials defaults, uid
  Kconfig.projbuild  app settings (menuconfig "App Config")
include/             headers, protocol constants (app_defines.h)
lib/led_strip_encoder  RMT encoder for the strip
ui/                  device page (vanilla TS, Vite, built into one gzipped file)
data/                LittleFS image: public_html/index.html.gz
sdkconfig.defaults   shared config; sdkconfig.release / sdkconfig.debug add build layers
sdkconfig.esp32-c3-devkitm-1-partisions.csv  nvs, phy_init, factory app 1.5 MB, storage (LittleFS) 2 MB
```

## Requirements

- PlatformIO (`platformio/espressif32`, ESP-IDF framework)
- Node.js 24 and pnpm 12 for the device page (`ui/`)

## Build and flash

PlatformIO environments (generated `sdkconfig.<env>` files are not committed):

- `esp32-c3-devkitm-1` (default, release): `-O2`, no logs, no console, silent asserts, silent reboot on panic
- `esp32-c3-devkitm-1-debug`: `-Og`, info logs

```bash
make build          # release build
make upload         # release build + flash
make build-debug    # debug build (upload-debug, flash-debug = + serial monitor)
make monitor        # serial monitor, 115200
make flash          # upload + monitor
make clean
make format         # clang-format src/ include/
make ui             # device page: install, lint, fmt check, test, build -> data/public_html
pio run -t uploadfs # flash LittleFS image (data/)
```

Device page dev server with API proxied to a device:

```bash
cd ui && DEVICE_URL=http://1.2.3.4 pnpm dev
```

## Configuration

`pio run -t menuconfig` → App Config:

| Option | Default |
|---|---|
| `APP_AP_SSID_PREFIX` | `lightbar` (SSID `lightbar-<random>`, generated once, kept in NVS) |
| `APP_AP_PASSWORD` | `lightbar` |
| `APP_DHCP_IP_START` | `1.2.3.4` (device address on its AP) |
| `APP_STA_MAXIMUM_RETRY` / `APP_STA_CONNECT_TIMEOUT` | 5 / 15 s |
| `APP_LIGHTS_COUNT` | 16 (1–255) |
| `APP_LIGHTS_GPIO` | 10 |
| `APP_LIGHTS_COLOR_ORDER` | GRB (or RGB) |

## Setup

1. Join the `lightbar-…` WiFi (password `lightbar`) and open http://1.2.3.4.
2. Submit the home network SSID and password; the device connects as STA and stores them in NVS.
3. Get the device IP in the home network from `host` in `GET http://1.2.3.4/api/lightbar` (or the router, or the scan in Lightbar Web) and add it in Lightbar Web.

## API

`GET /api/lightbar`:

```json
{ "type": "info", "data": { "leds": 16, "proto": 1, "uid": "lightbar-…", "network": 2, "host": "…" } }
```

- `uid`: AP SSID
- `network`: 1 = request came over the home network, 2 = over the AP (then `host` = home network IP)

`POST /api/lightbar`: binary body of chunks, each: type (4 B), payload size (4 B), payload, `EOL\0` (little-endian `uint32`).

| Type | Payload |
|---|---|
| `wifi` | `ssid[33]`, `password[65]` (zero-terminated) |
| `head` | `[version=1, nameLength, name…]` |
| `fram` | `[flags, tempo, colors…]` (legacy) |
| `frm2` | `[flags, pixelShift (int8), tempo, colors…]` |

- flags: bit 0 fade, bit 1 shuffle, bits 2–7 hue shift (signed, −32..31)
- tempo: 1–240 BPM, 241–254 = 255 − value ticks at 60 FPS, 255 = 1 tick
- color: bits 6–7 lightness, bits 0–5 hue (0–62); hue 63 achromatic (`0x3f` off, `0xff` white)
- frames shorter than the strip repeat along it; shuffle permutes each repeated segment
- hue shift and pixel shift accumulate per frame for the whole scheme, reset on scheme load

A body with frames is played right away; with a `head` chunk it is also saved as `/storage/lights/current` and restored on boot.

Other `GET` paths serve files from `/storage/public_html` (`.gz` preferred).

## Web app bridge

The web app runs over HTTPS, so it can open `http://<device>/#bridge=<origin>` in a popup and send `info` / `send` requests through `postMessage`. Allowed origins: `https://lightbar.wywiol.eu` and `*-lightbar-web.pawel-wywiol.workers.dev` previews (`ui/src/bridge.ts`).

## Release order

When the scheme format changes, flash firmware first, then deploy the web app.

## License

GPL-3.0, see [LICENSE](LICENSE).
