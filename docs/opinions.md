# User opinions

Real user reports (forums, Reddit, GitHub, hands-on tests), collected 2026-10-19. A single report is an anecdote; a pattern means several independent reports. Vendor posts are marked.

## ESP32-C3 Super Mini: pattern (10+ reports), negative

- Wi-Fi fails at default ~20 dBm TX power (AUTH_EXPIRED); works at 8.5–11 dBm or after a wire antenna mod. Cause named: clone revision with the crystal too close to the ceramic antenna ("mouse-bites" on PCB edges).
  - "4 out of 10 where unable to connect to wifi reporting error AUTH_EXPIRE" ([WLED forum, 2024-09](https://wled.discourse.group/t/some-c3-super-minis-cant-connect-to-wi-fi/12153))
  - "Always fails at 20 dB; about 3 in 10 at 17 dB and 2 in 10 at 14 dB from cold; none in 60 at 11 and 8.5 dB" ([HA community, 2026-10](https://community.home-assistant.io/t/esp32-c3-super-mini-wi-fi-auth-expired-no-tx-current-spikes-wroom-32d-works-perfectly/1021889))
  - "I'm unable to connect to Wi-Fi about 95% of the time" (new shipment) ([blog test, 2024-06](https://web.archive.org/web/2024/https://roryhay.es/blog/esp32-c3-super-mini-flaw))
  - WLED developer: "The fix is required for boards with a tiny 'crystal' radio, and for boards with an under-sized LDO. Unfortunately both are very common" ([WLED #4283](https://github.com/wled/WLED/issues/4283))
- Antenna mod helps many but not all ("If I remove the mod, it connects") ([Reddit r/esp32](https://www.reddit.com/r/esp32/comments/1k01wtp/)).
- GPIO2 as LED data: boot problems on clones ([Reddit r/WLED](https://www.reddit.com/r/WLED/comments/1p1ougm/), [WLED #5869](https://github.com/wled/WLED/issues/5869)).
- Positive: "I have a 10m strip syncing with a 5m strip, both on esp32c3" ([Reddit r/WLED](https://www.reddit.com/r/WLED/comments/1oiq63b/)).

## ESP32-C3 + WS2812 + Wi-Fi: pattern, confirmed by maintainers

- WLED developer: "known issue. disable wifi sleep and don't access it through wifi and it works fine. To fix it we need a new driver" and "yes only on C3" ([WLED #5198](https://github.com/wled/WLED/issues/5198)).
- "When the device is serving a web request it does flicker" (same issue).
- NeoPixelBus author: "the C3 (all the C chips?) is a single core. This is problematic with any routines that have long ISRs" ([WLED #4382](https://github.com/wled/WLED/issues/4382)).
- FastLED maintainer: RMT5 on S3/C3/C6/H2 "more sensitive to interrupt jitter… Under Wi-Fi, users report underruns → visible flicker" ([FastLED #2082](https://github.com/FastLED/FastLED/issues/2082)).
- XIAO ESP32C3 + FastLED: pattern "semi corrupted" once the Wi-Fi AP is enabled; same code fine on ESP32 WROOM ([Seeed forum, 2022](https://forum.seeedstudio.com/t/esp32c3-wifi-ap-led-strip-interference/267473)).
- ESP32 is not immune: "a setup with esp32 and 2 outputs (512 and 256 leds) where the second output regularly flickers especially when reloading the UI" (WLED developer, #4382).
- ESP32-S3: one WLED flicker report (Atom S3 Lite), closed as duplicate ([WLED #4993](https://github.com/wled/WLED/issues/4993)).

## Seeed XIAO ESP32C3 / ESP32S3: mostly positive

- "the Seeed Studio XIAO ESP32C3 is of excellent quality and well documented" ([Arduino forum, 2026-04](https://forum.arduino.cc/t/esp32-c3-super-mini-wifi-test-not-working-solved/1440107))
- "If you're wanting fairly cheap and still reliable, get the xiao boards" ([Reddit r/esp32](https://www.reddit.com/r/esp32/comments/1rbr9pl/))
- XIAO ESP32S3 FPC antenna inside enclosures: "when I enclosed everything wi-fi is not connecting"; counter-report: works in polycarbonate enclosures ([Seeed forum](https://forum.seeedstudio.com/t/xiao-esp32s3-antenna-wi-fi-not-working/272978)).
- XIAO ESP32S3 Sense (camera) runs hot, ~75 °C idle (one report) ([Seeed forum](https://forum.seeedstudio.com/t/my-xiao-esp32s3-with-camera-attached-is-overheating/276523)).

## Waveshare ESP32-C3-Zero / S3-Zero, ESP32-S3 Super Mini: pattern, negative Wi-Fi

- "As expected and reported by many users of this board the unmodified board has a bad WIFI-performance"; antenna mod "+10 dB" ([Elektor Labs, 2025-05](https://www.elektormagazine.nl/labs/esp32-s3-zero-antenna-mod)).
- "soldering the Mini Zero using the castelated pins killed the WiFi function" ([mischianti forum, 2026-09](https://mischianti.org/forums/topic/waveshare-esp32s3-zero-wifi-antenna/)).
- "Reminder: ESP32-S3 Super Mini Antenna is BAD" ([Reddit r/esp32, 2026-04](https://www.reddit.com/r/esp32/comments/1s9rj3d/)).

## Lolin S2 Mini: pattern, clone Wi-Fi faults

- "There are many Wemos S2 mini clones with faults. Some of them reset during the establishment of the Wi-Fi connection" ([OpenEPaperLink wiki](https://github.com/OpenEPaperLink/OpenEPaperLink/wiki/Troubleshooting-Access-Points)).

## Gledopto: mixed

- GL-C-016WL-D: "No different than any other esp32. Main disadvantage is a weak level shifter which limits cable distance and low max current if you power through the device" ([Reddit r/WLED, 2025-03](https://www.reddit.com/r/WLED/comments/1jkbmd8/)), the only 016-specific opinion found.
- Older models (015WL, MC-001WL): "a very weak output on the data lines (1KOhm resistor)" ([WLED forum, 2025-11](https://wled.discourse.group/t/random-leds-turning-on-after-switch-off/14920)); vendor: "the level shifter IC issue was already resolved in 2024" ([Reddit r/WLED, 2026-07](https://www.reddit.com/r/WLED/comments/1v74v1n/), vendor post).
- Positive: "Gledopto wins this contest… they have a built in relay and a great, highly reachable support staff" ([Reddit r/WLED](https://www.reddit.com/r/WLED/comments/1t0r5kx/)).
- No independent teardown or failure report for 016WL-D / 017WL-D found.

## QuinLED: positive, one known hardware issue

- "Every output is protected with fuses. No damage to ESP32 or components when an oops happens" ([Reddit r/WLED](https://www.reddit.com/r/WLED/comments/1mikki3/)).
- Maker on Dig-Uno / Dig-Quad v3.5r3: "the IC in the DC-DC converter when running at 24V is too close to its limits and will sometimes fail… maybe 10 to 15 cases at the most"; a user lost 3 of 3 Quads ([Reddit r/WLED, 2026-07](https://www.reddit.com/r/WLED/comments/1v2vj17/)).

## Athom: polarised

- "22 Addressable Controllers… Success rate: 59%" vs "I have about 5 pcs… they work fine for me" ([WLED forum](https://wled.discourse.group/t/psa-dont-buy-athom/10886)).
- "I've had products arrive bricked" ([Reddit r/WLED](https://www.reddit.com/r/WLED/comments/1ucn7gc/)).

## LED strips

- BTF: "Usually people trust BTF lighting LED products" ([WLED forum](https://wled.discourse.group/t/3316)); one user: 8 of 8 outdoor SK6812 5 m strips failed ([WLED forum](https://wled.discourse.group/t/3896)).
- Outdoor: "Dead pixels (and then dead strips) happen a LOT"; IP65 strips "corrode from the back"; recommends IP68 WS2815 and a few days burn-in before installing ([WLED forum](https://wled.discourse.group/t/15329)).
- One dead pixel at the start stops the rest of the strip ([WLED forum](https://wled.discourse.group/t/3398)).
- WLED author: "highly recommend SK6812 with an extra white channel… white color rendering index of RGB LEDs is absolutely terrible" ([WLED forum](https://wled.discourse.group/t/4540)).
- Power supplies: two cheap PSUs flickered, Mean Well worked ([WLED forum](https://wled.discourse.group/t/11296), anecdote with A/B test); Quindor teardown: "no reason left to buy a Chinese power supply" ([blog, 2016](https://blog.quindorian.org/2016/06/led-power-supply-china-vs-mean-well.html)).

## Not found

User opinions on Waveshare ESP32-S3-RS485-WLED, independent Dig-Next-2 reviews, Gledopto 016/017 teardowns, IP65 yellowing from users, BTF vs no-name copper thickness measurements.
