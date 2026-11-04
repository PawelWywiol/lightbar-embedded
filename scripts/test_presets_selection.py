import struct
import unittest

from presets_selection import parse_scheme, select_presets


def chunk(kind, payload):
    return kind + struct.pack("<I", len(payload)) + payload + b"\x00LOE"


def scheme(name, colors):
    encoded = name.encode()
    head = bytes([1, len(encoded)]) + encoded
    return chunk(b"daeh", head) + chunk(b"2mrf", bytes([0, 0, 60]) + bytes(colors))


class ParseSchemeTest(unittest.TestCase):
    def test_reads_name_and_first_frame_size(self):
        self.assertEqual(parse_scheme(scheme("Comet 16", [1] * 16)), ("Comet 16", 16))

    def test_reads_legacy_frame_size(self):
        data = chunk(b"daeh", bytes([1, 1]) + b"A") + chunk(b"marf", bytes([0, 60, 1, 2]))
        self.assertEqual(parse_scheme(data), ("A", 2))


class SelectPresetsTest(unittest.TestCase):
    def setUp(self):
        self.schemes = {
            "breathing.bin": ("Breathing", 8),
            "comet-64.bin": ("Comet 64", 64),
            "comet-128.bin": ("Comet 128", 128),
            "scanner.bin": ("Scanner", 16),
            "scanner-8.bin": ("Scanner 8", 8),
            "scanner-16.bin": ("Scanner 16", 16),
            "only-256.bin": ("Only 256", 16),
        }

    def test_picks_closest_size_and_keeps_others(self):
        self.assertEqual(
            select_presets(self.schemes, 86),
            ["breathing.bin", "comet-64.bin", "only-256.bin", "scanner-16.bin", "scanner.bin"],
        )

    def test_tie_prefers_smaller_size(self):
        self.assertIn("comet-64.bin", select_presets(self.schemes, 96))


if __name__ == "__main__":
    unittest.main()
