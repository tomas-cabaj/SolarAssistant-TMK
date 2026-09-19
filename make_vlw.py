# -*- coding: utf-8 -*-
"""
Generator smooth fontu (.vlw) pro TFT_eSPI, ulozeny jako C pole do .h souboru.

Pouziti:   python make_vlw.py 14 FontUi FontUi.h [glyphs.txt]

Sada znaku = ASCII 0x20-0x7E + vsechny znaky nad ASCII ze souboru glyphs.txt
(ten vyrabi mklang.py z prekladove tabulky) + stupen a druha mocnina.

Format VLW (vsechna cisla int32 big-endian):
  hlavicka 6 x int32:  pocet glyfu, verze(11), velikost, 0, ascent, descent
  pak pro kazdy glyf 7 x int32:
      unicode, vyska, sirka, xAdvance, dY (nad uctarou), dX (leva odsazeni), 0
  pak bitmapy glyfu za sebou, 1 bajt na pixel (alfa 0-255), vyska*sirka bajtu
"""
import struct, sys, io, os
from PIL import Image, ImageDraw, ImageFont

# DejaVu Sans Bold - volna licence (Bitstream Vera / DejaVu),
# TTF je soucasti projektu, viz LICENSE_DEJAVU.txt
FONT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "DejaVuSans-Bold.ttf")
SIZE      = int(sys.argv[1]) if len(sys.argv) > 1 else 13
OUT_NAME  = sys.argv[2] if len(sys.argv) > 2 else "FontUi"
OUT_PATH  = sys.argv[3] if len(sys.argv) > 3 else "FontUi.h"
GLYPHS    = sys.argv[4] if len(sys.argv) > 4 else "glyphs.txt"

# ---- sada znaku -----------------------------------------------------------
chars = [chr(c) for c in range(0x20, 0x7F)]

try:
    extra = io.open(GLYPHS, encoding="utf-8").read()
except OSError:
    extra = ""
    print("POZOR: %s nenalezen, generuji jen ASCII" % GLYPHS)

for ch in extra:
    if ch.strip() and ch not in chars:
        chars.append(ch)

for ch in [chr(0x00B0), chr(0x00B2)]:        # stupen, druha mocnina
    if ch not in chars:
        chars.append(ch)

font = ImageFont.truetype(FONT_PATH, SIZE)
ascent, descent = font.getmetrics()

glyphs = []
missing = []
for ch in chars:
    adv = int(round(font.getlength(ch)))
    bb = font.getbbox(ch)

    if bb is None or bb[2] - bb[0] <= 0 or bb[3] - bb[1] <= 0:
        # mezera a podobne - prazdna bitmapa 1x1, jen posun kurzoru
        glyphs.append({"cp": ord(ch), "w": 1, "h": 1, "adv": adv,
                       "dY": 1, "dX": 0, "bmp": bytes([0])})
        continue

    x0, y0, x1, y1 = bb
    w, h = x1 - x0, y1 - y0

    img = Image.new("L", (w, h), 0)
    ImageDraw.Draw(img).text((-x0, -y0), ch, font=font, fill=255)

    if img.getbbox() is None and ord(ch) > 0x20:
        missing.append(ch)

    glyphs.append({"cp": ord(ch), "w": w, "h": h, "adv": adv,
                   "dY": ascent - y0, "dX": x0, "bmp": img.tobytes()})

maxAsc = max(g["dY"] for g in glyphs)
maxDes = max(g["h"] - g["dY"] for g in glyphs)

# ---- slozeni souboru ------------------------------------------------------
out = io.BytesIO()
out.write(struct.pack(">6i", len(glyphs), 11, SIZE, 0, maxAsc, maxDes))
for g in glyphs:
    out.write(struct.pack(">7i", g["cp"], g["h"], g["w"], g["adv"], g["dY"], g["dX"], 0))
for g in glyphs:
    out.write(g["bmp"])

data = out.getvalue()

# ---- zapis jako C hlavicka ------------------------------------------------
lines = []
lines.append("// Vygenerovano skriptem make_vlw.py - needitovat rucne.")
lines.append("// Pismo: DejaVu Sans Bold (volna licence), velikost %d px, %d glyfu." % (SIZE, len(glyphs)))
lines.append("// Pokryva ASCII + cestinu, polstinu a nemcinu.")
lines.append("// Format VLW pro TFT_eSPI, nacita se pres tft.loadFont(%s)." % OUT_NAME)
lines.append("")
lines.append("#pragma once")
lines.append("#include <pgmspace.h>")
lines.append("")
lines.append("const uint8_t %s[] PROGMEM = {" % OUT_NAME)
for i in range(0, len(data), 16):
    lines.append("  " + ",".join("0x%02X" % b for b in data[i:i + 16]) + ",")
lines.append("};")
lines.append("")

with io.open(OUT_PATH, "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(lines))

print("pismo      : %s %d px" % (os.path.basename(FONT_PATH), SIZE))
print("glyfu      : %d  (z toho %d nad ASCII)" % (len(glyphs), len(glyphs) - 95))
print("ascent     : %d px   descent: %d px   vyska radku: %d px"
      % (maxAsc, maxDes, maxAsc + maxDes))
print("velikost   : %d bajtu (%.1f kB)" % (len(data), len(data) / 1024.0))
if missing:
    print("CHYBEJICI GLYFY: %s" % "".join(missing))
else:
    print("vsechny znaky maji glyf")
print("zapsano    : %s" % OUT_PATH)
