# AI-SCREENSHOTS · generování náhledů obrazovek 1:1

> **CZ:** Displej máš jen jeden a je na zdi. Náhledy v `images/` vznikají
> v Pythonu, aby šlo rozvržení kontrolovat bez nahrávání firmwaru.
>
> **EN:** There is only one display and it hangs on a wall. The previews in
> `images/` are produced in Python so the layout can be checked without
> flashing the firmware.

---

## 1 · K čemu to je

Obrázky slouží dvěma věcem:

1. **Dokumentace** — README a FEATURES je ukazují, aby bylo vidět, jak to
   vypadá, bez zapojování desky.
2. **Kontrola rozvržení** — vykreslení odhalí kolize, které z čísel v kódu
   nejsou vidět.

> **CZ:** Tohle není teorie. Renderer mi našel tři skutečné chyby v kódu:
> špatně zarovnané záhlaví na stránce Úspory, dvoupixelovou mezeru mezi
> „Výroba" a „Spotřeba" a špatný popisek „Nabito dnes" v Historii. Naposledy
> odhalil, že se na Historii překrývalo měřítko v kWh s nápovědou
> „změna klepnutím" — obojí bylo zarovnané doprava na x = 314.
>
> **EN:** This is not theory. The renderer found three real bugs in the code:
> a misaligned header on the Savings screen, a two-pixel gap between
> "Výroba" and "Spotřeba", and a wrong "Charged today" label in History. Most
> recently it caught the kWh scale overlapping the "tap to change" hint on
> History — both were right-aligned at x = 314.

---

## 2 · Jak to funguje

Dva soubory:

| | |
|---|---|
| **`render.py`** | Napodobuje primitiva TFT_eSPI nad PIL. Definuje třídu `Screen`, barvy a ikony. Sám nic nekreslí. |
| **`screens.py`** | Pro každou obrazovku funkce `scr0()` … `scr12()`, která volá stejná primitiva ve stejném pořadí jako skeč. Spuštěním vzniknou PNG. |

```bash
python screens.py
```

Výstup: `images/00-prehled.png` … `images/12-o-aplikaci.png` plus přehledový
list `images/vsechny-obrazovky.png` (mřížka 4 × 4).

### Proč to sedí 1:1

- Plátno je **320 × 480**, stejně jako displej nastojato.
- Barvy se převádějí z **RGB565**, stejných konstant jako ve skeči:
  ```python
  def c565(v):
      r = (v >> 11) & 0x1F
      g = (v >> 5)  & 0x3F
      b = v & 0x1F
      return (r * 255 // 31, g * 255 // 63, b * 255 // 31)

  C_PV = c565(0xFD20)      # ve skeci #define C_PV 0xFD20
  ```
- Text se kreslí **stejným TTF**, ze kterého je vygenerovaný font v desce:
  `DejaVuSans-Bold.ttf`.
- Zarovnání `TL / TR / MC / MR` odpovídá `*_DATUM` v TFT_eSPI přes mapu
  `ANCH = {"TL": "lt", "TR": "rt", "MC": "mm", "MR": "rm"}`.

### Velikosti písem

| Ve skeči | V rendereru | Poznámka |
|---|---|---|
| `FontUi` (smooth) | `F13` = DejaVu 13 px | přesná shoda |
| písmo 2 | `F2` = 12 px | přibližné |
| písmo 4 | `F4` = 20 px | přibližné |
| písmo 6 | `F6` = 34 px | přibližné |
| písmo 7 | `F7` = 36 px | sedmisegmentové, tvarem se liší |

> **CZ:** Smooth font sedí přesně, protože je to doslova stejný soubor.
> Vestavěná písma jsou **aproximace** — mají jiné tvary a rozestupy. Pro
> kontrolu rozvržení to stačí, pro kontrolu šířky na pixel přesně ne.
> Když je rozhodující šířka popisku, použij `check_fit.py`, ne obrázek.
>
> **EN:** The smooth font matches exactly because it is literally the same
> file. The built-in fonts are **approximations** with different shapes and
> spacing. Good enough for layout, not for pixel-exact width. When the label
> width decides, use `check_fit.py`, not the picture.

