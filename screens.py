# -*- coding: utf-8 -*-
"""Jednotlive obrazovky s fiktivnimi daty."""
import sys, os, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from render import *

ENGLISH = "--en" in sys.argv
if ENGLISH:
    OUT = D + "images-en/"

# Preklad textu v nahledech bez duplikovani definic vsech obrazovek.
# EN: Translate preview text without duplicating every screen definition.
_EN_REPLACEMENTS = {
    "PŘEHLED": "OVERVIEW", "BATERIE": "BATTERY", "DOBĚH BATERIE": "BATTERY RUNTIME",
    "SOLÁR": "SOLAR", "SÍŤ A ZÁTĚŽ": "GRID & LOAD", "MĚNIČ": "INVERTER",
    "POČASÍ": "WEATHER", "GRAFY": "CHARTS", "TEPLOTY": "TEMPERATURES",
    "HISTORIE": "HISTORY", "ÚSPORY": "SAVINGS", "PREDIKCE ÚSPOR": "SAVINGS FORECAST",
    "DNES A VČERA": "TODAY & YESTERDAY", "NÁVRATNOST": "PAYBACK",
    "CHYBY A VÝPADKY": "ERRORS & OUTAGES", "UPOZORNĚNÍ": "ALERTS",
    "NASTAVENÍ 2": "SETTINGS 2", "NASTAVENÍ": "SETTINGS", "O APLIKACI": "ABOUT",
    "Měnič": "Inverter", "Solární PV": "Solar PV", "Síť": "Grid", "Baterie": "Battery",
    "Predikce": "Forecast", "Dnes": "Today", "Měsíc": "Month", "Rok": "Year",
    "Vyrobeno": "Produced", "Zbývá": "Remaining", "Ušetřeno": "Saved",
    "Výroba": "Production", "Spotřeba": "Consumption", "Nabito": "Charged", "Vybito": "Discharged",
    "Soběstačnost": "Self-sufficiency", "proti včerejšku": "vs. yesterday",
    "Plán": "Plan", "Skutečnost": "Actual", "Roční plán": "Year plan",
    "Predikce úspor": "Savings forecast", "Plán vs. Skutečnost": "Plan vs. Actual",
    "Predikce úspor": "Savings forecast", "predikce úspor": "savings forecast",
    "Nabito dnes": "Charged today", "Vybito dnes": "Discharged today",
    "Zbývá v baterii": "Battery remaining",
    "Napětí": "Voltage", "Proud": "Current", "Výkon": "Power", "Zátěž": "Load",
    "Zatížení měniče": "Inverter load", "Frekvence": "Frequency", "Odběr": "Import",
    "Dodáno": "Export", "Vlastní spotřeba měniče": "Inverter self-consumption",
    "Počasí": "Weather", "Teplota": "Temperature", "Historie": "History",
    "Upozornění": "Alerts", "Chyby": "Errors", "Výpadky": "Outages",
    "Nastavení": "Settings", "O aplikaci": "About", "Volná paměť": "Free memory",
    "Vráceno": "Repaid", "Provoz od": "Operating since", "Odhad do splacení": "Estimated payback",
    "čas běhu": "uptime", "Signál": "Signal", "Wi-Fi síť": "Wi-Fi network",
    "OTA název": "OTA name", "RESTART ZAŘÍZENÍ": "RESTART DEVICE",
    "Maximum dne": "Daily maximum", "Min dnes": "Min today", "SOC večer": "SOC evening",
    "Dnes a včera": "Today & yesterday", "Max zátěže": "Max load",
    "NABÍJÍ SE": "CHARGING", "VYBÍJÍ SE": "DISCHARGING",
    "za 12 s": "in 12 s", "do Serialu": "to Serial", "porty cíle": "target ports",
}
_ORIGINAL_TXT = Screen.txt
def _translated_txt(self, text, x, y, font=F13, col=C_TXT, datum="TL"):
    if ENGLISH and isinstance(text, str):
        for source in sorted(_EN_REPLACEMENTS, key=len, reverse=True):
            target = _EN_REPLACEMENTS[source]
            text = text.replace(source, target)
    return _ORIGINAL_TXT(self, text, x, y, font, col, datum)
Screen.txt = _translated_txt

# ---- fiktivni data -------------------------------------------------------
pv_power, load_power, grid_power, batt_power = 1840, 620, 0, 1180
soc, batt_v, batt_a, capacity = 78, 27.4, 43.1, 9.6
grid_v, grid_hz, ac_v, ac_hz = 238, 50.0, 230, 50.0
load_pct, sys_power, bus_v = 26, 45, 404
inv_temp, max_va, pv_v, pv_a = 58.2, 3000, 27.2, 67.6
load_va, float_v, absorp_v, max_chg = 620, 27.3, 28.2, 80
cloud, irrad, temp_out, wind = 35, 612, 18.4, 10.9
pv_gen, pv_rem, pv_prog, pv_pred = 4.33, 2.45, 63, 1620
day_load, day_bin, day_bout, saved = 2.10, 2.35, 1.42, 21
peak_pv, max_load, min_soc = 2260, 1840, 62
sunrise, sunset = "06:42", "19:18"
clock = "14:23"

TITLES = {
 0:"SolarAssistant-TMK", 1:"BATERIE", 2:"DOBĚH BATERIE", 3:"SOLÁR", 4:"SÍŤ A ZÁTĚŽ",
 5:"MĚNIČ", 6:"POČASÍ", 7:"GRAFY", 8:"TEPLOTY", 9:"HISTORIE", 10:"ÚSPORY",
 11:"PREDIKCE ÚSPOR", 12:"DNES A VČERA", 13:"NÁVRATNOST", 14:"CHYBY A VÝPADKY",
 15:"UPOZORNĚNÍ", 16:"NASTAVENÍ", 17:"NASTAVENÍ 2", 18:"O APLIKACI"}
