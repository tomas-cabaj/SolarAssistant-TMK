# -*- coding: utf-8 -*-
"""Prevede schvalenou PNG ikonku do RGB565 bitmapy pro firmware.
EN: Convert the approved PNG icon into an RGB565 bitmap for the firmware.
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "assets" / "pv-disconnected-source.png"
HEADER = ROOT / "PVDisconnectedIcon.h"
WIDTH, HEIGHT = 40, 38
BACKGROUND = (24, 28, 24)  # C_CARD / dark card background

image = Image.open(SOURCE).convert("RGBA")
alpha = image.getchannel("A")
mask = alpha.point(lambda value: 255 if value >= 200 else 0)
bounds = mask.getbbox()
if not bounds:
    raise SystemExit("Ikona je prazdna / Icon is empty")

icon = image.crop(bounds).resize((WIDTH, HEIGHT), Image.Resampling.LANCZOS)
canvas = Image.new("RGBA", (WIDTH, HEIGHT), BACKGROUND + (255,))
canvas.alpha_composite(icon)
values = []
for red, green, blue in canvas.convert("RGB").get_flattened_data():
    values.append(((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3))

rows = ["  " + ", ".join(f"0x{v:04X}" for v in values[i:i + 12])
        for i in range(0, len(values), 12)]
HEADER.write_text(
    "#pragma once\n\n"
    "// Bitmapa RGB565 zustava ve flash a nezabira dynamickou RAM.\n"
    "// EN: RGB565 bitmap stays in flash; it uses no dynamic RAM.\n"
    f"constexpr int PV_DISCONNECTED_ICON_W = {WIDTH};\n"
    f"constexpr int PV_DISCONNECTED_ICON_H = {HEIGHT};\n"
    f"const uint16_t PV_DISCONNECTED_ICON_DATA[{WIDTH * HEIGHT}] PROGMEM = {{\n"
    + ",\n".join(rows) + "\n};\n",
    encoding="utf-8",
)
print(f"Vytvoreno {HEADER.name}: {WIDTH}x{HEIGHT} px")
