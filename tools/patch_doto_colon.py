#!/usr/bin/env python3
"""Patch the Doto colon in generated LVGL fonts: Doto draws each colon period
as a plus-shaped 5-dot cluster; Kachel wants one matrix dot per period
(Josch, 2026-07-25). Rewrites the ':' glyph bitmap in place — rerun after
every font regeneration.

Format notes (cost a debugging round): lv_font_conv --no-compress emits the
glyph bitmap as a CONTINUOUS 4bpp bitstream (rows are not byte-aligned) and
prints bytes as minimal hex (0x0, not 0x00) — parse with {1,2}, not {2}.

Usage: python3 tools/patch_doto_colon.py src/font_clock_100.c src/font_timer_44.c
"""
import re
import sys
from pathlib import Path


def patch(path: Path) -> None:
    text = path.read_text()
    dsc = re.findall(
        r"\{\.bitmap_index = (\d+), .*?\.box_w = (\d+), \.box_h = (\d+),", text)
    bi, bw, bh = (int(v) for v in dsc[-1])  # colon = last glyph (0x30-0x3A)

    start = text.index("glyph_bitmap[] = {")
    end = text.index("glyph_dsc[]", start)
    section = text[start:end]
    close = start + section.rindex("};")
    hexes = re.findall(r"0x([0-9a-fA-F]{1,2})\b", section[: section.rindex("};")])
    data = bytearray(int(x, 16) for x in hexes)

    def px(y, x):
        bit = (y * bw + x) * 4
        b = data[bi + bit // 8]
        return (b >> 4) if bit % 8 == 0 else (b & 15)

    # measure the cluster from the top arm (row 0): pitch = blob start,
    # dot = blob width — the center dot of each plus sits at (pitch, pitch)
    row0 = [px(0, x) for x in range(bw)]
    xs = [x for x, v in enumerate(row0) if v > 7]
    if not xs:
        sys.exit(f"{path.name}: colon row 0 empty — already patched or unexpected glyph")
    pitch, dot = xs[0], xs[-1] - xs[0] + 1
    if bw != 2 * pitch + dot:
        sys.exit(f"{path.name}: geometry mismatch (bw {bw}, pitch {pitch}, dot {dot})")

    # rebuild: zeros + one dot per period at the cluster centers
    nbits = bw * bh * 4
    new = bytearray((nbits + 7) // 8)
    def set_px(y, x):
        bit = (y * bw + x) * 4
        if bit % 8 == 0:
            new[bit // 8] |= 0xF0
        else:
            new[bit // 8] |= 0x0F
    for y0 in (pitch, bh - pitch - dot):
        for y in range(y0, y0 + dot):
            for x in range(pitch, pitch + dot):
                set_px(y, x)

    body_bytes = ", ".join(f"0x{b:x}" for b in new)
    lines, line = [], []
    for tok in body_bytes.split(", "):
        line.append(tok)
        if len(line) == 16:
            lines.append(", ".join(line)); line = []
    if line:
        lines.append(", ".join(line))
    body = ",\n    ".join(lines)

    marker = text.rindex('/* U+003A ":" */')
    patched = text[:marker] + f'/* U+003A ":" */\n    {body}\n' + text[close:]
    path.write_text(patched)
    print(f"{path.name}: colon -> two {dot}x{dot} dots at pitch {pitch} "
          f"(box {bw}x{bh}, {len(new)} bytes)")


if __name__ == "__main__":
    for arg in sys.argv[1:]:
        patch(Path(arg))
