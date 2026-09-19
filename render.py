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
        self.rrect(x, y, w, h, 8, C_CARD)
        self.rrect(x, y, w, h, 8, icol, fill=False)
        self.rrect(x + 6, y + 6, 38, 38, 6, C_BG)
        icon(self, x + 10, y + 10, icol)
        self.txt(title, x + 50, y + 6, F13, C_TXT)
        self.txt(l1,    x + 50, y + 23, F13, icol)
        self.txt(l2,    x + 50, y + 39, F13, C_DIM)

    def tile(self, x, y, w, h, label, value, col, small=False):
        self.rrect(x, y, w, h, 6, C_CARD)
        self.rrect(x, y, w, h, 6, col, fill=False)
        self.txt(label, x + 7, y + 4, F13, C_DIM)
        if small: self.txt(value, x + 7, y + 24, F13, col)
        else:     self.txt(value, x + 7, y + 20, F4, col)


# ---- ikony ---------------------------------------------------------------
def ico_inverter(s, x, y, c):
    s.rrect(x + 5, y + 3, 20, 26, 3, c, fill=False)
    s.rect(x + 9, y + 8, 12, 7, c)
    s.line(x + 9, y + 20, x + 21, y + 20, c)
    s.line(x + 9, y + 24, x + 21, y + 24, c)

def ico_panel(s, x, y, c):
    s.rect(x + 2, y + 5, 26, 16, c, fill=False)
    s.line(x + 10, y + 5, x + 10, y + 20, c)
    s.line(x + 19, y + 5, x + 19, y + 20, c)
    s.line(x + 2, y + 13, x + 27, y + 13, c)
    s.line(x + 15, y + 21, x + 15, y + 27, c)
    s.line(x + 8, y + 27, x + 22, y + 27, c)

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
    s.rrect(x + 4, y + 4, 18, 24, 3, c, fill=False)
    s.rect(x + 10, y + 1, 6, 3, c)
    s.tri(x + 15, y + 8, x + 9, y + 18, x + 13, y + 18, c)
    s.tri(x + 11, y + 24, x + 17, y + 14, x + 13, y + 14, c)

def ico_house(s, x, y, c):
    s.tri(x + 4, y + 15, x + 15, y + 4, x + 26, y + 15, c)
    s.rect(x + 8, y + 15, 15, 13, c, fill=False)
    s.rect(x + 13, y + 20, 5, 8, c)

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
