# -*- coding: utf-8 -*-
"""Overi, ze kazdy zobrazovany znak ma glyf ve fontu."""
import io, os
NL = chr(10); DQ = chr(34); BS = chr(92)
D = os.path.dirname(os.path.abspath(__file__)) + os.sep

glyphs = set(io.open(D + "glyphs.txt", encoding="utf-8").read())
ascii_ok = set(chr(c) for c in range(0x20, 0x7F))
src = io.open(D + "SolaAssistant-TMK.ino", encoding="utf-8").read()

lits = []
i, n = 0, len(src)
while i < n:
    c = src[i]
    if c == "/" and i+1 < n and src[i+1] == "/":
        while i < n and src[i] != NL: i += 1
        continue
    if c == "/" and i+1 < n and src[i+1] == "*":
        i += 2
        while i+1 < n and not (src[i] == "*" and src[i+1] == "/"): i += 1
        i += 2; continue
    if c == DQ:
        i += 1; buf = ""
        while i < n and src[i] != DQ:
            if src[i] == BS: buf += src[i:i+2]; i += 2; continue
            buf += src[i]; i += 1
        lits.append(buf); i += 1; continue
    i += 1

lang = io.open(D + "Lang.h", encoding="utf-8").read()
for line in lang.split(NL):
    if line.startswith("  { " + DQ) or "LANG_NAME" in line:
        lits += line.split(DQ)[1::2]

missing = {}
for lit in lits:
    for ch in lit:
        if ch in ascii_ok or ch in glyphs: continue
        missing.setdefault(ch, lit)

if missing:
    print("ZNAKY BEZ GLYFU (%d):" % len(missing))
    for ch, where in missing.items():
        print("   U+%04X  v retezci: %s" % (ord(ch), where[:40]))
else:
    print("vsechny zobrazovane znaky maji glyf (%d glyfu nad ASCII)" % len(glyphs))
