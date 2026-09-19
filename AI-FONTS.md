# AI-FONTS · diakritika, písma a formátování textu

> **CZ:** Nejvíc chyb v tomhle projektu vzniklo při vykreslování textu.
> Tenhle dokument popisuje, proč to vypadá tak, jak to vypadá.
>
> **EN:** Most bugs in this project came from drawing text. This document
> explains why it looks the way it does.

---

## 1 · Výchozí problém

**CZ:** Vestavěná písma TFT_eSPI (čísla 1, 2, 4, 6, 7) obsahují **jen ASCII.**
Žádné `č`, `ě`, `ř`, `ł`, `ü`, ani `°`, `€` nebo `²`. Když je nakreslíš,
nevykreslí se nic nebo obdélník. Rozhraní je přitom ve čtyřech jazycích,
z toho tři diakritiku potřebují.

**EN:** The built-in TFT_eSPI fonts (numbers 1, 2, 4, 6, 7) are **ASCII only.**
No `č`, `ě`, `ř`, `ł`, `ü`, and no `°`, `€` or `²` either. Draw them and you get
nothing or a rectangle. The interface is in four languages and three of them
need diacritics.

Řešení: vlastní **smooth font** ve formátu VLW, vygenerovaný z DejaVu Sans Bold
a zalinkovaný do binárky jako PROGMEM pole (`FontUi.h`).

---

## 2 · Pravidlo, na kterém všechno stojí

> **TFT_eSPI drží v jednu chvíli jen jeden smooth font. Dokud je načtený,
> `drawString()` ignoruje číslo písma, které mu předáš.**
>
> **EN: TFT_eSPI holds exactly one smooth font at a time. While it is loaded,
> `drawString()` ignores the font number you pass it.**

**CZ:** To znamená, že nejde jen tak střídat `drawString(s, x, y, 4)`
a `drawString(s, x, y)`. Musí se hlídat stav. Od toho jsou čtyři funkce:

**EN:** This means you cannot freely mix `drawString(s, x, y, 4)` with
`drawString(s, x, y)`. The state has to be tracked. Hence four functions:

```c
void czOn()  { if (!czLoaded) { tft.loadFont(FontUi); czLoaded = true;  } }
void czOff() { if (czLoaded)  { tft.unloadFont();     czLoaded = false; } }

// text s diakritikou / text with diacritics
void tCz(const char* str, int32_t x, int32_t y) {
  czOn();
  tft.drawString(str, x, y);
}

// cisla a jednotky vestavenym pismem / numbers and units in the built-in font
void tAs(const char* str, int32_t x, int32_t y, uint8_t font) {
  czOff();
  tft.drawString(str, x, y, font);
}
```

### Kdy které

| Chceš nakreslit | Použij |
|---|---|
| Cokoli z `TR(...)`, tedy překlad | `tCz()` |
| Řetězec obsahující `°`, `€`, `zł`, `Kč`, `²` | `tCz()` |
| Velké číslo, které má být výrazné (písmo 4, 6, 7) | `tAs()` |
| Čistě ASCII popisek, kde na velikosti záleží | `tAs()` |

**CZ:** Nikdy nevolej `tft.drawString()` přímo. Vždycky přes wrapper, jinak se
stav načteného fontu rozejde a text se nakreslí špatným písmem.

**EN:** Never call `tft.drawString()` directly. Always through a wrapper,
otherwise the loaded-font state drifts and the text comes out in the wrong font.

### Past: nekonečná rekurze

**CZ:** Při jedné automatizované náhradě `tft.drawString` → `tAs` se přepsalo
i tělo `tAs()` samotné a funkce začala volat sama sebe. Skeč se přeložila
a deska padala do watchdogu. `check_braces.py` to od té doby kontroluje.

**EN:** One automated replacement of `tft.drawString` → `tAs` also rewrote the
body of `tAs()` itself and the function started calling itself. The sketch
compiled and the board kept resetting. `check_braces.py` checks for it now.

---

## 3 · Míchání písem v jednom řádku

**CZ:** Teplota má být velká, ale `°C` je ve velkém písmu není. Řešení je
nakreslit hodnotu vestavěným písmem, změřit ji, a jednotku přilepit hned za ni
českým fontem. Měření musí proběhnout **ve správném stavu fontu** — proto to
přepínání kolem `textWidth()`:

**EN:** The temperature should be large, but `°C` does not exist in the large
font. The value is drawn in the built-in font, measured, and the unit is
appended right after it in the Czech font. The measurement has to happen **in
the right font state** — hence the switching around `textWidth()`:

```c
czOff();
int wv = tft.textWidth(valText, 4);   // sirka v pismu 4
czOn();
int wu = tft.textWidth(unit);         // sirka ve smooth fontu

int x0 = cx - (wv + 3 + wu) / 2;      // dvojice vycentrovana jako celek
tft.setTextDatum(TL_DATUM);
tAs(valText, x0,          cy - 29, 4);
tCz(unit,    x0 + wv + 3, cy - 24);
```

To dělá `halfGaugeU()` pro ukazatel teploty měniče a stejný postup je použitý
na stránce Počasí.

---

## 4 · Generování `Lang.h`

Zdrojem překladů je **`mklang.py`** — pole trojic `(ID, CZ, EN, PL, DE)`:

```python
 ("SELF_SUFF",  "Soběstačnost","Self-sufficiency","Samowystarcz.","Autarkie"),
```

Spuštěním `python mklang.py` vznikne:

