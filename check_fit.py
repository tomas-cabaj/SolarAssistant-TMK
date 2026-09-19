# -*- coding: utf-8 -*-
"""Overi, ze se popisky vejdou do dlazdic a karet ve vsech ctyrech jazycich."""
import io
import os
import re
from PIL import ImageFont

D = os.path.dirname(os.path.abspath(__file__)) + os.sep
NL = chr(10)

font = ImageFont.truetype(D + "DejaVuSans-Bold.ttf", 13)


def w(t):
    return int(font.getlength(t))


# ---- prekladova tabulka -------------------------------------------------
lang = io.open(D + "Lang.h", encoding="utf-8").read()
tr = {}
for line in lang.split(NL):
    if line.startswith('  { "') and "// T_" in line:
        cells = re.findall(r'"((?:[^"\\]|\\.)*)"', line)
        tid = line.split("// ")[1].strip()
        tr[tid] = cells

src = io.open(D + "SolaAssistant-TMK.ino", encoding="utf-8").read()

LANGS = ["CZ", "EN", "PL", "DE"]
problems = []

# ---- statBox / statBoxSmall: popisek se kresli na x+7 -------------------
for m in re.finditer(r"statBox(?:Small)?\(\s*(\d+),\s*([^,]+),\s*(\d+),[^,]+,\s*TR\((T_[A-Z0-9_]+)\)", src):
    boxw = int(m.group(3))
    tid = m.group(4)
    avail = boxw - 12
    for i, name in enumerate(LANGS):
        txt = tr.get(tid, [""] * 4)[i]
        if w(txt) > avail:
            problems.append(("dlazdice %3d px" % boxw, name, tid, txt, w(txt), avail))

# ---- card: texty zacinaji na x+50, karta je siroka 150 -----------------
for m in re.finditer(r"card\(\s*\d+,\s*\d+,\s*(\d+),[^,]+,[^,]+,[^,]+,\s*TR\((T_[A-Z0-9_]+)\)", src):
    boxw = int(m.group(1))
    tid = m.group(2)
    avail = boxw - 56
    for i, name in enumerate(LANGS):
        txt = tr.get(tid, [""] * 4)[i]
        if w(txt) > avail:
            problems.append(("karta %3d px" % boxw, name, tid, txt, w(txt), avail))

# ---- radky v NASTAVENI 2: popisek vlevo, hodnota vpravo ----------------
for tid in ["T_LANGUAGE", "T_REFRESH", "T_SLEEP", "T_BRIGHT", "T_RANGE",
            "T_LED_GREEN", "T_LED_ORANGE", "T_ROTATION"]:
    for i, name in enumerate(LANGS):
        txt = tr.get(tid, [""] * 4)[i]
        if w(txt) > 190:        # zbytek radku patri hodnote
            problems.append(("radek nastaveni", name, tid, txt, w(txt), 190))

# ---- popisky pod pulkruhovymi ukazateli --------------------------------
for m in re.finditer(r"halfGauge\([^;]*?TR\((T_[A-Z0-9_]+)\)\);", src, re.S):
    tid = m.group(1)
    for i, name in enumerate(LANGS):
        txt = tr.get(tid, [""] * 4)[i]
        if w(txt) > 148:
            problems.append(("popisek ukazatele", name, tid, txt, w(txt), 148))

if problems:
    print("PRETEKAJICI TEXTY (%d):" % len(problems))
    for kind, lng, tid, txt, got, avail in problems:
        print("  %-18s %s  %-14s %-28s %3d px / %3d px" % (kind, lng, tid, txt, got, avail))
else:
    print("vsechny popisky se vejdou")
