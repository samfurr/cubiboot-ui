#!/usr/bin/env python3
"""Generate the Settings gear in the same tiled RGB5A3 format as IPL icons.

Run from any directory; the checked-in texture requires no build-time tools.
Uses only Python's standard library. 64px keeps the zoomed cube face crisp.
"""

import math
from pathlib import Path
import struct

SIZE = 64
SAMPLES = 4
OUTPUT = Path(__file__).resolve().parents[1] / "patches/data/settings_tex.bin"


def gear(x, y):
    """Eight teeth, a solid hub, and a transparent circular axle hole."""
    radius = math.hypot(x, y)
    angle = (math.atan2(y, x) + math.pi / 8) % (math.pi / 4) - math.pi / 8
    # A rectangular tooth in each 45-degree sector overlaps the circular hub.
    tooth_x = radius * math.cos(angle)
    tooth_y = radius * math.sin(angle)
    return radius >= 9 and (radius <= 21 or (tooth_x <= 27 and abs(tooth_y) <= 5))


def pixel(px, py):
    totals = [0, 0, 0]
    coverage = 0
    for sy in range(SAMPLES):
        for sx in range(SAMPLES):
            x = px + (sx + 0.5) / SAMPLES - SIZE / 2
            y = py + (sy + 0.5) / SAMPLES - SIZE / 2
            if not gear(x, y):
                continue
            # Cool silver with a small top-left bevel and a shaded lower edge.
            shade = 214 - int(y * 1.1)
            if not gear(x - 1.2, y - 1.2):
                shade = 250
            elif not gear(x + 1.2, y + 1.2):
                shade = 132
            for i, offset in enumerate((-10, -5, 9)):
                totals[i] += max(0, min(255, shade + offset))
            coverage += 1
    if not coverage:
        return 0
    r, g, b = (value // coverage for value in totals)
    if coverage == SAMPLES * SAMPLES:
        return 0x8000 | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3)
    alpha = round(7 * coverage / (SAMPLES * SAMPLES))
    return (alpha << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4)


def main():
    # GX RGB5A3 stores big-endian pixels in row-major 4x4 tiles.
    pixels = [pixel(x, y) for y in range(SIZE) for x in range(SIZE)]
    tiled = [pixels[(by + y) * SIZE + bx + x]
             for by in range(0, SIZE, 4) for bx in range(0, SIZE, 4)
             for y in range(4) for x in range(4)]
    OUTPUT.write_bytes(struct.pack(f">{len(tiled)}H", *tiled))
    print(f"Generated {OUTPUT} ({OUTPUT.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
