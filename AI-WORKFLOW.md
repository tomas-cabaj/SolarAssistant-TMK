# AI-WORKFLOW · jak projekt měnit, aniž bys ho rozbil

> **CZ:** Skeč má přes 3000 řádků a **nikdy neprošla kompilátorem.** Jediné,
> co drží kvalitu, je disciplína popsaná níž. Drž se jí.
>
> **EN:** The sketch is over 3000 lines and **has never been through
> a compiler.** The only thing holding quality up is the discipline below.
> Stick to it.

---

## 1 · Jak dělat zásahy do skeče

**CZ:** Skeč se needituje po řádcích naslepo. Osvědčil se postup „najdi přesný
blok, nahraď ho celý, ověř". Na to používám krátký Python skript
s funkcí, která **tvrdě spadne, když kotva neexistuje**:

**EN:** Do not edit the sketch blindly by line number. What works is "find the
exact block, replace it whole, verify". I use a short Python script with
a helper that **fails hard when the anchor is missing**:

```python
# -*- coding: utf-8 -*-
import io

NL = chr(10)
p = r"C:\_AI\ESP32\SolaAssistant-TMK\SolaAssistant-TMK.ino"
s = io.open(p, encoding="utf-8").read()


def rep(old, new, tag):
    global s
    assert old in s, "NENALEZENO: " + tag
    s = s.replace(old, new, 1)


rep("stary blok", "novy blok", "popis zmeny")

io.open(p, "w", encoding="utf-8", newline=NL).write(s)
print("hotovo")
```

Proč zrovna takhle:

- `assert` znamená, že tichý neúspěch neexistuje. Když se kotva nenajde,
  skript spadne a soubor se nepřepíše.
- `replace(..., 1)` nahradí **jen první výskyt.** Když chceš všechny,
  napiš to explicitně a zdůvodni si to.
- `newline=NL` drží konce řádků LF i na Windows.
- Skript patří do dočasné složky, ne do projektu.

### Nahrazení celé funkce

```python
old = s[s.index("void scrHistory() {"):]
old = old[:old.index(NL + "}") + 2]      # po prvni zaviraci zavorku v levem sloupci
s = s.replace(old, new, 1)
```

Funguje to proto, že **uzavírací závorka funkce je jediná `}` na začátku
řádku.** Formátování v tomhle projektu to dodržuje.

---

## 2 · Past: escapování v shellu

> **CZ:** Opakovaně mě zdržovalo psaní skriptů přes heredoc v Bashi. Zpětná
> lomítka, `\n`, uvozovky a znakové třídy v regexech se cestou přes shell
> rozpadnou a chyba se projeví až ve výsledku.
>
> **EN:** Writing scripts through a Bash heredoc kept costing me time.
> Backslashes, `\n`, quotes and regex character classes get mangled on the way
> through the shell and the damage only shows up in the output.

Co funguje:

- **Piš skript do souboru** nástrojem na zápis souborů, pak ho spusť.
  Nepředávej dlouhý Python shellu na standardní vstup.
- Místo doslovných znaků používej konstanty: `NL = chr(10)`, `DQ = chr(34)`,
  `BS = chr(92)`, `SQ = chr(39)`.
- Když stačí ruční průchod řetězcem, **nepiš regex.** Skládání regexu
  z `chr()` konstant je nečitelné a stejně se rozbije.

---

## 3 · Povinná kontrola po každé změně

```bash
python check_braces.py
```

```bash
python check_fit.py
```

```bash
python check_glyphs.py
```

| Skript | Co hlídá |
|---|---|
| `check_braces.py` | Bilance složených závorek mimo komentáře a řetězce, každé `TR(T_*)` má protějšek v `Lang.h`, `tCz()`/`tAs()` nevolají samy sebe |
| `check_fit.py` | Popisky v dlaždicích, kartách, řádcích nastavení a pod ukazateli se vejdou **ve všech čtyřech jazycích** |
| `check_glyphs.py` | Každý znak nad ASCII, který se někde kreslí, má glyf ve fontu |

Když se měnilo rozvržení, přidej k tomu ještě:

```bash
python screens.py
```

a **podívej se na výsledné PNG.** Viz [AI-SCREENSHOTS.md](AI-SCREENSHOTS.md).

---

## 4 · Počítej rozvržení, nehádej ho

**CZ:** Většina vizuálních chyb v tomhle projektu byla aritmetická, ne
koncepční. Než něco přidáš na obrazovku, spočítej si svislý rozvrh na papíře.

**EN:** Most visual bugs here were arithmetic, not conceptual. Before adding
anything to a screen, work out the vertical layout on paper.

Limity:

