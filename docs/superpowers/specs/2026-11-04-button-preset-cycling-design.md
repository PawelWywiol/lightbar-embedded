# Button preset cycling

## Goal

A board with a user button plays preset schemes stored on the device. A click moves to the next preset; a 3 s hold starts automatic cycling. Boards without a button behave as they do today.

## Decisions

- Device list: presets from Lightbar Web only. User schemes are out of scope.
- Each family contributes one version: the size closest to the strip length, as in the web gallery.
- Storage: LittleFS, because Lightbar Web will later replace the list on the device.
- The firmware has no size logic. Whoever writes the list chooses the versions: the build today, Lightbar Web later.
- The list is limited by partition space only; it is not held in RAM.

## Behavior

| Action | Scheme | Auto |
|---|---|---|
| Click (released before 3 s) | next preset | off |
| Hold reaches 3 s, while still pressed | next preset, as visible confirmation | on, interval restarts |
| Release after a hold | none | none |
| Auto interval elapsed | next preset | stays on |
| Frames from the web, saved or preview | web scheme plays | off |
| Brightness, speed or hue change | none | none |

- Next preset: the file after the last played preset in name order, wrapping to the first. After a web scheme, the sequence continues from the last played preset.
- With no preset played yet, next preset is the first file.
- Brightness, speed and hue are kept on every scheme change.
- A file that fails to load is skipped; the search stops after one full pass.
- An empty or missing list only logs a message.
- A button action invalidates queued web requests through `next_lights_request_sequence`, so the latest action wins.

## Components

### Button: `src/app_button.c`

- `APP_BUTTON_GPIO`, default `-1`. At `-1` no timer or GPIO is set up.
- `sdkconfig.c3-super-mini` sets `9` (BOOT, active low, internal pull-up). Other boards stay at `-1` until their pin is confirmed.
- A 20 ms `esp_timer` polls the level, which also debounces.
- The timer callback only posts `APP_EVENT_BUTTON_CLICK` or `APP_EVENT_BUTTON_HOLD`.

### Presets: `src/app_presets.c`

- Owns the last preset name, the auto flag and the auto timer.
- Handles the button events and the auto timer event on the default event loop, as scheme loading does today.
- `APP_PRESETS_AUTO_INTERVAL` holds the interval in seconds, default 30.
- Lists `/storage/presets` on each step and picks the smallest name greater than the last one, so order does not depend on `readdir`.

### Lights: `src/app_lights.c`

- Exposes loading a scheme from a path, reused by web and preset flows.
- A mutex covers scheme loading and the frame work in the lights loop. Loading frees frame memory, and the event task can preempt the loop mid-frame; found by reading the code, not reproduced.
- Accepted web frames turn auto off through `app_presets`.

### Persistence

- NVS stores the preset name and auto flag, saved 3 s after a click or hold, like brightness.
- Auto steps do not save, so auto does not wear the flash. After a reboot in auto mode, cycling resumes from the preset chosen by the last user action.
- A web scheme saved with a `head` chunk clears the stored name and auto flag.
- A web preview clears the auto flag and keeps the stored name.
- Boot: a stored preset that exists is loaded; otherwise `lights/current` is loaded as today. If the stored file is gone, the next file in name order is used. A stored auto flag restarts the auto timer.

### List generation

- `make presets` exports all `preset-*` schemes from the public `GET /api/schemes` of Lightbar Web to `presets/<id without "preset-">.bin`, unchanged scheme bytes, removing older files. Admins edit presets in the app, so migrations are not the current list. These files are committed.
- A PlatformIO extra script runs for `buildfs` and `uploadfs`. It stages `data/` plus the selected presets under `presets/` in the env build directory and points `PROJECT_DATA_DIR` at it.
- Strip length comes from the generated `sdkconfig.<env>` when present, otherwise from the stacked `SDKCONFIG_DEFAULTS` of the env.
- Selection follows `selectSchemesForLights` in `lightbar-web/components/schemesGallery/schemeFamilies.ts`:
  - a family member is named `<family> <N>` where `N` equals the color count of its first frame;
  - the member closest to the strip length wins; on a tie, the smaller one;
  - every other scheme is kept, so `Scanner` and `Scanner <N>` are both listed.
- The list at implementation time gives 91 presets: 28 without a size and 7 families of 9 sizes, so 35 files per board.

## Docs

The README gains the button behavior, the BOOT strapping note and the need for `uploadfs` after `make presets`. `uploadfs` overwrites the partition, so `lights/current` is lost; WiFi credentials stay in NVS.

## Out of scope

- Uploading a chosen list from Lightbar Web and the free-space margin it needs.
- Changing the auto interval at runtime.
- Showing the auto mode in Lightbar Web.
- Reading user schemes from the production database.

## Verification

The repo has no firmware tests.

Automated:

- Build all environments.
- Check the staged image of each environment holds 35 files with the expected versions (86 LEDs: size 64, 256 LEDs: size 256).

On a C3 Super Mini:

- BOOT reads on GPIO 9 on the tested unit.
- Click after a web scheme moves to a preset.
- Hold starts cycling, jumps at once, then steps every 30 s.
- Click and web frames each leave auto; brightness, speed and hue changes do not.
- Adjustments survive all scheme changes.
- Reboot restores a clicked preset, auto mode and a saved web scheme.
- Rapid clicking causes no reset (`resetReason` stays unchanged, `uptime` keeps growing).
- A board without a button builds with `-1` and behaves as before.

## To confirm during implementation

- PlatformIO uses the replaced `PROJECT_DATA_DIR` for the image.
- `littlefs_create_partition_image` in `src/CMakeLists.txt` does not overwrite the staged image during `uploadfs`.
- Generated `sdkconfig.<env>` files pick up the new options, or must be deleted first.
