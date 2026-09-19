# AI-CONTEXT · kontext projektu pro jazykové modely

> **CZ:** Tenhle soubor je psaný pro AI, která projekt vidí poprvé. Shrnuje,
> co projekt je, jak je poskládaný a co o něm není vidět z kódu.
> Pro lidi je [README.md](README.md) a [FEATURES.md](FEATURES.md).
>
> **EN:** This file is written for an AI seeing the project for the first
> time. It covers what the project is, how it fits together and what the code
> does not show. Humans should read [README.md](README.md) and
> [FEATURES.md](FEATURES.md).

Navazující dokumenty · Follow-up documents:

| Soubor | O čem je |
|---|---|
| [AI-FONTS.md](AI-FONTS.md) | Diakritika, smooth font VLW, proč `tCz()` a `tAs()` |
| [AI-SCREENSHOTS.md](AI-SCREENSHOTS.md) | Generování náhledů obrazovek 1:1 |
| [AI-WORKFLOW.md](AI-WORKFLOW.md) | Jak projekt bezpečně měnit a co ověřit |

---

## 1 · Co to je

**CZ:** Firmware pro ESP32 s 3,5" dotykovým displejem, který visí na zdi
a ukazuje data z fotovoltaiky. Data čte z **Solar Assistant** přes jeho REST
API (`/api/v1/metrics`, basic auth) po lokální síti. Nic neřídí, jen zobrazuje.

**EN:** Firmware for an ESP32 with a 3.5" touch display that hangs on a wall
and shows photovoltaic data. It reads from **Solar Assistant** over its REST
API (`/api/v1/metrics`, basic auth) on the local network. It controls nothing,
it only displays.

Jedna skeč, třináct obrazovek, čtyři jazyky rozhraní, všechno v jedné složce.

---

## 2 · Hardware, který nesmíš uhádnout špatně

