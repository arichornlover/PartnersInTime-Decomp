#!/usr/bin/env python3
"""Validate a headless pit_launcher BMP screenshot.

The launcher writes a 24-bit BMP via ``--screenshot``. This checks that the
image is well formed, has the expected window size, and contains the theme's
signature colours, so a theme or layout regression fails the standard
ROM-independent check gate without a display.

    python tools/check_launcher.py build/ui-frames/launcher-home.bmp

Exit status is 0 on success and 1 on any failure.
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

# (name, r, g, b, minimum pixel count). The sidebar and card fills always
# appear; gold and cream are the accent and body-text colours drawn by the
# theme, so their absence means the style or text pipeline broke.
EXPECTED_COLORS = (
    ("sidebar", 0x12, 0x1A, 0x38, 2000),
    ("card", 0x14, 0x1C, 0x38, 2000),
    ("gold", 0xF2, 0xB2, 0x33, 40),
    ("cream text", 0xF4, 0xEF, 0xE0, 40),
)


def die(message: str) -> "None":
    print(f"FAIL: {message}")
    raise SystemExit(1)


def load_bmp(path: Path):
    raw = path.read_bytes()
    if len(raw) < 54 or raw[0:2] != b"BM":
        die(f"{path} is not a BMP")
    data_offset = struct.unpack_from("<I", raw, 10)[0]
    width = struct.unpack_from("<i", raw, 18)[0]
    height = struct.unpack_from("<i", raw, 22)[0]
    bpp = struct.unpack_from("<H", raw, 28)[0]
    if bpp != 24:
        die(f"{path} is {bpp}-bit; expected 24-bit")
    if height < 0:
        die(f"{path} is top-down; pit_launcher writes bottom-up BMPs")
    row = (width * 3 + 3) & ~3
    if data_offset + row * height > len(raw):
        die(f"{path} is truncated")
    return raw, data_offset, width, height, row


def pixel(raw, offset, width, height, row, x, y):
    pos = offset + (height - 1 - y) * row + x * 3
    b, g, r = raw[pos], raw[pos + 1], raw[pos + 2]
    return r, g, b


def main(argv):
    if len(argv) != 2:
        die("usage: check_launcher.py <screenshot.bmp>")
    path = Path(argv[1])
    if not path.exists():
        die(f"{path} does not exist")

    raw, offset, width, height, row = load_bmp(path)
    if (width, height) != (1200, 760):
        die(f"{path} is {width}x{height}; expected 1200x760")

    counts = {name: 0 for name, *_ in EXPECTED_COLORS}
    # Sample every second pixel in each direction: the fills are large and the
    # text glyphs are thin, but 40 pixels of tolerance leaves plenty of margin.
    for y in range(0, height, 2):
        for x in range(0, width, 2):
            r, g, b = pixel(raw, offset, width, height, row, x, y)
            for name, er, eg, eb, _need in EXPECTED_COLORS:
                if r == er and g == eg and b == eb:
                    counts[name] += 1

    failed = False
    for name, _r, _g, _b, need in EXPECTED_COLORS:
        got = counts[name]
        status = "ok" if got >= need else "MISSING"
        if got < need:
            failed = True
        print(f"  {name:>10}: {got:>7} px (need >= {need}) {status}")
    if failed:
        die(f"{path} is missing one or more theme colours")

    print(f"PASS: {path.name} is a valid {width}x{height} launcher frame")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
