#!/usr/bin/env python3
"""Rasterize the licensed PlayStation controller glyphs into a small C header."""
import sys
import subprocess
from pathlib import Path
from PIL import Image

source = Path(sys.argv[1])
output = Path(sys.argv[2])
size = 44
buttons = ("ps_x", "ps_circle", "ps_square", "ps_triangle")

def export_button(button, target):
    subprocess.run([
        "inkscape", str(source),
        "--export-id=" + button,
        "--export-id-only",
        "--export-width=" + str(size),
        "--export-height=" + str(size),
        "--export-filename=" + str(target),
    ], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)

def c_bytes(name, pixels):
    values = ",".join("0x%02x" % value for value in pixels)
    return "static const unsigned char %s[%d] = {%s};\n" % (name, len(pixels), values)

lines = [
    "/* Derived from RobTheFiveNine/flat-gamepad-icons; CC BY-SA 4.0. */",
    "#ifndef SAVEGUARD_BUTTON_ICONS_GENERATED_H",
    "#define SAVEGUARD_BUTTON_ICONS_GENERATED_H",
    "#define SG_BUTTON_ICON_WIDTH %d" % size,
    "#define SG_BUTTON_ICON_HEIGHT %d" % size,
]
for button in buttons:
    target = output.with_name(button + ".png")
    export_button(button, target)
    image = Image.open(target).convert("RGBA")
    if image.size != (size, size):
        image = image.resize((size, size), Image.Resampling.LANCZOS)
    lines.append(c_bytes(button + "_pixels", image.tobytes()))
    target.unlink()
lines.append("#endif")
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text("\n".join(lines) + "\n", encoding="utf-8")
