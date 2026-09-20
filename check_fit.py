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

src = io.open(D + "SolarAssistant-TMK.ino", encoding="utf-8").read()

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

# Payback page: labels plus the largest step, date labels and maximum values.
for i, name in enumerate(LANGS):
    for tid, avail, kind in [
        ("T_RESTART", 138, "restart tlacitko"),
        ("T_PLAN_VS_ACTUAL", 296, "legenda predikce"),
        ("T_TAP_HINT", 308, "napoveda upozorneni"),
        ("T_MAX_LOAD", 112, "porovnani maximum"),
    ]:
        txt = tr[tid][i]
        if w(txt) > avail:
            problems.append((kind, name, tid, txt, w(txt), avail))
    for tid in ["T_DAY", "T_NIGHT"]:
        txt = tr[tid][i]
        if w(txt) > 86:
            problems.append(("pocasi den/noc", name, tid, txt, w(txt), 86))
    for tid in ["T_W_CLEAR", "T_W_PARTLY", "T_W_OVERCAST", "T_W_FOG", "T_W_DRIZZLE",
                "T_W_RAIN", "T_W_SNOW", "T_W_SHOWERS", "T_W_SNOWSH", "T_W_STORM"]:
        txt = tr[tid][i]
        if w(txt) > 236:
            problems.append(("pocasi stav", name, tid, txt, w(txt), 236))

for i, name in enumerate(LANGS):
    for tid in ["T_ROI_INV", "T_ROI_ENERGY"]:
        txt = tr[tid][i] + "  " + tr["T_ROI_STEP"][i] + " 10000"
        if w(txt) > 292:
            problems.append(("navratnost hlavicka", name, tid, txt, w(txt), 292))
    for tid, avail in [("T_ROI_START", 296), ("T_ROI_DATE", 208),
                       ("T_ROI_SAVE_ERR", 308), ("T_ROI_WAIT", 308),
                       ("T_ROI_PAID", 184), ("T_REMAINING", 184)]:
        txt = tr[tid][i]
        if w(txt) > avail:
            problems.append(("navratnost", name, tid, txt, w(txt), avail))
for txt, avail in [("9999999 Kč", 200), ("9999.999 MWh", 200),
                   ("149999985 Kč", 184), ("100 %", 60)]:
    if w(txt) > avail:
        problems.append(("navratnost hodnota", "all", "value", txt, w(txt), avail))

if problems:
    print("PRETEKAJICI TEXTY (%d):" % len(problems))
    for kind, lng, tid, txt, got, avail in problems:
        print("  %-18s %s  %-14s %-28s %3d px / %3d px" % (kind, lng, tid, txt, got, avail))
    raise SystemExit(1)
else:
    print("vsechny popisky se vejdou")
