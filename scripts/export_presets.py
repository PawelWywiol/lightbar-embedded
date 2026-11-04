import pathlib
import sqlite3

ROOT = pathlib.Path(__file__).resolve().parent.parent
MIGRATIONS = ROOT.parent / "lightbar-web" / "migrations"
PRESETS = ROOT / "presets"
ID_PREFIX = "preset-"

database = sqlite3.connect(":memory:")
for migration in sorted(MIGRATIONS.glob("*.sql")):
    database.executescript(migration.read_text())

PRESETS.mkdir(exist_ok=True)
for old_file in PRESETS.glob("*.bin"):
    old_file.unlink()

rows = database.execute("SELECT id, data FROM schemes WHERE id LIKE ?", (ID_PREFIX + "%",)).fetchall()
for scheme_id, data in rows:
    (PRESETS / f"{scheme_id[len(ID_PREFIX):]}.bin").write_bytes(data)

print(f"Exported {len(rows)} presets")