```
 42        horní hranice obsahu
421 – 425  tečky stránkování — sem obsah nesmí
428        dolní hranice obsahu
```

Výšky:

| Prvek | Výška |
|---|---|
| Text ve smooth fontu 13 px | 13 px |
| Text v písmu 4 | 26 px |
| Dlaždice `statBox` | **minimálně 48 px** (hodnota se kreslí na `y+20`) |
| Pruh `bar()` | podle parametru, typicky 14 px |

Příklad — takhle vznikl blok soběstačnosti na stránce Úspory:

```
tabulka           78 – 228     3 řádky po 46 px + 6 px mezera
soběstačnost     236 – 249     popisek vlevo, procenta vpravo
pruh             254 – 268
proti včerejšku  274 – 287
nadpis grafu     307 – 320
graf             328 – 398
popisky dnů      404 – 416     ← 5 px nad tečkami na 421, těsné ale vejde se
```

### Kontrola vodorovných kolizí

**CZ:** Dva texty zarovnané k protilehlým okrajům se můžou potkat uprostřed,
a to se z kódu nepozná. Spočítej to přes šířku písma:

**EN:** Two texts aligned to opposite edges can meet in the middle, which the
code does not reveal. Compute it from the font metrics:

```python
from PIL import ImageFont
font = ImageFont.truetype("DejaVuSans-Bold.ttf", 13)

konec_vlevo  = 6   + int(font.getlength("Posledních 7 dní"))   # = 126
zacatek_vpravo = 314 - int(font.getlength("změna klepnutím"))  # = 187
# mezera 61 px, v poradku
```

**Projeď to pro všechny čtyři jazyky.** Němčina a polština bývají nejdelší,
ale ne vždycky — čeština má taky svoje.

---

## 5 · Regenerace odvozených souborů

| Změnil jsi | Spusť |
|---|---|
| překlad nebo přibyl řetězec | `python mklang.py` |
| znak nad ASCII, který nebyl | `python mklang.py` **a pak** `python make_vlw.py 13 FontUi FontUi.h glyphs.txt` |
| velikost písma | `python make_vlw.py <velikost> FontUi FontUi.h glyphs.txt` + `check_fit.py` |
| rozvržení obrazovky | `python screens.py` (a napřed sladit `screens.py` se skečí) |

---

## 6 · Co je potřeba udržovat v souladu

Tyhle věci nejsou provázané kódem, drží je jen pozornost:

1. **`screens.py` ↔ `.ino`** — kresba obrazovek, ruční kopie.
2. **`mklang.py` ↔ `.ino`** — znaky nad ASCII, které se kreslí jen z kódu,
   musí být v `chars.update(...)` v generátoru.
3. **`FEATURES.md`, `README*.md` ↔ skutečnost** — když přibude funkce, dopiš ji
   do FEATURES.md a do všech tří README. Jsou tři: `README.md` (CZ),
   `README.en.md`, `README.de.md`.
4. **Komentáře ↔ chování** — v README stálo „desetiminutové průměry" ještě
   dlouho poté, co kód ukládal špičky. Takový rozpor je horší než chybějící
   dokumentace.

---

## 7 · Práce s uživatelem

**CZ:** Zkušenosti z tohohle projektu, které stojí za předání:

**EN:** Lessons from this project worth passing on:

- **Na hardware se ptej, nehádej.** Tři kola se ztratila předpokladem, že
  „ESP32 Yellow" je 2,8" verze. Jedna otázka na úhlopříčku by to vyřešila.
- **Když uživatel něco odmítne, neprosazuj to znovu.** Částečné překreslování
  bez bliknutí bylo technicky lepší, ale vypadalo hůř a uživatel řekl ne.
- **Čísla si nech ověřit.** Špatný výpočet úspory (111 Kč místo 17 Kč) odhalila
  až tabulka od dodavatele, kterou uživatel poslal. Když hodnota vypadá
  divně, zeptej se na referenci.
- **Piš česky**, pokud uživatel píše česky. Komentáře v kódu dvojjazyčně
  CZ/EN, protože projekt míří na GitHub.
- **Diagnostiku dělej platnou.** Jednou jsem vyloučil správné zapojení dotyku
  testem, který si sám vytvořil druhou instanci `SPIClass(HSPI)` vedle
  běžící — vrátil nuly. Když test vrátí „nic", ověř nejdřív test.

---

## 8 · Otevřené položky

- [ ] **Skeč zkompilovat.** Nikdy neproběhlo. Při prvním překladu čekej chyby.
- [x] **Přístupové údaje jsou mimo skeč.** Patří do lokálního `secrets.h`,
      který `.gitignore` ignoruje. Veřejný vzor je `secrets.example.h`.
