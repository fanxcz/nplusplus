#!/usr/bin/env python3
"""Generate assets/npp-file.ico — the .npp file icon.

Design mirrors the repository brand mark (docs/assets/images/npp-mark.svg):
a dark rounded tile with the yellow "N", extended with a C++-style "++"
suffix. Rendered at 4x supersampling and box-downsampled, then packed as a
multi-size PNG-in-ICO (Windows Vista+).

Pure stdlib; run with any Python 3:

    python3 scripts/make_file_icon.py
"""

from __future__ import annotations

import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT_ICO = ROOT / "assets" / "npp-file.ico"
OUT_PNG = ROOT / "assets" / "npp-icon-256.png"

SS = 4          # supersample factor relative to the 64-unit mark
BASE = 64 * SS  # 256px render resolution

BG = (255, 255, 255, 255)      # white tile, like the C++ logo
BORDER = (208, 210, 214, 255)  # #d0d2d6
FG = (0, 89, 156, 255)         # #00599c — C++ blue

# N letterform from npp-mark.svg (64x64 viewBox), as a polygon.
N_PTS = [
    (15, 47), (15, 17), (22, 17), (42, 38), (42, 17),
    (49, 17), (49, 47), (42, 47), (22, 26), (22, 47),
]


def inside_poly(x: float, y: float, pts: list[tuple[float, float]]) -> bool:
    """Even-odd point-in-polygon test."""
    inside = False
    n = len(pts)
    j = n - 1
    for i in range(n):
        xi, yi = pts[i]
        xj, yj = pts[j]
        if (yi > y) != (yj > y) and x < (xj - xi) * (y - yi) / (yj - yi) + xi:
            inside = not inside
        j = i
    return inside


