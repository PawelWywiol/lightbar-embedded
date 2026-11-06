# Lightbar Embedded

ESP-IDF firmware for an ESP32-C3 driving an addressable RGB LED strip. Schemes are created in [Lightbar Web](https://github.com/PawelWywiol/lightbar-web) (https://lightbar.wywiol.eu) and sent to the device over HTTP.

The device runs its own WiFi access point for setup and joins the home network for daily use. The last scheme is restored on boot.

## Hardware

- ESP32-C3 DevKitM-1 (4 MB flash)
- WS2812B-compatible strip on GPIO 10
- 5 V supply sized for the strip

Other boards are PlatformIO environments (`pio run -e <env>`, see `platformio.ini`). `esp32-s3-devkitc-1` builds but is not tested on hardware yet. `gledopto-gl-c-016wl-d` (data GPIO 16, output relay GPIO 18) is tested on hardware; the manual says relay GPIO 12, but only GPIO 18 powered the output on the tested unit, matching its factory WLED config.

Each board + strip pair is one environment, its `SDKCONFIG_DEFAULTS` stacks one file per layer: common, board (`sdkconfig.gledopto`), strip and power supply (`sdkconfig.ws2811-256-6a`). Values in a generated `sdkconfig.<env>` win over these files, so delete it after changing a layer.

ESP32-C3 Super Mini: env `c3-super-mini-ws2811-86-usb`. The tested unit got `AUTH_EXPIRE` (reason 2) at default power and connects at 11 dBm (`APP_WIFI_MAX_TX_POWER`); `CONFIG_ESP_PHY_MAX_WIFI_TX_POWER=11` did not help. Weak range: [wire antenna](docs/c3-super-mini-antenna.md).

GL-C-016WL-D on macOS: its USB chip (CH340K, `1a86:7522`) needs the [WCH driver](https://github.com/WCHSoftGroup/ch34xser_macos); reading flash above 115200 baud returned corrupted data.

Strip length, GPIO, color order and network defaults: `pio run -t menuconfig` → App Config.

Choosing controllers, strips and power supplies: [docs](docs/README.md).

## Button

Boards with a user button (`APP_BUTTON_GPIO`, set only on the C3 Super Mini: BOOT, GPIO 9) play presets stored on the device. Boards without one behave as if the feature did not exist.

- Click: next preset, leaves auto cycling.
- Hold 3 s: next preset, starts auto cycling.
- A scheme sent from Lightbar Web leaves auto cycling. Brightness, speed and hue stay across all changes.
- The last preset and auto cycling survive a reboot; a scheme saved from Lightbar Web replaces them.

BOOT is a strapping pin: pressed during power-up or reset, the board enters download mode instead of the firmware.

The list comes from the production Lightbar Web API, because admins edit presets in the app and migrations do not follow: `make presets` exports it to `presets/`, and `pio run -t uploadfs` stages the versions closest to the strip length of that environment, using the same rule as the web gallery. `uploadfs` overwrites the last scheme from Lightbar Web; WiFi credentials stay.

## Remote

Builds with `APP_REMOTE_ENABLED` accept any Gledopto GL-RC-001WL (WiZmote packet format) over ESP-NOW, without pairing, on whatever WiFi channel the board uses. `REMOTE=1` or `REMOTE=0` in the build command overrides the sdkconfig layers, e.g. `REMOTE=1 pio run -e c3-super-mini-debug -t upload`; without it the last layer setting the option decides, otherwise it is off. The value is written to the generated `sdkconfig.<env>` on every build, so it never sticks from a previous flash.

| Button | Action |
| --- | --- |
| on / off | fade in and start, fade out and stop |
| brighter / dimmer | brightness step, never below the minimum |
| moon | speed ×1 → ×1.5 → ×2 → ×0.25 → ×0.5 |
| S2 | hue +4, back to 0 after 16 presses |
| S1 / S3 / S4 | auto cycling / previous / next preset |

The remote sends nothing while a button is held, so there are no long presses. While off only `on` works, and the board button and auto cycling wait. The power state survives a reboot; a scheme from Lightbar Web turns the lights on.

## Build and flash

`make` lists targets. `make upload` flashes only the firmware; the device page lives in the LittleFS image, so flash it with `pio run -t uploadfs` on first flash and after `make ui`.

Device page dev server with API proxied to a device:

```bash
cd ui && DEVICE_URL=http://1.2.3.4 pnpm dev
```

## Setup

1. Join the `lightbar-…` WiFi (password `lightbar`) and open http://1.2.3.4.
2. Submit the home network SSID (max 31 chars) and password (required, open networks not supported).
3. The page shows connection progress, the problem if it fails, and the device IP in the home network. Connecting can move the device to another WiFi channel and drop the page's connection; rejoin the `lightbar-…` WiFi if the page reports no response.
4. Add the device IP in Lightbar Web.

## API

Contract shared with Lightbar Web.

`GET /api/lightbar`:

```json
{ "type": "info", "data": { "leds": 16, "proto": 1, "uid": "lightbar-…", "network": 2, "host": "…" } }
```

- `adjust`: `{ "brightness", "speed", "hue" }`, see `ctrl`
- `uid`: AP SSID, ends with the last 3 bytes of the AP MAC address
- `network`: 1 = request came over the home network, 2 = over the AP (then `host` = home network IP, and `wifi` = home network connection)
- `link`: `{ "rssi", "drops", "dropReason", "uptime", "resetReason" }`; `rssi` in dBm, 0 = not connected; `drops` = lost connections since boot; `dropReason` = `wifi_err_reason_t` of the last one; `uptime` in seconds; `resetReason` = `esp_reset_reason_t`
- `wifi`: `{ "state", "ssid", "reason", "attempt" }`; state 0 = no credentials, 1 = connecting, 2 = waiting for IP, 3 = connected; `reason` = last ESP-IDF `wifi_err_reason_t` since the last success or credentials change, 0 = none

`POST /api/lightbar`: binary body of chunks, each: type (4 B), payload size (4 B), payload, `EOL\0` (little-endian `uint32`). Responds `{"ok":true}` once the body is stored; processing runs afterwards.

| Type | Payload |
|---|---|
| `wifi` | `ssid[33]`, `password[65]` (zero-terminated) |
| `head` | `[version=1, nameLength, name…]` (ignored by firmware, marks the scheme to be saved) |
| `fram` | `[flags, tempo, colors…]` (legacy) |
| `frm2` | `[flags, pixelShift (int8), tempo, colors…]` |
| `ctrl` | `[brightness (0–100), speed (25–200, % of tempo), hue (int8, −32..31)]`, applied at once and kept across schemes and reboots (saved 3 s after the last change); brightness follows a gamma 2.2 curve; invalid values are ignored |

- flags: bit 0 fade, bit 1 shuffle, bits 2–7 hue shift (signed, −32..31)
- tempo: 1–240 BPM, 241–254 = 255 − value ticks at 60 FPS, 255 = 1 tick
- color: bits 6–7 lightness, bits 0–5 hue (0–62); hue 63 achromatic (`0x3f` off, `0xff` white)
- frames shorter than the strip repeat along it; shuffle permutes each repeated segment
- hue shift and pixel shift accumulate per frame for the whole scheme, reset on scheme load

A body with frames and no other chunk types is played right away; with a `head` chunk it is also saved and restored on boot. A `ctrl` chunk may accompany frames.

## Web app bridge

The web app runs over HTTPS and cannot call the device over plain HTTP, so it opens `http://<device>/#bridge=<origin>` in a popup and sends `info` / `send` requests through `postMessage`. Allowed origins are listed in `ui/src/bridge.ts`.

## Release order

When the scheme format changes, flash firmware first, then deploy the web app.

## License

GPL-3.0, see [LICENSE](LICENSE).