if ENGLISH:
    TITLES = {0:"SolarAssistant-TMK", 1:"BATTERY", 2:"BATTERY RUNTIME", 3:"SOLAR", 4:"GRID & LOAD",
              5:"INVERTER", 6:"WEATHER", 7:"CHARTS", 8:"TEMPERATURES", 9:"HISTORY", 10:"SAVINGS",
              11:"SAVINGS FORECAST", 12:"TODAY & YESTERDAY", 13:"PAYBACK", 14:"ERRORS & OUTAGES",
              15:"ALERTS", 16:"SETTINGS", 17:"SETTINGS 2", 18:"ABOUT"}

PAGE_ICONS = {1:3, 2:8, 3:1, 4:2, 5:0, 6:6, 7:7, 8:5, 9:8,
              10:9, 11:10, 12:14, 13:15, 14:12, 15:13, 16:11,
              17:11, 18:4}

HDR_H, NAV_Y, CONT_Y = 40, 432, 42


def chrome(s, idx):
    """hlavicka a ovladaci lista"""
    s.rect(0, 0, W, HDR_H, C_CARD)
    s.line(0, HDR_H, W - 1, HDR_H, C_LINE)
    s.circle(W - 14, 20, 5, C_BATT)
    icon = PAGE_ICONS.get(idx, -1)
    title_x = 46 if icon >= 0 else 8
    if icon >= 0:
        s.im.paste(UI_ICONS[icon], (0, 1))
    if idx == 0:
        s.txt(TITLES[idx], 8, 10, F13, C_TXT)
        s.txt(clock, W - 26, 12, F13, C_TXT, "TR")
    else:
        wt = s.d.textlength(TITLES[idx], font=F13)
        wc = s.d.textlength(clock, font=F13)
        ws = s.d.textlength("za 12 s", font=F13)
        lo, hi = title_x + wt + math.ceil(wc / 2) + 8, W - 34 - ws - math.ceil(wc / 2)
        two_rows = lo > hi
        cx = (46 + math.ceil(wc / 2) if icon >= 0 else 8 + math.ceil(wc / 2)) if two_rows else min(max(160, lo), hi)
        s.txt(TITLES[idx], title_x, 1 if two_rows else 10, F13, C_TXT)
        s.txt(clock, cx, 25 if two_rows else 19, F13, C_TXT, "MC")
        s.txt("za 12 s", W - 26, 18 if two_rows else 12, F13, C_DIM, "TR")

    s.rect(0, HDR_H - 6, W, 3, C_CARD)
    s.rect(0, HDR_H - 6, 128, 3, C_BATT)
    s.rect(0, HDR_H - 3, W, 3, C_CARD)
    s.rect(0, HDR_H - 3, 210, 3, C_LOAD)

    # ovladaci lista
    s.rect(0, NAV_Y - 2, W, H - NAV_Y + 2, C_BG)
    s.line(0, NAV_Y - 2, W - 1, NAV_Y - 2, C_LINE)
    y, h = NAV_Y + 2, 38
    def button(x, active=False):
        s.rrect(x, y + 2, 88, h, 8, C_TRACK)
        s.rrect(x, y, 88, h - 2, 8, C_CARD)
        s.rrect(x, y, 88, h - 2, 8, C_PV if active else C_LINE, fill=False)
        s.line(x + 10, y + 3, x + 77, y + 3, C_TXT if active else C_DIM)

    button(8);   s.im.paste(UI_NAV_ICONS[0], (37, y + 3))
    button(116, idx == 0); s.im.paste(UI_NAV_ICONS[1], (145, y + 3))
    button(224); s.im.paste(UI_NAV_ICONS[2], (253, y + 3))

    dx = W // 2 - (len(TITLES) * 10) // 2
    for i in range(len(TITLES)):
        s.circle(dx + i * 10 + 4, NAV_Y - 9, 2, C_TXT if i == idx else C_LINE)


def kw(w):
    return "%.2f kW" % (w / 1000.0) if abs(w) >= 1000 else "%d W" % w


# ============================== 0 PREHLED =================================
def scr0():
    s = Screen()
    s.card(6,   44, 150, 56, UI_ICON_INVERTER, C_LOAD, "Měnič", "%.1f °C" % inv_temp,
           "využití %d %%" % load_pct)
    s.card(164, 44, 150, 56, UI_ICON_SOLAR, C_PV, "Solární PV", kw(pv_power),
           "%.1fV/%.1fA" % (pv_v, pv_a))
    s.card(6,  104, 150, 56, UI_ICON_GRID, C_GRID, "Síť", "%dV/%.1fHz" % (grid_v, grid_hz), kw(grid_power))
    s.card(164,104, 150, 56, UI_ICON_BATTERY, C_BATT, "Baterie", "%.1f V" % batt_v, "%d %%" % soc)
    s.txt("+%.0fA" % batt_a, 164 + 50 + s.d.textlength("%d %%" % soc, font=F13) + 8,
          104 + 39, F13, C_BATT)

    s.rrect(6, 164, 308, 64, 8, C_CARD)
    s.rrect(6, 164, 308, 64, 8, C_PV, fill=False)
    s.txt("Predikce", 14, 168, F13, C_DIM)
    s.txt("Dnes %d Kč / %d Kč" % (saved, saved + int(pv_rem * 6.5)), 160, 177, F13, C_BATT, "MC")
    s.txt("%d %%" % pv_prog, 306, 168, F13, C_PV, "TR")
    s.txt("%.2f kWh" % pv_gen, 14, 188, F13, C_TXT)
    s.txt("Vyrobeno", 14, 206, F13, C_DIM)
    s.txt("%.2f kWh" % pv_rem, 306, 188, F13, C_TXT, "TR")
    s.txt("Zbývá", 306, 206, F13, C_DIM, "TR")
    s.bar(104, 192, 112, 12, pv_prog / 100.0, C_PV)

    s.gauge(84,  300, 62, 13, load_power, 2000, C_LOAD, kw(load_power), "Zátěž")
    s.gauge(236, 300, 62, 13, pv_power, 2000, C_PV, kw(pv_power), "Solární PV")
    s.gauge(84,  394, 62, 13, abs(grid_power), 2000, C_DIM, kw(grid_power), "Síť")
    s.gauge(236, 394, 62, 13, abs(batt_power), 1000, C_BATT, "+%.2f kW" % (batt_power/1000.0), "Baterie nabíjí")
    chrome(s, 0); return s