---

## 3 · Dostupná primitiva

```python
s = Screen()

s.rect(x, y, w, h, col, fill=True)          # drawRect / fillRect
s.rrect(x, y, w, h, r, col, fill=True)      # drawRoundRect / fillRoundRect
s.line(x0, y0, x1, y1, col)
s.circle(cx, cy, r, col, fill=True)
s.tri(x0, y0, x1, y1, x2, y2, col)
s.txt(text, x, y, font=F13, col=C_TXT, datum="TL")

s.arc(cx, cy, rin, rout, a0, a1, col)       # odpovida arcRing()
s.ticks(cx, cy, r)                          # gaugeTicks()
s.gauge(cx, cy, r, th, val, mx, col, vtext, label)   # halfGauge()
s.bar(x, y, w, h, frac, col)                # bar()
s.card(x, y, w, h, icon, icol, title, l1, l2)        # card()
s.tile(x, y, w, h, label, value, col, small=False)   # statBox / statBoxSmall
```

Ikony jsou funkce `ico_inverter`, `ico_panel`, `ico_pylon`, `ico_battery`,
`ico_house`, `ico_bolt`, `ico_clock`, `ico_sun`, `ico_cloud` — berou
`(screen, x, y, color)`.

> **CZ:** `small=True` u `tile()` odpovídá `statBoxSmall()` ve skeči: hodnota
> se kreslí **českým fontem místo písma 4.** Používá se všude, kde hodnota
> obsahuje diakritiku nebo měnu — třeba „175 Kč". Když to spleteš, obrázek
> ukáže text, který se na desce nevykreslí.
>
> **EN:** `small=True` on `tile()` matches `statBoxSmall()`: the value is drawn
> **in the Czech font instead of font 4.** Used wherever the value contains
> diacritics or a currency — "175 Kč" for instance. Get it wrong and the
> picture shows text that the board will not render.

---

## 4 · Fiktivní data

Nahoře v `screens.py` jsou konstanty (`soc`, `pv_power`, `batt_v`, `DAYS`, …).
Jsou vymyšlené, ale **realistické** — odpovídají tomu, co měnič skutečně vrací.
Nesmyslné hodnoty by schovaly problémy s šířkou.

Funkce `chrome(s, idx)` dokreslí hlavičku, tečky stránkování a navigační
tlačítka, které jsou na každé obrazovce stejné.

---

## 5 · Povinnost: renderer se musí držet skeče

> **CZ:** Renderer je **ruční kopie** kresby ze skeče, nic se negeneruje
> automaticky. Když změníš rozvržení v `.ino`, **musíš stejnou změnu udělat
> v `screens.py`**, jinak obrázky lžou. Lhaní obrázků je horší než žádné
> obrázky, protože podle nich pak kontroluješ.
>
> **EN:** The renderer is a **hand copy** of the sketch's drawing code, nothing
> is generated. Change the layout in the `.ino` and **you must make the same
> change in `screens.py`**, otherwise the images lie. Lying images are worse
> than no images, because you then verify against them.

Postup po každé změně rozvržení:

1. Uprav `.ino`.
2. Uprav odpovídající `scrN()` v `screens.py` — stejná čísla, stejné pořadí.
3. `python screens.py`
4. Podívej se na výsledné PNG. Ne jen na to, že skript doběhl.

### Na co se v obrázku dívat

- Nepřekrývají se dva texty zarovnané k témuž okraji?
- Nezasahuje obsah pod y = 421, kde jsou tečky stránkování?
- Nepřetéká hodnota z dlaždice přes zaoblený rámeček?
- Nesplývají dva šedé popisky vedle sebe? (Na stránce Úspory se to řešilo
  tak, že záhlaví sloupců dostalo barvu svého sloupce.)
