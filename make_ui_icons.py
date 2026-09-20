# -*- coding: utf-8 -*-
"""Vytvori LCD bitmapy z puvodni sady ikon. / Builds LCD bitmaps from the original icon sheet."""

from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "assets" / "ui-icons-source.png"
HEADER = ROOT / "UiIcons.h"
PREVIEW = ROOT / "images" / "ui-icons-38.png"
NAV_PREVIEW = ROOT / "images" / "ui-nav-30.png"
SIZE = 38
NAV_SIZE = 30

# Pevne vyrezane bunky overene proti zdrojovemu obrazku 1254 x 1254.
# EN: Fixed cell crops verified against the 1254 x 1254 source image.
X_RANGES = ((39, 318), (338, 617), (637, 917), (937, 1216))
Y_RANGES = ((38, 317), (336, 614), (632, 907), (925, 1202))
NAMES = (
    "INVERTER", "SOLAR", "GRID", "BATTERY",
    "HOUSE", "TEMPERATURE", "WEATHER", "CHART",
    "HISTORY", "SAVINGS", "FORECAST", "SETTINGS",
    "OFFLINE", "ALERT", "COMPARE", "PAYBACK",
)


def source_icons():
    image = Image.open(SOURCE).convert("RGBA")
    if image.size != (1254, 1254):
        raise SystemExit(f"Neocekavana velikost zdroje / Unexpected source size: {image.size}")
    result = []
    for y0, y1 in Y_RANGES:
        for x0, x1 in X_RANGES:
            tile = image.crop((x0, y0, x1, y1))
            tile.thumbnail((SIZE, SIZE), Image.Resampling.LANCZOS)
            canvas = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 255))
            canvas.alpha_composite(tile, ((SIZE - tile.width) // 2, (SIZE - tile.height) // 2))
            result.append(canvas.convert("RGB"))
    return result


def nav_arrow(right=False):
    """Antialiasovana sipka ve stylu sady. / Antialiased arrow matching the icon set."""
    scale = 6
    image = Image.new("RGB", (NAV_SIZE * scale, NAV_SIZE * scale), (24, 28, 24))
    draw = ImageDraw.Draw(image)
    points = [(21, 5), (10, 15), (21, 25)]
    if right:
        points = [(NAV_SIZE - 1 - x, y) for x, y in points]
    shadow = [(x * scale + scale, y * scale + scale) for x, y in points]
    line = [(x * scale, y * scale) for x, y in points]
    draw.line(shadow, fill=(5, 7, 7), width=7 * scale, joint="curve")
    draw.line(line, fill=(246, 248, 250), width=6 * scale, joint="curve")
    for x, y in line:
        draw.ellipse((x - 3 * scale, y - 3 * scale, x + 3 * scale, y + 3 * scale), fill=(246, 248, 250))
    return image.resize((NAV_SIZE, NAV_SIZE), Image.Resampling.LANCZOS)


def rgb565(image):
    values = []
    for r, g, b in image.get_flattened_data():
        values.append(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
    return values


def array(name, image, pixels="UI_ICON_PIXELS"):
    values = rgb565(image)
    rows = []
    for offset in range(0, len(values), 12):
        rows.append("  " + ", ".join(f"0x{value:04X}" for value in values[offset:offset + 12]))
    return f"const uint16_t {name}[{pixels}] PROGMEM = {{\n" + ",\n".join(rows) + "\n};\n"


def main():
    icons = source_icons()
    HEADER.write_text(
        "#pragma once\n\n"
        "// Bitmapy jsou v RGB565 a zustavaji ve flash, nezabiraji dynamickou RAM.\n"
        "// EN: RGB565 bitmaps stay in flash and use no dynamic RAM.\n"
        "constexpr int UI_ICON_SIZE = 38;\n"
        "constexpr int UI_ICON_PIXELS = UI_ICON_SIZE * UI_ICON_SIZE;\n"
        "constexpr int UI_NAV_ICON_SIZE = 30;\n"
        "constexpr int UI_NAV_ICON_PIXELS = UI_NAV_ICON_SIZE * UI_NAV_ICON_SIZE;\n"
        "enum UiIconId : uint8_t {\n  UI_ICON_" + ",\n  UI_ICON_".join(NAMES) + ",\n  UI_ICON_COUNT\n};\n"
        "static_assert(UI_ICON_COUNT == 16, \"Sada musi obsahovat 16 ikon\");\n\n"
        + "\n".join(array("UI_ICON_" + name + "_DATA", icon) for name, icon in zip(NAMES, icons))
        + "\n" + array("UI_NAV_LEFT_DATA", nav_arrow(False), "UI_NAV_ICON_PIXELS")
        + "\n" + array("UI_NAV_HOME_DATA", icons[4].resize((NAV_SIZE, NAV_SIZE), Image.Resampling.LANCZOS), "UI_NAV_ICON_PIXELS")
        + "\n" + array("UI_NAV_RIGHT_DATA", nav_arrow(True), "UI_NAV_ICON_PIXELS")
        + "\nconst uint16_t* const UI_ICON_DATA[UI_ICON_COUNT] PROGMEM = {\n  "
        + ",\n  ".join("UI_ICON_" + name + "_DATA" for name in NAMES)
        + "\n};\n",
        encoding="utf-8",
        newline="\n",
    )

    PREVIEW.parent.mkdir(exist_ok=True)
    preview = Image.new("RGB", (4 * 52, 4 * 52), (0, 0, 0))
    for index, icon in enumerate(icons):
        preview.paste(icon, (index % 4 * 52 + 7, index // 4 * 52 + 7))
    preview.save(PREVIEW)

    nav_icons = (nav_arrow(False), icons[4].resize((NAV_SIZE, NAV_SIZE), Image.Resampling.LANCZOS), nav_arrow(True))
    nav_preview = Image.new("RGB", (3 * 38, 38), (0, 0, 0))
    for index, icon in enumerate(nav_icons):
        nav_preview.paste(icon, (index * 38 + 4, 4))
    nav_preview.save(NAV_PREVIEW)
    print(f"vytvoreno {HEADER.name}: {len(icons)} ikon, {SIZE} x {SIZE} px")


if __name__ == "__main__":
    main()
