import base64
import json
import pathlib
import sys
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parent.parent
PRESETS = ROOT / "presets"
SCHEMES_URL = "https://lightbar.wywiol.eu/api/schemes"
SCHEMES_LIMIT = 500
ID_PREFIX = "preset-"

request = urllib.request.Request(SCHEMES_URL, headers={"User-Agent": "lightbar-embedded-export"})
with urllib.request.urlopen(request, timeout=30) as response:
    schemes = json.load(response)

if len(schemes) >= SCHEMES_LIMIT:
    sys.exit(f"API returned its limit of {SCHEMES_LIMIT} schemes, the list may be incomplete")

presets = {scheme["id"][len(ID_PREFIX) :]: base64.b64decode(scheme["data"]) for scheme in schemes if scheme["id"].startswith(ID_PREFIX)}
if not presets:
    sys.exit("API returned no presets")

PRESETS.mkdir(exist_ok=True)
for old_file in PRESETS.glob("*.bin"):
    old_file.unlink()

for name, data in presets.items():
    (PRESETS / f"{name}.bin").write_bytes(data)

print(f"Exported {len(presets)} presets")
