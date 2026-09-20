# -*- coding: utf-8 -*-
"""
Rychla kontrola skece po automatickem patchi.
EN: Quick check of the sketch after an automated patch.

Skec ma pres 3000 radku a neprekladame ji pri kazde zmene. Tohle odhali
rozbitou strukturu driv, nez se k tomu dostane kompilator: nevyvazene
zavorky, chybejici preklady, rekurzivni volani wrapperu.

EN: The sketch is over 3000 lines and is not compiled after every change.
    This catches a broken structure before the compiler does: unbalanced
    braces, missing translations, a wrapper calling itself.

Pouziti / Usage:   python check_braces.py
"""
import io
import os
import re

NL = chr(10)
DQ = chr(34)
SQ = chr(39)
BS = chr(92)

D = os.path.dirname(os.path.abspath(__file__)) + os.sep
src = io.open(D + "SolarAssistant-TMK.ino", encoding="utf-8").read()
for header in ("UiStyle.h", "WeatherIcons.h", "UiIcons.h"):
    src += NL + io.open(D + header, encoding="utf-8").read()
lang = io.open(D + "Lang.h", encoding="utf-8").read()

problems = []

# ---- bilance zavorek mimo komentare a retezce ---------------------------
# EN: brace balance outside comments and string literals
depth = 0
line = 1
state = 0          # 0 kod, 1 radkovy komentar, 2 blokovy, 3 retezec, 4 znak
i, n = 0, len(src)
while i < n:
    c = src[i]
    if c == NL:
        line += 1
    if state == 0:
        if c == "/" and i + 1 < n and src[i + 1] == "/":
            state = 1
            i += 1
        elif c == "/" and i + 1 < n and src[i + 1] == "*":
            state = 2
            i += 1
        elif c == DQ:
            state = 3
        elif c == SQ:
            state = 4
        elif c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth < 0:
                problems.append("zaviraci zavorka navic na radku %d" % line)
                depth = 0
    elif state == 1:
        if c == NL:
            state = 0
    elif state == 2:
        if c == "*" and i + 1 < n and src[i + 1] == "/":
            state = 0
            i += 1
    elif state == 3:
        if c == BS:
            i += 1
        elif c == DQ:
            state = 0
    elif state == 4:
        if c == BS:
            i += 1
        elif c == SQ:
            state = 0
    i += 1

if depth != 0:
    problems.append("neuzavrenych zavorek: %d" % depth)

# ---- kazde TR(...) musi mit protejsek v Lang.h --------------------------
# EN: every TR(...) needs a counterpart in Lang.h
known = set(re.findall(r"T_[A-Z0-9_]+", lang))
used = set(re.findall(r"TR" + re.escape("(") + r"(T_[A-Z0-9_]+)", src))
for tid in sorted(used - known):
    problems.append("TR(%s) neni v Lang.h" % tid)

# ---- wrappery fontu se nesmi volat samy ---------------------------------
# EN: the font wrappers must not call themselves
for fn in ("tCz", "tAs"):
    m = re.search(r"void " + fn + r"\([^)]*\)[^{]*{(.*?)" + NL + "}", src, re.S)
    if m and re.search(r"" + BS + "b" + fn + BS + "s*" + re.escape("("), m.group(1)):
        problems.append("%s() vola sam sebe - nekonecna rekurze" % fn)

# ---- vysledek ------------------------------------------------------------
if problems:
    print("PROBLEMY (%d):" % len(problems))
    for p in problems:
        print("  " + p)
    raise SystemExit(1)

print("struktura v poradku - zavorky vyvazene, %d prekladu pouzito" % len(used))
