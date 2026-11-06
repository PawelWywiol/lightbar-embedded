# ESP-NOW remote (Gledopto GL-RC-001WL)

## Goal

Selected devices are controlled by the Gledopto GL-RC-001WL remote over ESP-NOW. Remote support is chosen when the firmware is built.

## Remote facts (verified with a sniffer on ESP32-C3, 2026-11-06)

- Broadcast ESP-NOW, 13 bytes: `[0]` 0x91 for on, 0x81 otherwise; `[1..4]` sequence u32 LE; `[5]` 0x20; `[6]` button; `[7]` 0x01; `[8]` battery 0-100; rest 0.
- Every press is a burst of 1-20 copies with the same sequence.
- Codes: on 1, off 2, moon 3, dimmer 8, brighter 9, S1 16, S2 17, S3 18, S4 19.
- Received on channels 1, 2, 4, 6, 10-13, so the device listens on whatever channel it uses (router or AP channel 1).
- Holding a button sends no repeats; hold is not detectable.

## Behavior

| Button | Action |
| --- | --- |
| on | start the animation, fade from 0 to the saved brightness |
| off | fade to 0, then stop the animation |
| brighter / dimmer | brightness ±`APP_REMOTE_BRIGHTNESS_STEP`, limited to `APP_REMOTE_BRIGHTNESS_MIN`..100 |
| moon | speed cycle 100 → 150 → 200 → 25 → 50 → 100; a value outside the cycle goes to 100 |
| S2 | hue +4, wrapping within -32..31 (16 presses return to 0) |
| S1 | next preset and auto cycling, like holding the board button; again restarts the interval |
| S3 | previous preset, leaves auto |
| S4 | next preset, leaves auto |

- Any remote with this packet format is accepted, no pairing.
- While off only `on` works; board button clicks and auto steps are ignored, auto resumes after `on`.
- Power state is stored in NVS and survives a reboot; a device that was off stays dark.
- A scheme sent from Lightbar Web turns the device on (fade in).
- Remote adjustments use `set_lights_adjust`, so they are saved like web adjustments.

## Build selection

Effective `CONFIG_APP_REMOTE_ENABLED`, recomputed on every build by a PlatformIO pre script:

1. `REMOTE=1` / `REMOTE=0` environment variable, if set;
2. otherwise the last sdkconfig layer defining it (e.g. `sdkconfig.gledopto` for a permanent remote);
3. otherwise `n`.

The script writes the value into the generated `sdkconfig.<env>`, which wins over layers. Implementation starts by verifying ESP-IDF picks up the change and rebuilds.

Presets (list, saved preset, auto) are active when the board has a button or the remote is enabled.

## Design

- `app_remote.c` (new): with the remote enabled, `esp_now_init` after WiFi starts; receive callback validates length 13, `[5]==0x20`, `[7]==0x01`, `[0]` 0x91/0x81, drops a repeated sequence and posts `APP_EVENT_REMOTE_BUTTON` with the code. Handler on the default event loop maps codes to actions.
- `app_lights.c`: `set_lights_power(bool)` and `is_lights_power_on()`. A power level multiplier combined with brightness in `scale_brightness`, stepped by the lights loop over `APP_LIGHTS_POWER_FADE_MS`. At level 0 with power off frames stop advancing and the strip gets zeros. Power state saved in NVS (`APP_NVS_LIGHTS_POWER_KEY`). A web scheme calls `set_lights_power(true)`.
- `app_presets.c`: events renamed to `APP_EVENT_PRESETS_NEXT`, `APP_EVENT_PRESETS_PREVIOUS`, `APP_EVENT_PRESETS_AUTO`; board button posts NEXT/AUTO; `find_previous_preset` added; events ignored while off; `restore_preset` gate becomes button or remote.
- Kconfig: `APP_REMOTE_ENABLED` (n), `APP_REMOTE_BRIGHTNESS_STEP` (10), `APP_REMOTE_BRIGHTNESS_MIN` (5), `APP_LIGHTS_POWER_FADE_MS` (1000), `APP_NVS_LIGHTS_POWER_KEY`.
- `scripts/remote_option.py` (new pre script) with unit tests for the value resolution.

## Testing

- Python unit tests for the build option resolution.
- All envs build with and without `REMOTE=1`.
- Hardware on C3 Super Mini (debug env with logs): every button, duplicates ignored, off → reboot stays off, web send while off turns on, board button and auto ignored while off, AP mode (channel 1) reception, flashing without `REMOTE=1` removes the remote.
