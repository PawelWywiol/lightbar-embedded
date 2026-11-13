import re
import struct

HEAD = b"daeh"
FRAME = b"3mrf"
FRAME_HEADER_SIZE = 4
SIZED_NAME = re.compile(r"^(.+) (\d+)$")


def parse_scheme(data):
    name = ""
    offset = 0
    while offset < len(data):
        kind = data[offset : offset + 4]
        size = struct.unpack_from("<I", data, offset + 4)[0]
        payload = data[offset + 8 : offset + 8 + size]
        if kind == HEAD:
            name = payload[2 : 2 + payload[1]].decode()
        elif kind == FRAME:
            return name, size - FRAME_HEADER_SIZE
        offset += 12 + size
    raise ValueError("scheme has no frames")


def family_of(name, count):
    match = SIZED_NAME.match(name)
    return match.group(1) if match and int(match.group(2)) == count else None


def select_presets(schemes, lights_count):
    closest = {}
    for file_name, (name, count) in schemes.items():
        family = family_of(name, count)
        if family is None:
            continue
        current = closest.get(family)
        rank = (abs(count - lights_count), count)
        if current is None or rank < current[0]:
            closest[family] = (rank, file_name)

    selected = {file_name for _, file_name in closest.values()}
    return sorted(
        file_name
        for file_name, (name, count) in schemes.items()
        if family_of(name, count) is None or file_name in selected
    )
