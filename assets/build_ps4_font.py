#!/usr/bin/env python3
"""Build a compact anti-aliased DejaVu Sans atlas for the PS4 dashboard."""
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

font_path = Path(sys.argv[1])
output = Path(sys.argv[2])
chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 "
font_size = 48
cell_w, cell_h = 48, 56
cols = 8
rows = (len(chars) + cols - 1) // cols
atlas = Image.new("RGBA", (cols * cell_w, rows * cell_h), (255, 255, 255, 0))
draw = ImageDraw.Draw(atlas)
font = ImageFont.truetype(str(font_path), font_size)
advances = []
for index, ch in enumerate(chars):
    col, row = index % cols, index // cols
    x0, y0, x1, y1 = draw.textbbox((0, 0), ch, font=font)
    draw.text((col * cell_w + 2 - x0, row * cell_h + 2 - y0),
              ch, font=font, fill=(255, 255, 255, 255))
    advances.append(round(font.getlength(ch) * 100))

def c_array(name, c_type, values, per_line):
    out = ["static const %s %s[%d] = {" % (c_type, name, len(values))]
    for start in range(0, len(values), per_line):
        out.append(" " + ",".join(str(v) for v in values[start:start + per_line]) + ",")
    out.append("};")
    return "\\n".join(out)

lines = [
    "/* DejaVu Sans Bold glyph atlas; generated at build time. */
    "#ifndef SAVEGUARD_UI_FONT_GENERATED_H",
    "#define SAVEGUARD_UI_FONT_GENERATED_H",
    "#define SG_FONT_ATLAS_WIDTH %d" % atlas.width,
    "#define SG_FONT_ATLAS_HEIGHT %d" % atlas.height,
    "#define SG_FONT_CELL_WIDTH %d" % cell_w,
    "#define SG_FONT_CELL_HEIGHT %d" % cell_h,
    "#define SG_FONT_ATLAS_COLS %d" % cols,
    "#define SG_FONT_GLYPH_COUNT %d" % len(chars),
    c_array("sg_font_advances", "unsigned short", advances, 12),
    c_array("sg_font_rgba", "unsigned char", atlas.tobytes(), 24),
    "#endif",
]
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text("\\n".join(lines) + "\\n", encoding="utf-8")