# ============================== 1 BATERIE =================================
def scr1():
    s = Screen()
    cx, cy, r, th = 160, 208, 118, 22
    col = C_BATT
    s.arc(cx, cy, r - th, r, 180, 360, C_TRACK)
    s.arc(cx, cy, r - th, r, 180, 180 + 180 * soc / 100.0, col)
    s.ticks(cx, cy, r)

    num = "%d" % soc
    wn = s.d.textlength(num, font=F7)
    x0 = cx - (wn + 5 + 12) / 2
    s.txt(num, x0, 138, F7, col)
    s.txt("%", x0 + wn + 5, 162, F13, C_DIM)
    s.txt("+%.1f %%/hod" % (batt_power / 1000.0 / capacity * 100), cx, 196, F4, C_BATT, "MC")
    s.txt("NABÍJÍ SE", cx, 220, F13, C_TXT, "MC")

    sy, sh, sg = 232, 50, 5
    s.tile(6,   sy,             150, sh, "Napětí", "%.1f V" % batt_v, C_TXT)
    s.tile(164, sy,             150, sh, "Proud",  "%.1f A" % batt_a, C_TXT)
    s.tile(6,   sy+sh+sg,       150, sh, "Výkon",  kw(batt_power), C_BATT)
    s.tile(164, sy+sh+sg,       150, sh, "Zbývá v baterii", "%.2f kWh" % (capacity*soc/100), C_TXT)
    s.tile(6,   sy+2*(sh+sg),   150, sh, "Nabito dnes", "%.2f kWh" % day_bin, C_BATT)
    s.tile(164, sy+2*(sh+sg),   150, sh, "Vybito dnes", "%.2f kWh" % day_bout, C_PV)

    s.txt("Min dnes", 6, 396, F13, C_DIM)
    s.txt("%d %%" % min_soc, 150, 396, F13, C_TXT, "TR")
    evening = "%.0f %%" % min(100, soc + pv_rem / capacity * 100)
    s.txt("SOC večer", 314 - s.d.textlength(evening, font=F13) - 8, 396, F13, C_DIM, "TR")
    s.txt(evening, 314, 396, F13, C_BATT, "TR")
    chrome(s, 1); return s