| | |
|---|---|
| Deska | **ESP32-3248S035R** — takzvaná „Cheap Yellow Display", 3,5" varianta |
| Čip | ESP32-D0WD-V3, 4 MB flash, **bez PSRAM** |
| Displej | ST7796, 480 × 320 nativně, **v projektu otočený nastojato 320 × 480** |
| Podsvícení | **GPIO 27** (ne 21 jako u 2,8" verze) |
| Dotyk | XPT2046 **na stejné SPI sběrnici jako displej**, CS na **GPIO 33** |
| RGB LED | GPIO 4 / 16 / 17, **aktivní v nule** |
| Fotorezistor | GPIO 34 — na desce je, ale v tomhle projektu se nepoužívá |

> **CZ:** Existuje několik desek se jménem „ESP32 Yellow". Liší se úhlopříčkou,
> řadičem i pinem podsvícení. Když se netrefíš, displej zůstane černý a nic ti
> to neřekne. **Ptej se uživatele na úhlopříčku dřív, než začneš psát kód** —
> tohle mě stálo tři kola.
>
> **EN:** Several boards are sold as "ESP32 Yellow". They differ in size,
> controller and backlight pin. Guess wrong and the display stays black without
> a word of explanation. **Ask the user for the diagonal before writing code** —
> this cost me three rounds.

### Nastavení, bez kterého to nejde

1. **`User_Setup_CYD.h` se musí zkopírovat přes `User_Setup.h`** v knihovně
   TFT_eSPI. Skeč to kontroluje `#if`em a bez toho se nepřeloží.
2. V `User_Setup.h` musí být **`USE_HSPI_PORT`**. Bez něj TFT_eSPI použije
   VSPI, komunikace „projde", ale na displeji není nic.
3. Arduino IDE → **Partition Scheme: Huge APP (3 MB)**. Jinak se binárka
   nevejde — font a překlady zaberou skoro 100 kB.
4. Deska v IDE: **ESP32 Dev Module**, Serial 115200 Bd.

---

## 3 · Mapa souborů

```
SolaAssistant-TMK.ino     hlavní skeč, ~3000 řádků, všechno kromě generovaných dat
Lang.h                    GENEROVANÝ  167 řetězců × 4 jazyky
FontUi.h                  GENEROVANÝ  smooth font VLW jako PROGMEM pole, 139 glyfů
glyphs.txt                GENEROVANÝ  seznam znaků nad ASCII, vstup pro make_vlw.py
User_Setup_CYD.h          konfigurace TFT_eSPI, kopíruje se do knihovny
DejaVuSans-Bold.ttf       zdrojové písmo, volná licence

mklang.py                 generátor Lang.h + glyphs.txt   ← zdroj překladů
make_vlw.py               generátor FontUi.h z TTF
render.py                 primitiva pro náhledy, napodobuje TFT_eSPI
screens.py                kresba třinácti náhledů → images/
check_fit.py              kontrola, že se popisky vejdou ve všech jazycích
check_glyphs.py           kontrola, že každý zobrazovaný znak má glyf
check_braces.py           kontrola struktury skeče po automatickém patchi

README.md .en .de         pro lidi, tři jazyky
FEATURES.md               popis všech obrazovek a funkcí
AI-*.md                   tenhle set dokumentů
images/                   náhledy 1:1, odkazují se z README
```

> **CZ:** Soubory označené GENEROVANÝ **nikdy needituj ručně.** Uprav generátor
> a spusť ho. V hlavičce každého z nich to je napsané.
>
> **EN:** Never hand-edit the files marked GENEROVANÝ. Change the generator and
> run it. Each file says so in its header.

### Jedna složka, žádné verze vedle sebe

**CZ:** Uživatel si výslovně přeje iterovat v místě. Nezakládej
`projekt_v2`, `projekt_nove`, `*.bak` vedle sebe. Staré věci mažeme.

**EN:** The user explicitly wants to iterate in place. Do not create
`project_v2`, `project_new` or `*.bak` side by side. Old versions get deleted.

---

## 4 · Jak skeč funguje

### Tok dat

```
loop()
 ├─ tickUptime()            hlídá přetečení millis()
 ├─ ArduinoOTA.handle()
 ├─ handleTouch()           dotyk → přepnutí obrazovky nebo změna nastavení
 ├─ každých 100 ms          updateLoadLed()   aby bylo vidět blikání
 ├─ každou 1 s              updateClock() + hlavička
 └─ každých CFG_FETCH ms    fetchData() → parse JSON → pushHistory() → drawScreen()
```

`fetchData()` stáhne JSON ze Solar Assistanta a naplní globální `v_*`, `i_*`
a `w_*` proměnné. `pushHistory()` je zařadí do denního grafu a jednou za den
uzavře den do historie.

### Obrazovky

Index 0–12, přepínají se `[<] [domů] [>]` dole (`NAV_Y 432`, výška 44).
Na úvodní obrazovce se dá klepnout přímo na kteroukoli kartu.

| # | Obrazovka | Funkce kreslení |
|---|---|---|
| 0 | Přehled | `scrOverview()` |
| 1 | Baterie | `scrBattery()` |
| 2 | Doběh | `scrRuntime()` |
| 3 | Solár | `scrSolar()` |
| 4 | Síť a zátěž | `scrGridLoad()` |
| 5 | Počasí | `scrWeather()` |
| 6 | Měnič | `scrInverter()` |
| 7 | Grafy 0–24 h | `scrGraphs()` |
| 8 | Historie | `scrHistory()` — klepnutím 7 dní / 31 dní / 12 měsíců |
| 9 | Úspory | `scrSavings()` |
| 10 | Nastavení | `scrSettings()` |
| 11 | Nastavení 2 | `scrSettings2()` |
| 12 | O aplikaci | `scrAbout()` |

### Překreslování

**CZ:** Obrazovka se při obnově dat smaže a nakreslí celá znovu. Zkoušel jsem
částečné překreslování přes `setTextPadding()`, aby to neproblikávalo —
uživatel to odmítl, protože bloky pak nevypadaly stejně hezky jako na úvodní
straně. **Krátké bliknutí je přijatelné, ošklivé bloky ne.** Nevracej to zpět.

**EN:** On a data refresh the screen is cleared and redrawn whole. I tried
partial redraws via `setTextPadding()` to avoid the flicker — the user rejected
it, because the blocks then did not look as good as on the overview.
**A short flicker is acceptable, ugly blocks are not.** Do not bring it back.

### Paměť NVS (`Preferences`)

Ukládá se: uživatelská nastavení, dnešní graf po deseti minutách, denní
historie (kruhový buffer 31 dní), měsíční součty (12 položek) a půlnoční
základy počítadel. Všechno přežije restart i výpadek napájení.

---

## 5 · Výpočty, u kterých záleží na definici

### Denní hodnoty se počítají proti půlnočnímu základu

**CZ:** Měnič vrací **kumulativní** počítadla energie. Dokud jsem je bral tak,
jak přišla, ukazovala aplikace úsporu 111 Kč místo 17 Kč. Od každého počítadla
se odečítá hodnota, kterou mělo o půlnoci:

**EN:** The inverter returns **cumulative** energy counters. While I used them
as they came, the app showed a saving of 111 Kč instead of 17 Kč. Each counter
has its midnight value subtracted:

```c
float dayLoad()   { return v_load_energy    > baseLoad   ? v_load_energy    - baseLoad   : 0; }
float dayGridIn() { return v_grid_energy_in > baseGridIn ? v_grid_energy_in - baseGridIn : 0; }
```

### Úspora

```
úspora = (spotřeba za den − odběr ze sítě za den) × cena za kWh
```

**CZ:** Tedy energie, kterou dům spotřeboval, ale nemusel koupit. Uživatel to
ověřoval proti tabulce od svého dodavatele, takže tuhle definici neměň bez
důvodu.

**EN:** That is the energy the house used but did not have to buy. The user
verified it against their supplier's table, so do not change this definition
without a reason.

### Soběstačnost

```
soběstačnost = (spotřeba − odběr ze sítě) / spotřeba
```

### Rychlost nabíjení v %/hod

**CZ:** Počítá se z **výkonu a kapacity**, ne z měřených změn SOC. SOC má
rozlišení 1 %, takže měřený rozdíl skákal mezi nulou a nesmyslem:

**EN:** Computed from **power and capacity**, not from measured SOC changes.
SOC has 1 % resolution, so the measured difference jumped between zero and
nonsense:

```c
socRate = (v_batt_power / 1000.0f) / v_batt_capacity * 100.0f;
```

### Denní graf ukládá špičku, ne průměr

**CZ:** Den má 144 desetiminutových úseků. Do každého se ukládá **maximum**.
Průměr by krátké odběry schoval — odběr 1500 W trvající dvě minuty by
z desetiminutového úseku udělal 300 W a v grafu by po něm nezbyla stopa.

**EN:** The day has 144 ten-minute slots and each keeps the **peak**. An
average would hide short draws — 1500 W for two minutes averages to 300 W over
ten minutes and the chart would keep no trace of it.

Poloha vzorku v grafu: `x = PLOT_X0 + slot * PLOT_STEP` = `31 + slot * 2`,
rámeček `30 … 318`. V 9:00 vychází slot 54, tedy x = 139 — **38 % šířky.**

### millis()

**CZ:** Přeteče po 49 dnech. Rozdíly `millis() - uložený_čas` nad `uint32_t`
to přežijí samy, protože podtečení vrátí správný rozdíl. Co to nepřežije, je
absolutní doba běhu — od toho je `tickUptime()` v `loop()` a `uptimeSec()`.
Nula je platný čas, takže se první načtení hlídá příznakem `haveFetch`, ne
porovnáním `lastOkFetch == 0`.

**EN:** Wraps after 49 days. Differences of the form `millis() - stamp` over
`uint32_t` survive it, because the underflow yields the correct difference.
What does not survive is the absolute uptime — hence `tickUptime()` in `loop()`
and `uptimeSec()`. Zero is a valid timestamp, so the first fetch is tracked by
the `haveFetch` flag, not by comparing `lastOkFetch == 0`.

---

## 6 · Konvence v kódu

- **Komentáře jsou dvojjazyčné.** Česky, pak řádek `// EN:` s angličtinou.
  Komentuj **proč**, ne co — co je vidět z kódu.
- **Žádná diakritika ve zdrojácích mimo řetězce.** Komentáře jsou psané bez
  háčků a čárek („preteceni", „zavorky"), protože Arduino IDE si s kódováním
  historicky nerozumělo. Řetězce v `Lang.h` diakritiku mají.
- **Žádné zkratky v názvech**, které by čtenář musel luštit.
- Barvy jsou `#define C_*` v RGB565, nikdy ne holá čísla v kresbě.
- Formátování čísel jde přes `fmt()`, `fmt2()`, `fmtPower()`, `fmtSigned()` —
  sdílejí kruhový buffer osmi řetězců, takže **nedrž výsledek přes víc než
  osm dalších volání.**

### Svislý rozvrh obrazovky

```
  0 –  42   hlavička (název, hodiny, stav spojení)
 42 – 428   obsah
421 – 425   tečky stránkování   ← sem obsah nesmí zasahovat
432 – 476   navigační tlačítka
```

Dlaždice `statBox` kreslí hodnotu písmem 4, které je vysoké 26 px, na `y+20`.
**Dlaždice proto musí být aspoň 48 px vysoká**, jinak text přeteče přes
zaoblený rámeček. Jednou jsem tam dal 42 px a vypadalo to rozbitě.

---

## 7 · Co je v projektu vědomě nedořešené

- **Skeč nikdy neprošla kompilátorem.** Vznikala celá v editoru, ověřovaly ji
  jen skripty v `check_*.py`. Při prvním překladu čekej chyby.
- **Přístupové údaje patří výhradně do `secrets.h`**, který je v `.gitignore`.
  Pro veřejný repozitář existuje `secrets.example.h`; skutečný soubor nikdy
  necommituj.
- Arduino generuje prototypy funkcí automaticky, ale **pokud funkci předáš
  vlastní `struct`, generátor selže** hláškou `'NazevTypu' does not name
  a type`. Používej skalární parametry.
- Název `stat()` koliduje s POSIXovým `sys/stat.h`. Proto se funkce jmenuje
  `statBox()`.
