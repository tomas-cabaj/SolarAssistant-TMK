# -*- coding: utf-8 -*-
"""
Vykresli nahledy obrazovek 1:1 (320x480) s fiktivnimi daty.
Napodobuje primitiva TFT_eSPI, pouziva stejny font jako firmware.
"""
import io, os, math
from PIL import Image, ImageDraw, ImageFont

D = os.path.dirname(os.path.abspath(__file__)) + os.sep
OUT = D + "images" + os.sep
os.makedirs(OUT, exist_ok=True)

W, H = 320, 480

# ---- barvy prevedene z RGB565 -------------------------------------------
def c565(v):
    r = (v >> 11) & 0x1F
    g = (v >> 5) & 0x3F
    b = v & 0x1F
    return (r * 255 // 31, g * 255 // 63, b * 255 // 31)

C_BG    = c565(0x0000)
C_CARD  = c565(0x18E3)
C_LINE  = c565(0x39E7)
C_DIM   = c565(0x8410)
C_TXT   = (255, 255, 255)
C_PV    = c565(0xFD20)
C_LOAD  = c565(0x2D7F)
C_BATT  = c565(0x2666)
C_GRID  = c565(0xF800)
C_WEATH = c565(0x07FF)
C_TRACK = c565(0x2124)
C_STALE = c565(0x6000)
LOGO_DARK = c565(0x21AB)

FP = D + "DejaVuSans-Bold.ttf"
F13 = ImageFont.truetype(FP, 13)          # cesky smooth font
F2  = ImageFont.truetype(FP, 12)          # vestavene pismo 2
F4  = ImageFont.truetype(FP, 20)          # vestavene pismo 4
F6  = ImageFont.truetype(FP, 34)          # vestavene pismo 6
F7  = ImageFont.truetype(FP, 36)          # sedmisegmentove pismo 7

ANCH = {"TL": "lt", "TR": "rt", "MC": "mm", "MR": "rm"}

# Nahled pouziva stejne zmensene bitmapy jako firmware.
# EN: The preview uses the same downscaled bitmaps as the firmware.
_ICON_SHEET = Image.open(OUT + "ui-icons-38.png").convert("RGB")
UI_ICONS = tuple(_ICON_SHEET.crop((x * 52 + 7, y * 52 + 7, x * 52 + 45, y * 52 + 45))
                 for y in range(4) for x in range(4))
_NAV_SHEET = Image.open(OUT + "ui-nav-30.png").convert("RGB")
UI_NAV_ICONS = tuple(_NAV_SHEET.crop((x * 38 + 4, 4, x * 38 + 34, 34)) for x in range(3))

UI_ICON_INVERTER, UI_ICON_SOLAR, UI_ICON_GRID, UI_ICON_BATTERY = range(4)
UI_ICON_HOUSE = 4


class Screen:
    def __init__(self):
        self.im = Image.new("RGB", (W, H), C_BG)
        self.d = ImageDraw.Draw(self.im)

    # ---- primitiva --------------------------------------------------
    def rect(self, x, y, w, h, col, fill=True):
        if fill: self.d.rectangle([x, y, x + w - 1, y + h - 1], fill=col)
        else:    self.d.rectangle([x, y, x + w - 1, y + h - 1], outline=col)

    def rrect(self, x, y, w, h, r, col, fill=True):
        if fill: self.d.rounded_rectangle([x, y, x + w - 1, y + h - 1], r, fill=col)
        else:    self.d.rounded_rectangle([x, y, x + w - 1, y + h - 1], r, outline=col)

    def line(self, x0, y0, x1, y1, col):
        self.d.line([x0, y0, x1, y1], fill=col)

    def circle(self, cx, cy, r, col, fill=True):
        box = [cx - r, cy - r, cx + r, cy + r]
        if fill: self.d.ellipse(box, fill=col)
        else:    self.d.ellipse(box, outline=col)

    def tri(self, x0, y0, x1, y1, x2, y2, col):
        self.d.polygon([(x0, y0), (x1, y1), (x2, y2)], fill=col)

    def txt(self, s, x, y, font=F13, col=C_TXT, datum="TL"):
        self.d.text((x, y), s, font=font, fill=col, anchor=ANCH[datum])

    # ---- slozene prvky ----------------------------------------------
    def panel(self, x, y, w, h, border=C_LINE):
        self.rrect(x, y, w, h, 8, C_CARD)
        self.rrect(x, y, w, h, 8, border, fill=False)

    def arc(self, cx, cy, rin, rout, a0, a1, col):
        a = a0
        while a <= a1:
            r = math.radians(a)
            c, s = math.cos(r), math.sin(r)
            self.line(cx + c * rin, cy + s * rin, cx + c * rout, cy + s * rout, col)
            a += 0.15

    def ticks(self, cx, cy, r):
        for i in range(21):
            a = math.radians(180 + 180 * i / 20)
            c, s = math.cos(a), math.sin(a)
            big = (i % 5 == 0)
            r1, r2 = r + 2, r + (7 if big else 4)
            self.line(cx + c * r1, cy + s * r1, cx + c * r2, cy + s * r2,
                      C_DIM if big else C_TRACK)

    def gauge(self, cx, cy, r, th, val, mx, col, vtext, label):
        f = max(0.0, min(1.0, val / mx if mx else 0))
        self.arc(cx, cy, r - th, r, 180, 360, C_TRACK)
        if f > 0.004:
            self.arc(cx, cy, r - th, r, 180, 180 + 180 * f, col)
        self.ticks(cx, cy, r)
        self.txt(vtext, cx, cy - 16, F4, C_TXT, "MC")
        self.txt(label, cx, cy + 12, F13, C_DIM, "MC")

    def bar(self, x, y, w, h, frac, col):
        frac = max(0.0, min(1.0, frac))
        self.rrect(x, y, w, h, h // 2, C_TRACK)
        fw = int(w * frac)
        if fw <= 0: return
        if fw < h: fw = h
        self.rrect(x, y, fw, h, h // 2, col)

    def card(self, x, y, w, h, icon, icol, title, l1, l2):
        self.panel(x, y, w, h)
        self.im.paste(UI_ICONS[icon], (x + 6, y + 6))
        self.txt(title, x + 50, y + 6, F13, C_TXT)
        self.txt(l1,    x + 50, y + 23, F13, icol)
        self.txt(l2,    x + 50, y + 39, F13, C_DIM)

    def tile(self, x, y, w, h, label, value, col, small=False):
        self.panel(x, y, w, h)
        # Nadpis je sedy a vlevo, hodnota zustava vycentrovana.
        # EN: The label is grey and left-aligned; the value stays centred.
        self.txt(label, x + 8, y + 6, F13, C_DIM)
        if small: self.txt(value, x + w // 2, y + h - 14, F13, col, "MC")
        else:     self.txt(value, x + w // 2, y + h - 15, F4, col, "MC")


# ---- ikony ---------------------------------------------------------------
def weather_icon(s, x, y, code, night=False):
    kind = (0 if code == 0 else 1 if code <= 2 else 2 if code == 3 else
            3 if code <= 48 else 4 if code <= 57 else 5 if code <= 67 else
            6 if code <= 77 else 7 if code <= 82 else 8 if code <= 86 else 9)
    if kind in (0, 1, 7):
        if night:
            s.circle(x + 20, y + 19, 13, C_WEATH)
            s.circle(x + 26, y + 14, 12, C_CARD)
            s.line(x + 42, y + 10, x + 48, y + 10, C_TXT)
            s.line(x + 45, y + 7, x + 45, y + 13, C_TXT)
        else:
            s.circle(x + 23, y + 23, 11, C_PV)
            for a, b, c, d in [(0,-15,0,-20),(11,-11,15,-15),(15,0,20,0),
                                (11,11,15,15),(0,15,0,20),(-11,11,-15,15),
                                (-15,0,-20,0),(-11,-11,-15,-15)]:
                s.line(x+23+a, y+23+b, x+23+c, y+23+d, C_PV)
    if kind == 0: return
    colour = C_DIM if kind == 9 else C_TXT
    s.circle(x+18, y+33, 11, colour); s.circle(x+32, y+26, 15, colour)
    s.circle(x+46, y+34, 10, colour); s.rrect(x+14, y+33, 37, 12, 5, colour)
    if kind == 3:
        for row in range(3): s.line(x+8+row*3, y+49+row*5, x+51-row*2, y+49+row*5, C_DIM)
    elif kind in (6, 8):
        for i in range(3):
            cx, cy = x+16+i*15, y+53+(i%2)*4
            s.line(cx-3,cy,cx+3,cy,C_WEATH); s.line(cx,cy-3,cx,cy+3,C_WEATH)
            s.line(cx-2,cy-2,cx+2,cy+2,C_WEATH)
    elif kind == 9:
        s.tri(x+32,y+43,x+21,y+55,x+33,y+53,C_PV)
        s.tri(x+30,y+50,x+40,y+48,x+25,y+63,C_PV)
    elif kind in (4, 5, 7):
        length = 3 if kind == 4 else 8
        for i in range(3):
            dx = x+18+i*14
            s.line(dx,y+49,dx-3,y+49+length,C_WEATH)
            s.line(dx+1,y+49,dx-2,y+49+length,C_WEATH)

def ico_inverter(s, x, y, c):
    s.rrect(x + 4, y + 3, 22, 26, 4, C_TXT, fill=False)
    s.rrect(x + 8, y + 7, 14, 9, 2, C_BG)
    s.line(x + 10, y + 12, x + 12, y + 10, c); s.line(x + 12, y + 10, x + 15, y + 14, c); s.line(x + 15, y + 14, x + 19, y + 10, c)
    s.circle(x + 11, y + 22, 2, C_TXT); s.circle(x + 19, y + 22, 2, C_TXT)

def ico_panel(s, x, y, c):
    s.circle(x + 23, y + 6, 4, C_PV)
    s.rect(x + 2, y + 8, 25, 15, c, fill=False)
    s.line(x + 10, y + 9, x + 10, y + 22, c); s.line(x + 18, y + 9, x + 18, y + 22, c)
    s.line(x + 3, y + 15, x + 26, y + 15, c); s.line(x + 15, y + 23, x + 15, y + 28, C_TXT); s.line(x + 8, y + 28, x + 22, y + 28, C_TXT)

def ico_pylon(s, x, y, c):
    s.line(x + 5, y + 28, x + 13, y + 5, c)
    s.line(x + 25, y + 28, x + 17, y + 5, c)
    s.line(x + 13, y + 5, x + 17, y + 5, c)
    s.line(x + 4, y + 10, x + 25, y + 10, c)
    s.line(x + 2, y + 16, x + 27, y + 16, c)
    s.line(x + 9, y + 18, x + 21, y + 18, c)
    s.line(x + 10, y + 23, x + 20, y + 23, c)
    s.line(x + 11, y + 10, x + 19, y + 16, c)
    s.line(x + 19, y + 10, x + 11, y + 16, c)

def ico_battery(s, x, y, c):
    s.rrect(x + 5, y + 4, 20, 24, 4, C_TXT, fill=False); s.rect(x + 11, y + 1, 8, 3, C_TXT)
    s.rrect(x + 9, y + 8, 12, 16, 2, c)
    s.tri(x + 17, y + 9, x + 11, y + 17, x + 15, y + 17, C_TXT); s.tri(x + 13, y + 23, x + 19, y + 15, x + 15, y + 15, C_TXT)

def ico_house(s, x, y, c):
    s.tri(x + 2, y + 15, x + 15, y + 3, x + 28, y + 15, C_TXT)
    s.rect(x + 6, y + 14, 19, 14, C_TXT)
    s.rect(x + 9, y + 18, 5, 5, c); s.rect(x + 17, y + 18, 5, 10, c)

def ico_bolt(s, x, y, c):
    s.tri(x + 18, y + 3, x + 8, y + 17, x + 15, y + 17, c)
    s.tri(x + 12, y + 28, x + 22, y + 13, x + 15, y + 13, c)

def ico_clock(s, x, y, c):
    s.circle(x + 15, y + 15, 12, c, fill=False)
    s.line(x + 15, y + 15, x + 15, y + 8, c)
    s.line(x + 15, y + 15, x + 20, y + 17, c)

def ico_sun(s, x, y, c):
    s.circle(x + 15, y + 15, 7, c)
    for a in range(0, 360, 45):
        r = math.radians(a)
        s.line(x + 15 + math.cos(r) * 10, y + 15 + math.sin(r) * 10,
               x + 15 + math.cos(r) * 14, y + 15 + math.sin(r) * 14, c)

def ico_cloud(s, x, y, c):
    s.circle(x + 10, y + 18, 6, c)
    s.circle(x + 18, y + 15, 8, c)
    s.circle(x + 24, y + 19, 5, c)
    s.rect(x + 10, y + 19, 15, 5, c)
