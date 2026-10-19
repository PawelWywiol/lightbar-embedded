# Lightbar Embedded

ESP-IDF firmware for an ESP32-C3 driving an addressable RGB LED strip. Schemes are created in [Lightbar Web](https://github.com/PawelWywiol/lightbar-web) (https://lightbar.wywiol.eu) and sent to the device over HTTP.

The device runs its own WiFi access point for setup and joins the home network for daily use. The last scheme is restored on boot.

## Hardware

- ESP32-C3 DevKitM-1 (4 MB flash)
- WS2812B-compatible strip on GPIO 10
- 5 V supply sized for the strip

Other boards are PlatformIO environments (`pio run -e <env>`, see `platformio.ini`). `esp32-s3-devkitc-1` and `gledopto-gl-c-016wl-d` (data GPIO 16, output relay GPIO 12) build but are not tested on hardware yet.

Strip length, GPIO, color order and network defaults: `pio run -t menuconfig` → App Config.

Choosing controllers, strips and power supplies: [docs](docs/README.md).

## Build and flash

`make` lists targets. `make upload` flashes only the firmware; the device page lives in the LittleFS image, so flash it with `pio run -t uploadfs` on first flash and after `make ui`.

Device page dev server with API proxied to a device:

```bash
cd ui && DEVICE_URL=http://1.2.3.4 pnpm dev
```

## Setup

1. Join the `lightbar-…` WiFi (password `lightbar`) and open http://1.2.3.4.
2. Submit the home network SSID (max 31 chars) and password (required, open networks not supported).
3. Get the device IP in the home network from `host` in `GET http://1.2.3.4/api/lightbar` (or the router, or the scan in Lightbar Web) and add it in Lightbar Web.

## API

Contract shared with Lightbar Web.

`GET /api/lightbar`:

```json
{ "type": "info", "data": { "leds": 16, "proto": 1, "uid": "lightbar-…", "network": 2, "host": "…" } }
```

- `uid`: AP SSID
- `network`: 1 = request came over the home network, 2 = over the AP (then `host` = home network IP)

`POST /api/lightbar`: binary body of chunks, each: type (4 B), payload size (4 B), payload, `EOL\0` (little-endian `uint32`). Responds `{"ok":true}` once the body is stored; processing runs afterwards.

| Type | Payload |
|---|---|
| `wifi` | `ssid[33]`, `password[65]` (zero-terminated) |
| `head` | `[version=1, nameLength, name…]` (ignored by firmware, marks the scheme to be saved) |
| `fram` | `[flags, tempo, colors…]` (legacy) |
| `frm2` | `[flags, pixelShift (int8), tempo, colors…]` |

- flags: bit 0 fade, bit 1 shuffle, bits 2–7 hue shift (signed, −32..31)
- tempo: 1–240 BPM, 241–254 = 255 − value ticks at 60 FPS, 255 = 1 tick
- color: bits 6–7 lightness, bits 0–5 hue (0–62); hue 63 achromatic (`0x3f` off, `0xff` white)
- frames shorter than the strip repeat along it; shuffle permutes each repeated segment
- hue shift and pixel shift accumulate per frame for the whole scheme, reset on scheme load

A body with frames and no other chunk types is played right away; with a `head` chunk it is also saved and restored on boot.

## Web app bridge

The web app runs over HTTPS and cannot call the device over plain HTTP, so it opens `http://<device>/#bridge=<origin>` in a popup and sends `info` / `send` requests through `postMessage`. Allowed origins are listed in `ui/src/bridge.ts`.

## Release order

When the scheme format changes, flash firmware first, then deploy the web app.

## License

GPL-3.0, see [LICENSE](LICENSE).
