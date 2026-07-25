#!/usr/bin/env python3
"""Patch the Doto colon in generated LVGL fonts: the family draws each colon
period as a plus-shaped 5-dot cluster; Kachel wants one matrix dot per period
(Josch, 2026-07-25). Rewrites the ':' glyph bitmap in place — rerun after any
font regeneration.

Usage: python3 tools/patch_doto_colon.py src/font_clock_100.c src/font_timer_44.c
"""
import math
import re
import sys
from pathlib import Path


def make_dot_bitmap(box_w: int, box_h: int, dot_d: float) -> list[list[int]]:
    """4bpp rows: two rounded-square dots (ROND 25) at 11% / 89% box height."""
    rows = [[0] * box_w for _ in range(box_h)]
    centers = [(box_w / 2, box_h * 0.11), (box_w / 2, box_h * 0.89)]
    half = dot_d / 2
    corner = dot_d * 0.25  # ROND 25 corner radius
    for cx, cy in centers:
        y0, y1 = int(max(0, cy - half - 1)), int(min(box_h, cy + half + 2))
        for y in range(y0, y1):
            for x in range(box_w):
                # signed distance to rounded square, 2x2 supersample
                cov = 0.0
                for sy in (0.25, 0.75):
                    for sx in (0.25, 0.75):
                        dx = abs(x + sx - cx) - (half - corner)
                        dy = abs(y + sy - cy) - (half - corner)
                        dx, dy = max(dx, 0.0), max(dy, 0.0)
                        d = math.hypot(dx, dy) - corner
                        cov += max(0.0, min(1.0, 0.5 - d))
                v = int(round(cov / 4 * 15))
                if v > rows[y][x]:
                    rows[y][x] = v
    return rows


def patch(path: Path) -> None:
    text = path.read_text()

    # colon is the last glyph (range 0x30-0x3A): last non-zero dsc entry
    dsc = re.findall(
        r"\{\.bitmap_index = (\d+), \.adv_w = \d+, \.box_w = (\d+), \.box_h = (\d+), "
        r"\.ofs_x = (-?\d+), \.ofs_y = (-?\d+)\}",
        text,
    )
    bitmap_index, box_w, box_h = (int(v) for v in dsc[-1][:3])

    # digit dot diameter: Doto dot = 146.5/1000 em; em = line height source —
    # infer from box_h (colon spans 646/1000 em)
    em = box_h * 1000 / 646
    dot_d = 146.5 / 1000 * em

    rows = make_dot_bitmap(box_w, box_h, dot_d)
    row_bytes = []
    for r in rows:
        packed = []
        for i in range(0, box_w, 2):
            hi = r[i]
            lo = r[i + 1] if i + 1 < box_w else 0
            packed.append(f"0x{(hi << 4) | lo:02x}")
        row_bytes.append(", ".join(packed))
    body = ",\n    ".join(row_bytes)

    # splice: replace everything between the colon's comment marker and the
    # closing of glyph_bitmap[]
    marker = text.rindex("/* U+003A \":\" */")
    end = text.index("};", marker)
    new = (
        f'/* U+003A ":" */\n    {body}\n'
    )
    path.write_text(text[:marker] + new + text[end:])
    print(f"{path.name}: colon -> two single dots "
          f"(box {box_w}x{box_h}, dot {dot_d:.1f}px, {len(rows)} rows)")


if __name__ == "__main__":
    for arg in sys.argv[1:]:
        patch(Path(arg))