- **`Lang.h`** — výčet `T_*`, tabulka `STRINGS[STR_N][LANG_N]` a makro
  `#define TR(id) STRINGS[id][lang]`
- **`glyphs.txt`** — seznam všech znaků nad ASCII, které se v rozhraní
  vyskytnou; vstup pro generátor fontu

### Past, která mě chytla: znaky mimo překladovou tabulku

**CZ:** Symbol `€` se na displeji kreslil jako obdélník. Důvod: měny nejsou
v překladové tabulce, jsou v poli `OPT_CURR[]` přímo ve skeči, takže je
`mklang.py` do `glyphs.txt` nezapsal a font je neobsahoval. Generátor proto
znaky, které se zobrazují jen z kódu, přidává natvrdo:

**EN:** The `€` sign rendered as a rectangle. The currencies are not in the
translation table, they live in the `OPT_CURR[]` array inside the sketch, so
`mklang.py` never wrote them to `glyphs.txt` and the font lacked them. The
generator now adds the code-only characters explicitly:

```python
# Znaky, ktere nejsou v prekladove tabulce, ale zobrazuji se z kodu:
# symboly men z pole OPT_CURR, stupen a druha mocnina u jednotek.
chars.update("Kč€ zł $ °²")
```

**Když přidáš do skeče řetězec se znakem nad ASCII, který není v překladech,
musíš ho dopsat sem.** `check_glyphs.py` to odhalí.

---

## 5 · Generování `FontUi.h`

```bash
python make_vlw.py 13 FontUi FontUi.h glyphs.txt
```

Sada znaků = ASCII 0x20–0x7E + všechno z `glyphs.txt` + `°` a `²`.
Výsledek: 139 glyfů, 44 nad ASCII, soubor má asi 80 kB.

### Formát VLW

Všechna čísla jsou **int32 big-endian**:

```
hlavicka 6 × int32:  pocet glyfu, verze(11), velikost, 0, ascent, descent
pak pro kazdy glyf 7 × int32:
    unicode, vyska, sirka, xAdvance, dY (nad uctarou), dX (leve odsazeni), 0
pak bitmapy glyfu za sebou, 1 bajt na pixel (alfa 0-255), vyska × sirka bajtu
```

### Proč zrovna 13 px

**CZ:** Původně to bylo Tahoma 14 px. DejaVu je při stejné velikosti širší,
takže po výměně za volné písmo začaly popisky přetékat z dlaždic. Snížil jsem
na 13 px a pět překladů zkrátil (`Samowystarcz.`, `Dischg. today`,
`WECHSELRICHT.`). **Když měníš velikost, musíš znovu projet
`check_fit.py`.**

**EN:** It used to be Tahoma at 14 px. DejaVu is wider at the same size, so
after the switch to a free font the labels started overflowing the tiles.
I dropped to 13 px and shortened five translations. **Change the size and you
must re-run `check_fit.py`.**

---

## 6 · Formátování čísel

```c
char fbuf[8][24];                 // kruhovy buffer osmi retezcu
uint8_t fidx = 0;
```

| Funkce | K čemu |
|---|---|
| `fmt("%.1f kWh", x)` | jedna hodnota typu float |
| `fmt2("%lu d %lu h", a, b)` | dvě celá čísla |
| `fmtPower(w)` | automaticky `W` nebo `kW` podle velikosti |
| `fmtSigned(w)` | totéž se znaménkem — `+1.18 kW` / `-620 W` |

**CZ:** Návratová hodnota ukazuje do sdíleného bufferu. Po osmi dalších
voláních se přepíše. V praxi to nevadí, protože se výsledek hned kreslí, ale
**neukládej si ho do proměnné a nepoužívej později.**

**EN:** The return value points into a shared buffer and is overwritten after
eight further calls. That is fine in practice because the result is drawn
immediately, but **do not stash it in a variable for later.**

### `%lu` a `uint32_t`

**CZ:** Na ESP32 je `uint32_t` totéž co `unsigned int`, ale `%lu` očekává
`unsigned long`. Stejná velikost, jiný typ — kompilátor na to umí zanadávat.
Hodnoty se proto přetypovávají: `(unsigned long)uptimeSec()`.

**EN:** On the ESP32 `uint32_t` is `unsigned int`, while `%lu` expects
`unsigned long`. Same size, different type — the compiler can complain. The
values are cast: `(unsigned long)uptimeSec()`.

---

## 7 · Hodiny: `strftime` tady nefunguje

**CZ:** `strftime(buf, n, "%H:%M", &t)` vracel jen dvojtečku — substituce se
neprovedla. Nezkoumal jsem proč, prostě to nahraď:

**EN:** `strftime(buf, n, "%H:%M", &t)` returned just the colon — the
substitution did not happen. I did not dig into why, just replace it:

```c
snprintf(clockStr, sizeof(clockStr), "%02d:%02d", t.tm_hour, t.tm_min);
```

---

## 8 · Postup při změně textů

1. Uprav **`mklang.py`** (ne `Lang.h`).
2. `python mklang.py` → přegeneruje `Lang.h` a `glyphs.txt`.
3. Pokud přibyl znak nad ASCII: `python make_vlw.py 13 FontUi FontUi.h glyphs.txt`.
4. `python check_glyphs.py` → každý znak má glyf?
5. `python check_fit.py` → vejde se popisek ve všech čtyřech jazycích?
6. `python check_braces.py` → nechybí překlad, sedí struktura?
7. `python screens.py` → přegenerovat náhledy.