# ============================== 2 DOBEH ===================================
def scr2():
    s = Screen()
    h = capacity * (100 - soc) / 100.0 / (batt_power / 1000.0)
    s.txt("%d:%02d" % (int(h), int((h - int(h)) * 60)), W//2, 110, F7, C_BATT, "MC")
    s.txt("Plná za", W//2, 156, F13, C_TXT, "MC")
    s.bar(30, 180, 260, 16, soc / 100.0, C_BATT)

    sy, sh, sg = 210, 52, 6
    s.tile(6,   sy,           150, sh, "Stav nabití", "%d %%" % soc, C_BATT)
    s.tile(164, sy,           150, sh, "Výkon", kw(batt_power), C_BATT)
    s.tile(6,   sy+sh+sg,     150, sh, "Proud", "%.1f A" % batt_a, C_TXT)
    s.tile(164, sy+sh+sg,     150, sh, "Kapacita", "%.1f kWh" % capacity, C_TXT)
    s.tile(6,   sy+2*(sh+sg), 150, sh, "Zbývá v baterii", "%.2f kWh" % (capacity*soc/100), C_TXT)
    s.tile(164, sy+2*(sh+sg), 150, sh, "Zátěž", kw(load_power), C_LOAD)

    by = sy + 3 * (sh + sg)
    hl = (capacity * soc / 100.0) / (load_power / 1000.0)
    s.txt("při zátěži", 6, by, F13, C_DIM)
    s.txt("%d h %02d min" % (int(hl), int((hl - int(hl)) * 60)), 314, by, F13, C_TXT, "TR")
    chrome(s, 2); return s


# ============================== 3 SOLAR ===================================
def scr3():
    s = Screen()
    s.gauge(160, 160, 100, 20, pv_power, 2000, C_PV, kw(pv_power), "aktuální výkon FVE")
    s.txt("Průběh dne", 6, 186, F13, C_DIM)
    s.txt("%d %%" % pv_prog, 314, 186, F2, C_PV, "TR")
    s.bar(6, 204, 308, 14, pv_prog / 100.0, C_PV)

    sy, sh, sg = 228, 50, 5
    s.tile(6,   sy,           150, sh, "Dnes vyrobeno", "%.2f kWh" % pv_gen, C_PV)
    s.tile(164, sy,           150, sh, "Zbývá dnes", "%.2f kWh" % pv_rem, C_TXT)
    s.tile(6,   sy+sh+sg,     150, sh, "Predikce", kw(pv_pred), C_WEATH)
    s.tile(164, sy+sh+sg,     150, sh, "Osvit", "%d W/m2" % irrad, C_WEATH)
    s.tile(6,   sy+2*(sh+sg), 150, sh, "Napětí panelů", "%.1f V" % pv_v, C_TXT)
    s.tile(164, sy+2*(sh+sg), 150, sh, "Proud panelů", "%.1f A" % pv_a, C_TXT)

    s.txt("Špička dnes", 6, 396, F13, C_DIM)
    s.txt(kw(peak_pv), 314, 396, F13, C_PV, "TR")
    chrome(s, 3); return s


# ============================== 4 SIT =====================================
def scr4():
    s = Screen()
    s.gauge(84,  150, 68, 15, abs(grid_power), 2000, C_DIM, kw(grid_power), "bez odběru")
    s.gauge(236, 150, 68, 15, load_power, 2000, C_LOAD, kw(load_power), "zátěž domu")
    s.txt("Zatížení měniče", 6, 178, F13, C_DIM)
    s.txt("%d %%" % load_pct, 314, 178, F2, C_LOAD, "TR")
    s.bar(6, 196, 308, 14, load_pct / 100.0, C_LOAD)

    sy, sh, sg = 216, 48, 6
    s.tile(6,   sy,       150, sh, "Napětí sítě", "%d V" % grid_v, C_TXT)
    s.tile(164, sy,       150, sh, "Frekvence", "%.1f Hz" % grid_hz, C_TXT)
    s.tile(6,   sy+sh+sg, 150, sh, "Výstup měniče", "%d V" % ac_v, C_TXT)
    s.tile(164, sy+sh+sg, 150, sh, "Výstup Hz", "%.1f Hz" % ac_hz, C_TXT)
    s.tile(6,   sy+2*(sh+sg), 150, sh, "Odebráno", "0.00 kWh", C_GRID)
    s.tile(164, sy+2*(sh+sg), 150, sh, "Dodáno", "0.00 kWh", C_BATT)

    fy = sy + 3 * (sh + sg) + 4
    s.txt("Vlastní spotřeba měniče", 6, fy, F13, C_DIM)
    s.txt("%d W" % sys_power, 314, fy, F13, C_TXT, "TR")
    s.txt("Maximum dnes", 6, 400, F13, C_DIM)
    s.txt(kw(max_load), 314, 400, F13, C_LOAD, "TR")
    chrome(s, 4); return s


# ============================== 5 POCASI ==================================
def scr5():
    s = Screen()
    s.panel(6, 48, 308, 92)
    weather_icon(s, 18, 64, 2)
    s.txt("polojasno", 190, 62, F13, C_TXT, "MC")
    s.txt("%.1f °C" % temp_out, 190, 92, F6, C_WEATH, "MC")
    s.txt("Stav: Den", 190, 124, F13, C_DIM, "MC")

    s.gauge(160, 213, 62, 13, cloud, 100, C_DIM, "%d %%" % cloud, "oblačnost")
    s.txt("Den", 48, 177, F13, C_PV, "MC"); s.txt("53 %", 48, 198, F13, C_PV, "MC")
    s.txt("Noc", 272, 177, F13, C_LOAD, "MC"); s.txt("47 %", 272, 198, F13, C_LOAD, "MC")
    s.rrect(12, 242, 296, 12, 6, C_LINE)
    s.rect(14, 245, 153, 6, C_PV); s.rect(167, 245, 139, 6, C_LOAD)

    sy, sh, sg = 262, 48, 5
    s.tile(6,   sy,           150, sh, "Vítr", "%.1f km/h" % wind, C_WEATH)
    s.tile(164, sy,           150, sh, "Osvit", "%d W/m2" % irrad, C_PV)
    s.tile(6,   sy+sh+sg,     150, sh, "Východ", sunrise, C_PV)
    s.tile(164, sy+sh+sg,     150, sh, "Západ", sunset, C_LOAD)
    s.tile(6,   sy+2*(sh+sg), 150, sh, "Délka dne", "12:36", C_PV)
    s.tile(164, sy+2*(sh+sg), 150, sh, "Délka noci", "11:24", C_LOAD)
    chrome(s, 6); return s


# ============================== 6 MENIC ===================================
def scr6():
    s = Screen()
    s.gauge(160, 160, 100, 20, inv_temp, 100, C_PV, "%.1f °C" % inv_temp, "teplota měniče")
    s.txt("Využití výkonu", 6, 186, F13, C_DIM)
    s.txt("%d %%" % load_pct, 314, 186, F2, C_LOAD, "TR")
    s.bar(6, 204, 308, 14, load_pct / 100.0, C_LOAD)

    sy, sh, sg = 228, 50, 5
    s.tile(6,   sy,           150, sh, "Max výkon", "%d VA" % max_va, C_TXT)
    s.tile(164, sy,           150, sh, "Zdánlivý výkon", "%d VA" % load_va, C_TXT)
    s.tile(6,   sy+sh+sg,     150, sh, "Bus napětí", "%d V" % bus_v, C_TXT)
    s.tile(164, sy+sh+sg,     150, sh, "Max nabíj. proud", "%d A" % max_chg, C_BATT)
    s.tile(6,   sy+2*(sh+sg), 150, sh, "Absorpce", "%.2f V" % absorp_v, C_BATT)
    s.tile(164, sy+2*(sh+sg), 150, sh, "Udržovací", "%.2f V" % float_v, C_BATT)

    s.txt("Vlastní spotřeba měniče", 6, 396, F13, C_DIM)
    s.txt("%d W" % sys_power, 314, 396, F13, C_TXT, "TR")
    chrome(s, 5); return s


# ============================== 7 GRAFY ===================================
def day_curve(i, peak, width, centre):
    """zvonovity prubeh dne"""
    return peak * math.exp(-((i - centre) ** 2) / (2.0 * width ** 2))


def scr7():
    s = Screen()
    GR_L, GR_W, X0, STEP = 30, 288, 31, 2
    now = 86          # 14:20

    def frame(gy, gh, title, right, col):
        s.txt(title, GR_L, gy - 18, F13, C_DIM)
        s.txt(right, 314, gy - 18, F13, col, "TR")
        s.rect(GR_L, gy, GR_W, gh, C_BG)
        s.rect(GR_L, gy, GR_W, gh, C_LINE, fill=False)
        for k in range(1, 4):
            y = gy + gh * k // 4
            for x in range(GR_L + 3, GR_L + GR_W - 2, 6): s.d.point((x, y), C_CARD)
        for hh in range(6, 24, 6):
            x = X0 + hh * 6 * STEP
            for y in range(gy + 3, gy + gh - 2, 5): s.d.point((x, y), C_CARD)
        s.line(X0 + now * STEP, gy + 1, X0 + now * STEP, gy + gh - 2, C_LINE)

    def yax(gy, gh, a, b, c):
        s.txt(a, GR_L - 1, gy + 8, F13, C_DIM, "MR")
        s.txt(b, GR_L - 1, gy + gh // 2, F13, C_DIM, "MR")
        s.txt(c, GR_L - 1, gy + gh - 8, F13, C_DIM, "MR")

    # 1) FVE a zatez
    frame(68, 90, "FVE a zátěž", "max 2400 W", C_TXT)
    yax(68, 90, "2k", "1k", "0")
    prev = None
    for i in range(now):
        v = day_curve(i, 2260, 26, 82)
        y = 68 + 89 - int(v * 88 / 2400)
        x = X0 + i * STEP
        if prev: s.line(prev[0], prev[1], x, y, C_PV)
        prev = (x, y)
    prev = None
    for i in range(now):
        v = 400 + 500 * abs(math.sin(i / 7.0)) + (900 if 40 < i < 46 else 0)
        y = 68 + 89 - int(v * 88 / 2400)
        x = X0 + i * STEP
        if prev: s.line(prev[0], prev[1], x, y, C_LOAD)
        prev = (x, y)
    s.txt("FVE", GR_L + 4, 160, F13, C_PV)
    s.txt("Zátěž", GR_L + 44, 160, F13, C_LOAD)

    # 2) vykon baterie
    frame(202, 90, "Výkon baterie", "+-1500 W", C_BATT)
    yax(202, 90, "+2k", "0", "-2k")
    s.line(GR_L + 1, 202 + 45, GR_L + GR_W - 2, 202 + 45, C_LINE)
    prev = None
    for i in range(now):
        v = day_curve(i, 1500, 24, 80) - 700 * math.exp(-((i - 30) ** 2) / 300.0)
        y = 202 + 45 - int(v * 43 / 2000)
        x = X0 + i * STEP
        if prev: s.line(prev[0], prev[1], x, y, C_BATT)
        prev = (x, y)

    # 3) SOC
    frame(336, 60, "Stav nabití", "%d %%" % soc, C_BATT)
    yax(336, 60, "100", "50", "0")
    prev = None
    for i in range(now):
        v = 45 + 33 * (1 / (1 + math.exp(-(i - 60) / 12.0)))
        y = 336 + 59 - int(v * 58 / 100)
        x = X0 + i * STEP
        if prev: s.line(prev[0], prev[1], x, y, C_BATT)
        prev = (x, y)

    for hh in range(0, 25, 6):
        x = min(max(X0 + hh * 6 * STEP, GR_L), GR_L + GR_W - 8)
        s.txt(str(hh), x, 406, F13, C_DIM, "MC")
    chrome(s, 7); return s


# ============================== 8 TEPLOTY =================================
def scr8temps():
    s = Screen()
    GR_L, GR_W, X0, STEP = 30, 288, 31, 2
    now = 86

    def frame(gy, title, right, col, lo, hi, values):
        gh = 120
        s.txt(title, GR_L, gy - 18, F13, C_DIM)
        s.txt(right, 314, gy - 18, F13, col, "TR")
        s.rect(GR_L, gy, GR_W, gh, C_LINE, fill=False)
        for k in range(1, 4):
            y = gy + gh * k // 4
            for x in range(GR_L + 3, GR_L + GR_W - 2, 6): s.d.point((x, y), C_CARD)
        prev = None
        for i in range(now):
            v = values(i)
            x = X0 + i * STEP
            y = gy + gh - 1 - int((v - lo) * (gh - 2) / (hi - lo))
            if prev: s.line(prev[0], prev[1], x, y, col)
            prev = (x, y)
        s.txt(str(hi), GR_L - 1, gy + 8, F13, C_DIM, "MR")
        s.txt(str((hi + lo) // 2), GR_L - 1, gy + gh // 2, F13, C_DIM, "MR")
        s.txt(str(lo), GR_L - 1, gy + gh - 8, F13, C_DIM, "MR")

    frame(82, "Teplota měniče", "58.2 °C", C_PV, 0, 100,
          lambda i: 47 + 12 * math.sin(i / 20.0))
    frame(272, "Venkovní teplota", "18.4 °C", C_WEATH, -20, 40,
          lambda i: 11 + 8 * math.sin((i - 45) / 40.0))
    for hh in range(0, 25, 6):
        x = min(max(X0 + hh * 6 * STEP, GR_L), GR_L + GR_W - 8)
        s.txt(str(hh), x, 412, F13, C_DIM, "MC")
    chrome(s, 8); return s


# ============================== 9 HISTORIE ================================
DAYS = [(12, 3.9, 2.4, 2.2, 1.8, 24), (13, 5.3, 3.4, 4.1, 3.3, 34),
        (14, 1.3, 1.0, 1.0, 1.4, 10), (15, 5.9, 2.2, 5.4, 2.6, 22),
        (16, 5.3, 3.4, 4.1, 3.3, 34), (17, 4.5, 3.0, 3.4, 3.1, 30),
        (18, 4.33, 2.1, 2.35, 1.42, 21)]

def scr8():
    s = Screen()
    s.txt("Posledních 7 dní", 6, 48, F13, C_DIM)
    s.txt("změna klepnutím", 314, 48, F13, C_LINE, "TR")
    gx, gy, gw, gh = 32, 74, 282, 140
    mx = max(max(d[1], d[2]) for d in DAYS)
    s.rect(gx, gy, gw, gh, C_LINE, fill=False)
    for k in range(1, 4):
        y = gy + gh * k // 4
        for x in range(gx + 3, gx + gw - 2, 6): s.d.point((x, y), C_CARD)
    s.txt("%.1f" % mx, gx - 4, gy + 2, F13, C_DIM, "TR")
    s.txt("%.1f" % (mx / 2), gx - 4, gy + gh // 2, F13, C_DIM, "TR")
    s.txt("0", gx - 4, gy + gh - 2, F13, C_DIM, "TR")

    step = gw // len(DAYS)
    for i, (dn, pv, ld, bi, bo, sv) in enumerate(DAYS):
        x = gx + i * step
        bw = max(1, step // 2 - 2)
        hp = int(pv * (gh - 4) / mx)
        hl = int(ld * (gh - 4) / mx)
        s.rect(x + 2, gy + gh - hp - 1, bw, hp, C_PV)
        s.rect(x + 3 + bw, gy + gh - hl - 1, bw, hl, C_LOAD)
        s.txt(str(dn), x + step // 2, gy + gh + 12, F13, C_DIM, "MC")

    s.txt("Výroba", 12, 236, F13, C_PV)
    s.txt("Spotřeba", 170, 236, F13, C_LOAD)
    s.txt("%.1f kWh" % mx, 314, 236, F13, C_DIM, "TR")

    sPv = sum(d[1] for d in DAYS); sLd = sum(d[2] for d in DAYS)
    sBi = sum(d[3] for d in DAYS); sBo = sum(d[4] for d in DAYS)
    sSv = sum(d[5] for d in DAYS)

    sy, sh, sg = 260, 50, 5
    s.tile(6,   sy,       150, sh, "Výroba", "%.1f kWh" % sPv, C_PV)
    s.tile(164, sy,       150, sh, "Spotřeba", "%.1f kWh" % sLd, C_LOAD)
    s.tile(6,   sy+sh+sg, 150, sh, "Nabito", "%.1f kWh" % sBi, C_BATT)
    s.tile(164, sy+sh+sg, 150, sh, "Vybito", "%.1f kWh" % sBo, C_PV)
    s.tile(6,   sy+2*(sh+sg), 308, sh, "Ušetřeno", "%d Kč" % sSv, C_BATT)
    chrome(s, 9); return s


# ============================== 10 USPORY =================================
def scr9():
    s = Screen()
    y = 52
    s.txt("Výroba", 150, y, F13, C_PV, "TR")
    s.txt("Spotřeba", 236, y, F13, C_LOAD, "TR")
    s.txt("Ušetřeno", 306, y, F13, C_BATT, "TR")
    y += 26

    rows = [("Dnes", pv_gen, day_load, saved),
            ("Měsíc", 75.5, 54.0, 540),
            ("Rok", 612.4, 486.2, 4862)]
    rowH = 46
    for i, (lb, pv, ld, sv) in enumerate(rows):
        ry = y + i * (rowH + 6)
        s.panel(6, ry, 308, rowH)
        s.txt(lb, 14, ry + 15, F13, C_TXT)
        s.txt("%.1f kWh" % pv, 150, ry + 15, F13, C_PV, "TR")
        s.txt("%.1f kWh" % ld, 236, ry + 15, F13, C_LOAD, "TR")
        s.txt("%d Kč" % sv, 306, ry + 15, F13, C_BATT, "TR")

    uy = y + 3 * (rowH + 6) + 2
    ss = 78
    s.txt("Soběstačnost", 6, uy, F13, C_DIM)
    s.txt("%d %%" % ss, 314, uy, F13, C_PV, "TR")
    s.bar(6, uy + 18, 308, 14, ss / 100.0, C_PV)
    s.txt("proti včerejšku", 6, uy + 38, F13, C_DIM)
    s.txt("+12 %", 314, uy + 38, F13, C_BATT, "TR")

    gy = uy + 92
    gh = 70
    s.txt("Ušetřeno", 6, gy - 21, F13, C_DIM)
    gx, gw = 32, 282
    s.txt("34 Kč", 314, gy - 21, F13, C_DIM, "TR")
    s.rect(gx, gy, gw, gh, C_LINE, fill=False)
    vals = [24, 34, 10, 22, 34, 30, 21, 28, 19, 26, 31, 15, 23, 21]
    mx = max(vals)
    for j, label in enumerate((mx, mx / 2, 0)):
        sy = gy + (0 if j == 0 else gh // 2 if j == 1 else gh - 2)
        s.txt("%d" % label, gx - 4, sy, F2, C_DIM, "TR")
    for j in range(1, 4):
        yy = gy + (gh * j) // 4
        for xx in range(gx + 3, gx + gw - 2, 6):
            s.line(xx, yy, min(xx + 2, gx + gw - 3), yy, C_LINE)
    step = gw // len(vals)
    for i, v in enumerate(vals):
        hh = int(v * (gh - 4) / mx)
        s.rect(gx + i * step + 1, gy + gh - hh - 1, step - 3, hh, C_BATT)
        if i % 2 == 0:
            s.txt(str(12 + i), gx + i * step + step // 2, gy + gh + 12, F13, C_DIM, "MC")
    chrome(s, 10); return s


# ============================== 11 PREDIKCE USPORY ========================
def scr11forecast():
    s = Screen()
    price = 6.0
    plan_kwh = [5.5, 15.3, 72.1, 65.8, 82.8, 92.4, 104.0, 108.0, 57.9, 31.1, 22.0, 9.9]
    actual = [28, 84, 416, 384, 468, 532, 604, 620, 347, 0, 0, 0]
    plan = [v * price for v in plan_kwh]
    s.tile(6, 52, 150, 52, "Plán", "%d Kč" % plan[8], C_PV, small=True)
    s.tile(164, 52, 150, 52, "Skutečnost", "%d Kč" % actual[8], C_BATT, small=True)
    s.tile(6, 110, 150, 52, "Roční plán", "%d Kč" % sum(plan), C_PV, small=True)
    s.tile(164, 110, 150, 52, "Ušetřeno", "%d Kč" % sum(actual), C_BATT, small=True)
    gx, gy, gw, gh = 32, 196, 282, 184
    mx = max(max(plan), max(actual))
    s.txt("Predikce úspor", gx, gy - 22, F13, C_DIM)
    s.txt("%d Kč" % mx, gx + gw, gy - 22, F13, C_DIM, "TR")
    s.rect(gx, gy, gw, gh, C_LINE, fill=False)
    for j, label in enumerate((mx, mx / 2, 0)):
        sy = gy + (0 if j == 0 else gh // 2 if j == 1 else gh - 2)
        s.txt("%d" % label, gx - 4, sy, F2, C_DIM, "TR")
    for j in range(1, 4):
        yy = gy + (gh * j) // 4
        for xx in range(gx + 3, gx + gw - 2, 6):
            s.line(xx, yy, min(xx + 2, gx + gw - 3), yy, C_LINE)
    step = gw // 12
    for i in range(12):
        x = gx + i * step
        hp = int(plan[i] * (gh - 4) / mx)
        ha = int(actual[i] * (gh - 4) / mx)
        s.rect(x + 2, gy + gh - hp - 1, 9, hp, C_PV)
        if ha: s.rect(x + 12, gy + gh - ha - 1, 9, ha, C_BATT)
        s.txt(str(i + 1), x + step // 2, gy + gh + 12, F13, C_DIM, "MC")
    s.txt("Plán vs. Skutečnost", 160, 404, F13, C_DIM, "MC")
    chrome(s, 11); return s


# ============================== 12 NASTAVENI ==============================
def scr10():
    s = Screen()
    sh, sg = 50, 6
    y = 52
    s.tile(6, y, 308, sh, "IP adresa desky", "192.168.10.113", C_TXT, small=True)
    y += sh + sg
    s.tile(6,   y, 150, sh, "Signál", "-58 dBm", C_TXT)
    s.tile(164, y, 150, sh, "Čas běhu", "0r 0m 0d 6h 52m", C_TXT, small=True)
    y += sh + sg
    s.tile(6,   y, 150, sh, "Wi-Fi síť", "Cabajovi", C_DIM, small=True)
    s.tile(164, y, 150, sh, "OTA název", "esp32-solar-lcd", C_DIM, small=True)

    by, bh, bw = 282, 52, 150
    s.rrect(6, by, bw, bh, 8, C_CARD);   s.rrect(6, by, bw, bh, 8, C_WEATH, fill=False)
    s.txt("VÝPIS HODNOT", 6 + bw//2, by + 16, F13, C_WEATH, "MC")
    s.txt("do Serialu", 6 + bw//2, by + 34, F13, C_DIM, "MC")
    s.rrect(164, by, bw, bh, 8, C_CARD); s.rrect(164, by, bw, bh, 8, C_PV, fill=False)
    s.txt("TEST SPOJENÍ", 164 + bw//2, by + 16, F13, C_PV, "MC")
    s.txt("porty cíle", 164 + bw//2, by + 34, F13, C_DIM, "MC")

    s.rrect(6, 342, 308, 42, 8, C_CARD); s.rrect(6, 342, 308, 42, 8, C_GRID, fill=False)
    s.txt("RESTART ZAŘÍZENÍ", 160, 363, F13, C_GRID, "MC")
    s.txt("zatím nic", 160, 398, F13, C_DIM, "MC")
    chrome(s, 16); return s


# ============================== 13 NASTAVENI 2 ============================
ROWS = [("Jazyk","Čeština"),("Obnova","20 s"),("Zhasnout","5 min"),("Jas","100 %"),
        ("Rozsah ukazatelů","2.0 kW"),("Otočení","0"),("LED zelená do","800 W"),
        ("LED oranžová do","2000 W"),("LED bliká nad","vypnuto"),
        ("Cena kWh","6.00"),("Měna","Kč")]

def scr11():
    s = Screen()
    for i, (lb, val) in enumerate(ROWS):
        y = 46 + i * 34
        s.panel(6, y, 308, 32)
        s.txt(lb, 100, y + 16, F13, C_TXT, "MC")
        s.txt(val, 255, y + 16, F13, C_PV, "MC")
    chrome(s, 17); return s


# ============================== 14 CHYBY A VYPADKY =========================
def scr_errors():
    s = Screen()
    for i, (title, detail) in enumerate([("VÝPADEK SPOJENÍ", "14:10  selhání 3"),
                                         ("RESET POČÍTADLA", "11:40  selhání 1")]):
        y = 50 + i * 56
        s.panel(6, y, 308, 50)
        s.txt(title, 160, y + 13, F13, C_PV, "MC")
        s.txt(detail, 160, y + 36, F13, C_DIM, "MC")
    s.panel(6, 354, 308, 50, C_GRID)
    s.txt("SMAZAT SEZNAM CHYB", 160, 379, F13, C_GRID, "MC")
    chrome(s, 14); return s


# ============================== 15 UPOZORNENI =============================
def scr_alerts():
    s = Screen()
    for i, (label, val) in enumerate([("SOC pod", "20 %"), ("Teplota nad", "70 °C"),
                                      ("Výpadek po", "2 min"), ("LED při chybě", "červená"),
                                      ("Odběr ze sítě nad", "2.0 kW"), ("Po dobu", "30 min")]):
        y = 48 + i * 48
        s.panel(6, y, 308, 43)
        s.txt(label, 100, y + 21, F13, C_TXT, "MC"); s.txt(val, 255, y + 21, F13, C_PV, "MC")
    s.txt("Klepnutím na řádek změníte hodnotu", 160, 356, F13, C_DIM, "MC")
    chrome(s, 15); return s


# ============================== 16 DNES A VCERA ============================
def scr_compare():
    s = Screen()
    s.txt("DNES", 205, 50, F13, C_BATT, "MC"); s.txt("VČERA", 278, 50, F13, C_DIM, "MC")
    for i, (label, now, old, col) in enumerate([("Výroba", "4.3 kWh", "5.8 kWh", C_PV),
                                                  ("Spotřeba", "2.1 kWh", "3.0 kWh", C_LOAD),
                                                  ("Ušetřeno", "21 Kč", "29 Kč", C_BATT)]):
        y = 64 + i * 38; s.panel(6, y, 308, 34)
        s.txt(label, 62, y + 17, F13, C_TXT, "MC"); s.txt(now, 205, y + 17, F13, col, "MC"); s.txt(old, 278, y + 17, F13, C_DIM, "MC")
    s.panel(6, 182, 308, 36); s.txt("Max zátěže", 62, 200, F13, C_TXT, "MC"); s.txt("1.84 kW", 205, 200, F13, C_LOAD, "MC"); s.txt("2.10 kW", 278, 200, F13, C_DIM, "MC")
    gx, gy, gh = 6, 250, 154; s.txt("Výroba a spotřeba", 160, 230, F13, C_DIM, "MC"); s.rect(gx, gy, 308, gh, C_LINE, fill=False)
    for k in range(1, 4):
        y = gy + gh * k // 4
        for x in range(gx + 3, gx + 306, 6): s.d.point((x, y), fill=C_CARD)
    s.txt("VČERA", 85, gy + 11, F13, C_DIM, "MC"); s.txt("DNES", 235, gy + 11, F13, C_BATT, "MC")
    s.txt("5.8 / 3.0 kWh", 85, gy + 29, F13, C_TXT, "MC"); s.txt("4.3 / 2.1 kWh", 235, gy + 29, F13, C_TXT, "MC")
    for x, h, col in [(36, 84, C_PV), (88, 44, C_LOAD), (194, 64, C_PV), (246, 34, C_LOAD)]: s.rect(x, gy + gh - h - 22, 30, h, col)
    for x, text, col in [(51,"FV",C_PV),(103,"Spotř.",C_LOAD),(209,"FV",C_PV),(261,"Spotř.",C_LOAD)]: s.txt(text, x, gy + gh - 9, F13, col, "MC")
    chrome(s, 12); return s


# ============================== 17 O APLIKACI =============================
def logo(s, y):
    s.rrect(18, y, 284, 86, 10, C_PV)
    s.rrect(34, y + 18, 116, 50, 6, LOGO_DARK)
    s.rect(34, y + 72, 32, 6, C_GRID)
    s.txt("solar", 92, y + 43, F4, C_TXT, "MC")
    s.txt("assistant", 228, y + 43, F4, LOGO_DARK, "MC")

def scr12():
    s = Screen()
    logo(s, 50)
    s.txt("Cabaj Tomáš  2026", W//2, 152, F13, C_TXT, "MC")
    sy, sh, sg = 176, 50, 6
    s.tile(6,   sy,       150, sh, "Deska", "ESP32-3248S035R", C_TXT, small=True)
    s.tile(164, sy,       150, sh, "Firmware", "v2.00", C_PV, small=True)
    s.tile(6,   sy+sh+sg, 150, sh, "Flash", "4 MB", C_TXT, small=True)
    s.tile(164, sy+sh+sg, 150, sh, "RAM / NVS", "148 kB / 104", C_TXT, small=True)

    y = sy + 2 * (sh + sg) + 6
    s.panel(6, y, 308, 96)
    s.txt("Kontakt", 160, y + 14, F13, C_TXT, "MC")
    s.txt("Web  www.pcprovas.cz", 160, y + 37, F13, C_WEATH, "MC")
    s.txt("GitHub  github.com/tomas-cabaj", 160, y + 59, F13, C_WEATH, "MC")
    s.txt("E-mail  t.cabaj@email.cz", 160, y + 81, F13, C_WEATH, "MC")
    chrome(s, 18); return s


# ==========================================================================
def scr_roi():
    s = Screen()
    def button(x, y, text):
        s.rrect(x, y, 42, 34, 5, C_LINE)
        s.txt(text, x + 21, y + 17, F13, C_TXT, "MC")
    for y, label, value, step in [(46, "Počáteční investice", "120 000 Kč", 1000),
                                  (120, "Využitá energie", "6 000 kWh", 10)]:
        s.panel(6, y, 308, 70)
        s.txt(label, 92, y + 14, F13, C_DIM, "MC")
        s.txt("Krok %s" % ("1 000" if step == 1000 else str(step)), 242, y + 14, F13, C_DIM, "MC")
        button(12, y + 31, "-"); button(266, y + 31, "+")
        s.txt(value, 160, y + 51, F13, C_TXT, "MC")
    s.txt("5.00 Kč/kWh", 160, 202, F13, C_DIM, "MC")
    s.panel(6, 214, 308, 96)
    s.circle(78, 262, 40, C_LINE)
    s.d.pieslice((38, 222, 118, 302), -90, 0, fill=C_BATT)
    s.circle(78, 262, 26, C_CARD)
    s.txt("25 %", 78, 262, F13, C_TXT, "MC")
    s.txt("Vráceno", 210, 230, F13, C_DIM, "MC"); s.txt("30 000 Kč", 210, 251, F13, C_BATT, "MC")
    s.txt("Zbývá", 210, 275, F13, C_DIM, "MC"); s.txt("90 000 Kč", 210, 296, F13, C_TXT, "MC")
    s.txt("Provoz od (den / měsíc / rok)", 160, 320, F13, C_DIM, "MC")
    button(12, 332, "-"); button(266, 332, "+")
    for p, value in enumerate(["19", "09", "2024"]):
        s.txt(value, 86 + p * 74, 349, F13, C_PV if p == 0 else C_TXT, "MC")
    s.panel(6, 374, 308, 42, C_PV)
    s.txt("Odhad do splacení", 14, 381, F13, C_DIM)
    s.txt("6.0 let", 235, 400, F4, C_PV, "MC")
    chrome(s, 13)
    return s


NAMES = ["00-prehled", "01-baterie", "02-dobeh", "03-solar", "04-sit-zatez",
         "06-menic", "05-pocasi", "07-grafy", "08-teploty", "09-historie",
         "10-uspory", "11-predikce", "16-dnes-vcera", "18-navratnost",
         "14-chyby", "15-upozorneni", "12-nastaveni", "13-nastaveni2", "17-o-aplikaci"]
FUNCS = [scr0, scr1, scr2, scr3, scr4, scr6, scr5, scr7, scr8temps, scr8,
         scr9, scr11forecast, scr_compare, scr_roi, scr_errors, scr_alerts,
         scr10, scr11, scr12]

rendered = []
for name, fn in zip(NAMES, FUNCS):
    img = fn().im
    rendered.append(img)
    target = OUT + name + ".png"
    try:
        img.save(target)
        print("  ", name + ".png")
    except PermissionError:
        # Windows muze nahled drzet otevreny; nova verze se neztrati.
        # EN: Windows may keep a preview open; preserve the new version.
        img.save(OUT + name + ".new.png")
        print("  ", name + ".png ZAMKNUTO -> " + name + ".new.png")

# Přehledový list má čtyři sloupce a tolik řad, kolik vyžadují obrazovky.
cols = 4
rows = (len(NAMES) + cols - 1) // cols
sheet = Image.new("RGB", (cols * (W + 10) + 10, rows * (H + 10) + 10), (32, 32, 36))
for i, im in enumerate(rendered):
    sheet.paste(im, (10 + (i % cols) * (W + 10), 10 + (i // cols) * (H + 10)))
sheet.save(OUT + "vsechny-obrazovky.png")
print("   vsechny-obrazovky.png")