def rounded_rect(x: float, y: float, w: float, h: float, r: float) -> bool:
    """Point-in-rounded-rect for a rect at (x,y) size (w,h)."""
    if x < 0 or y < 0 or x >= w or y >= h:
        return False
    # clamp to the inner corner centers
    cx = min(max(x, r), w - r)
    cy = min(max(y, r), h - r)
    if (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
        return True
    return r <= x < w - r or r <= y < h - r


def make_plus(cx: float, cy: float, half: float, bar: float):
    def test(x: float, y: float) -> bool:
        return (abs(x - cx) <= bar and abs(y - cy) <= half) or (
            abs(y - cy) <= bar and abs(x - cx) <= half
        )

    return test


def render() -> list[list[tuple[int, int, int, int]]]:
    """Render the 256x256 icon as RGBA rows."""
    px = [[(0, 0, 0, 0) for _ in range(BASE)] for _ in range(BASE)]

    # Geometry in 64-unit mark space, scaled by SS.
    s = 64.0 * SS
    radius = 13 * SS
    border_w = 1.5 * SS

    # N glyph: shrink the brand N and shift it left to leave room for ++.
    n_scale = 0.66
    n_ox, n_oy = 3.0, 13.0
    n_poly = [
        ((px_ * n_scale + n_ox) * SS, (py_ * n_scale + n_oy) * SS)
        for px_, py_ in N_PTS
    ]

    # Two C++-style plus signs to the right of the N (64-unit space).
    plus_half = 5.2
    plus_bar = 1.7
    plus1 = make_plus(41.5, 32.0, plus_half, plus_bar)
    plus2 = make_plus(52.5, 32.0, plus_half, plus_bar)

    for y in range(BASE):
        fy = (y + 0.5) / SS
        for x in range(BASE):
            fx = (x + 0.5) / SS
            col = (0, 0, 0, 0)
            if rounded_rect(fx, fy, 64.0, 64.0, 13.0):
                # border ring vs fill
                inner = rounded_rect(
                    fx - border_w, fy - border_w, 64 - 2 * border_w,
                    64 - 2 * border_w, 13 - border_w,
                )
                col = BG if inner else BORDER
                # N and ++ glyphs sit on top of the tile
                gx = (fx - n_ox) / n_scale
                gy = (fy - n_oy) / n_scale
                if inside_poly(gx, gy, N_PTS) or plus1(fx, fy) or plus2(fx, fy):
                    col = FG
            px[y][x] = col
    return px


def downsample(px: list[list[tuple[int, int, int, int]]], size: int):
    """Box-downsample BASE->size RGBA."""
    f = BASE // size
    out = []
    for y in range(size):
        row = []
        for x in range(size):
            r = g = b = a = 0
            for dy in range(f):
                for dx in range(f):
                    pr, pg, pb, pa = px[y * f + dy][x * f + dx]
                    r += pr; g += pg; b += pb; a += pa
            n = f * f
            row.append((r // n, g // n, b // n, a // n))
        out.append(row)
    return out


def png_bytes(rows: list[list[tuple[int, int, int, int]]], size: int) -> bytes:
    """Encode RGBA rows as a PNG (stdlib only)."""
    raw = bytearray()
    for row in rows:
        raw.append(0)  # filter: none
        for r, g, b, a in row:
            raw += bytes((r, g, b, a))

    def chunk(tag: bytes, data: bytes) -> bytes:
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", ihdr)
        + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + chunk(b"IEND", b"")
    )


def bmp_dib(rows: list[list[tuple[int, int, int, int]]], size: int) -> bytes:
    """Encode RGBA rows as a 32bpp DIB (bottom-up) for classic ICO entries."""
    # BITMAPINFOHEADER: 40 bytes, height doubled for XOR+AND planes.
    header = struct.pack(
        "<IiiHHIIiiII", 40, size, size * 2, 1, 32, 0, 0, 0, 0, 0, 0
    )
    xor = bytearray()
    and_mask = bytearray()
    # DIB rows are bottom-up.
    for y in range(size - 1, -1, -1):
        for x in range(size):
            r, g, b, a = rows[y][x]
            xor += bytes((b, g, r, a))  # BGRA
    # AND mask: 1 bit per pixel, rows padded to 32 bits, bottom-up.
    row_bytes = (size + 31) // 32 * 4
    for y in range(size - 1, -1, -1):
        bits = bytearray(row_bytes)
        for x in range(size):
            if rows[y][x][3] < 128:
                bits[x // 8] |= 0x80 >> (x % 8)
        and_mask += bits
    body = bytes(xor) + bytes(and_mask)
    # biCompression=BI_RGB(0); biSizeImage covers both planes.
    header = struct.pack(
        "<IiiHHIIiiII", 40, size, size * 2, 1, 32, 0, len(body), 0, 0, 0, 0
    )
    return header + body


def ico_bytes(images: list[tuple[int, bytes]]) -> bytes:
    """Pack (size, dib) pairs into a classic BMP-format ICO container."""
    header = struct.pack("<HHH", 0, 1, len(images))
    entries = b""
    offset = 6 + 16 * len(images)
    blobs = b""
    for size, data in images:
        entries += struct.pack(
            "<BBBBHHII", size if size < 256 else 0, size if size < 256 else 0,
            0, 0, 1, 32, len(data), offset,
        )
        blobs += data
        offset += len(data)
    return header + entries + blobs


def main() -> None:
    px = render()
    sizes = [16, 24, 32, 48, 64, 128, 256]
    # Keep PNGs for documentation/Linux pixmaps; the .ico uses classic
    # DIB entries because some shell icon paths reject PNG-compressed icons.
    dibs = []
    for size in sizes:
        small = downsample(px, size)
        dibs.append((size, bmp_dib(small, size)))
    OUT_ICO.write_bytes(ico_bytes(dibs))
    OUT_PNG.write_bytes(png_bytes(downsample(px, 256), 256))
    print(f"wrote {OUT_ICO} ({OUT_ICO.stat().st_size} bytes)")
    print(f"wrote {OUT_PNG}")


if __name__ == "__main__":
    main()
