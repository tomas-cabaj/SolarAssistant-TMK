"""Check weather icon bounds and screen registry; render an icon contact sheet."""
import ast
import math
import re
from pathlib import Path
from PIL import Image, ImageChops
from render import Screen, weather_icon, C_CARD, C_TXT, F13

root = Path(__file__).resolve().parent
source = (root / "SolarAssistant-TMK.ino").read_text(encoding="utf-8")
table = source.split("const ScreenDefinition screenDefinitions[] = {", 1)[1].split("};", 1)[0]
entries = re.findall(r"\{(T_\w+),\s*(scr\w+)\}", table)
count = int(re.search(r"#define SCREENS\s+(\d+)", source)[1])
assert len(entries) == count
assert len({fn for _, fn in entries}) == count
for _, fn in entries:
    assert re.search(r"void " + fn + r"\(\)", source), fn
translations = {}
for line in (root / "Lang.h").read_text(encoding="utf-8").splitlines():
    if "// T_" in line:
        translations[line.split("// ")[1].strip()] = re.findall(r'"([^"\n]*)"', line)
for title, _ in entries:
    assert title in translations, title
    for label in translations[title]:
        wt = math.ceil(F13.getlength(label))
        assert wt <= 286, (label, wt)
        for status in ["za 12 s", "OFFLINE", "99999 min"]:
            wc, ws = math.ceil(F13.getlength("23:59")), math.ceil(F13.getlength(status))
            lo, hi = 16 + wt + math.ceil(wc / 2), 286 - ws - math.ceil(wc / 2)
            if lo <= hi:
                cx = min(max(160, lo), hi)
                assert cx - wc / 2 >= 8 + wt + 7
                assert cx + wc / 2 <= 294 - ws - 7
            else:
                assert 8 + wc + 8 <= 294 - ws
tree = ast.parse((root / "screens.py").read_text(encoding="utf-8"))
assigns = {n.targets[0].id: n.value for n in tree.body
           if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name)}
assert len(ast.literal_eval(assigns["TITLES"])) == count
assert len(ast.literal_eval(assigns["NAMES"])) == count
assert len(assigns["FUNCS"].elts) == count

sheet = Image.new("RGB", (5 * 100, 4 * 100), C_CARD)
codes = [0, 2, 3, 45, 51, 61, 71, 80, 85, 95]
for n, (night, code) in enumerate((night, code) for night in (False, True) for code in codes):
    canvas = Screen()
    canvas.im.paste(C_CARD, (0, 0, 320, 480))
    weather_icon(canvas, 16, 12, code, night)
    box = ImageChops.difference(canvas.im, Image.new("RGB", canvas.im.size, C_CARD)).getbbox()
    assert box and box[0] >= 16 and box[1] >= 12 and box[2] <= 80 and box[3] <= 76, (code, night, box)
    canvas.txt(f"{code} / {'noc' if night else 'den'}", 50, 89, F13, C_TXT, "MC")
    sheet.paste(canvas.im.crop((0, 0, 100, 100)), ((n % 5) * 100, (n // 5) * 100))
sheet.save(root / "images" / "weather-icons.png")
print(f"OK: {count} screens; 20 weather variants stay within 64x64 px")
