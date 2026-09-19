# Kontrola změn a vydání

## Po každé změně

1. Zkontrolovat každý nový index pole, dělení nulou, přetečení a časové
   výpočty založené na `millis()`.
2. Nenechat kumulativní hodnoty přetéct: pro omezené celočíselné úložiště
   používat saturační sčítání; kalendářní součty vždy svázat s konkrétním rokem.
   Výpadek mezi dvěma platnými vzorky musí zůstat v grafu označen červeně.
   Seznam chyb smí mít jen pevně daný počet položek; nové záznamy přepisují
   nejstarší. Každý údaj API musí před zápisem projít kontrolou rozsahu.
3. Text kreslit pouze přes `tCz()` nebo `tAs()`. Překlady upravit v
   `mklang.py`, potom přegenerovat `Lang.h` a `glyphs.txt`.
4. Spustit:

   ```cmd
   py -3.13 mklang.py
   py -3.13 make_vlw.py 13 FontUi FontUi.h glyphs.txt
   py -3.13 check_braces.py
   py -3.13 check_fit.py
   py -3.13 check_glyphs.py
   py -3.13 screens.py
   ```

5. Zkontrolovat nově vytvořené PNG v `images/` a zkompilovat skeč v Arduino
   IDE s deskou **ESP32 Dev Module** a schématem oddílů **Huge APP (3 MB)**.

## Vydání firmwaru

Po výslovném pokynu autora změnit `FW_VERSION`, přidat záznam do
`CHANGELOG.md`, vytvořit Git commit, označit jej tagem verze a vytvořit
odpovídající GitHub release. Soubor `secrets.h` nikdy necommitovat.
