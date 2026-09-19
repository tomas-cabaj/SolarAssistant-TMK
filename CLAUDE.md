# Pokyny pro AI v tomhle projektu

Než začneš cokoli měnit, přečti si v tomhle pořadí:

1. **[AI-CONTEXT.md](AI-CONTEXT.md)** — co projekt je, jaký je hardware,
   mapa souborů, jak skeč funguje, výpočty a konvence
2. **[AI-WORKFLOW.md](AI-WORKFLOW.md)** — jak dělat zásahy a co po nich ověřit
3. **[AI-FONTS.md](AI-FONTS.md)** — diakritika a písma; sem se dívej vždycky,
   když kreslíš text
4. **[AI-SCREENSHOTS.md](AI-SCREENSHOTS.md)** — náhledy obrazovek 1:1

## Zkrácený výtah

- **Komunikuj česky.** Komentáře v kódu dvojjazyčně: česky, pak řádek `// EN:`.
  V komentářích nepiš diakritiku, v řetězcích ano.
- **Jedna složka.** Neděláme verze vedle sebe, staré věci mažeme.
- **`Lang.h`, `FontUi.h` a `glyphs.txt` jsou generované.** Uprav generátor,
  ne je.
- **Nikdy nevolej `tft.drawString()` přímo** — jen přes `tCz()` nebo `tAs()`.
- **Po každé změně skeče:** `python check_braces.py`, `python check_fit.py`,
  `python check_glyphs.py`. Po změně rozvržení navíc `python screens.py`
  a podívej se na výsledné PNG.
- **Skeč nikdy neprošla kompilátorem.** Kontroly výše jsou jediné, co ji drží.
- **Přístupové údaje patří jen do `secrets.h`.** Je ignorovaný Gitem;
  veřejný vzor je `secrets.example.h`.
