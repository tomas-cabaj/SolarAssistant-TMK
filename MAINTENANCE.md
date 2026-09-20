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
   py -3.13 make_ui_icons.py
   py -3.13 screens.py
   ```

5. Zkontrolovat nově vytvořené PNG v `images/` a zkompilovat skeč v Arduino
   IDE s deskou **ESP32 Dev Module** a schématem oddílů **Minimal SPIFFS
   (1.9MB APP with OTA)**.

## Společná grafika a počasí

- Karty vykreslovat přes `uiPanel` / `uiStat` v `UiStyle.h`; hlavní obrazovka
  je grafický vzor. Při změně synchronizovat `render.py` a `screens.py`.
- Zdroj sady ikon je `assets/ui-icons-source.png`. Neupravovat motivy ručně;
  `make_ui_icons.py` z něj vytváří `UiIcons.h` v RGB565 a kontrolní náhledy.
  Pole zůstávají v `PROGMEM`, aby dlouhý provoz nezatěžoval dynamickou RAM.
- Názvy a vykreslování stránek jsou společně v `screenDefinitions`.
  Pořadí měnit pouze přes pojmenované hodnoty `ScreenId`; počet hlídá
  `static_assert`.
- Animace ukazatelů smí interpolovat pouze zobrazení. Nesmí přepisovat živé
  API hodnoty a nesmí používat blokující čekání, aby pokračovalo OTA i dotyk.
- Vlastní typy nepoužívat v signaturách funkcí přímo v `.ino`, pokud nejsou
  deklarované v hlavičce; Arduino může automatický prototyp vložit před typ.
- Kurzor grafu vybírá pouze existující položku `dHas`; v mezeře po výpadku se
  použije nejbližší platný vzorek.
- Ikony a slovní popis používají `weatherKind`. Procenta den/noc musí dát
  dohromady 100; neznámý čas se nesmí zobrazovat jako polární noc.
- Po úpravě kontrolovat všech 19 náhledů a varianty počasí v `check_ui.py`.

## Návratnost FVE – trvalé údaje

- Ruční údaje jsou oddělené od API a historie, v jediném verzovaném NVS
  záznamu `roi1`. Zápis probíhá pouze při změně hodnoty; ne při vykreslování.
- Investice a celé kWh mají rozsah 0–9 999 999. Výpočet úspor používá
  `double`, aktuální cenu a měnu z Nastavení. Změna ceny přecení celou energii.
- Datum musí být platné (2000–2199). Odhad vyžaduje čas, kladnou investici,
  energii a uplynulý den; nula, budoucí datum a chybějící čas nesmějí způsobit
  dělení nulou. Splacená investice má samostatný stav.
- Při změnách zachovat `static_assert` pro kalendář a saturaci a kontrolu
  šířky nejdelších hodnot ve všech jazycích v `check_fit.py`.
- Na desce ověřit uložení po odpojení napájení, ruční opravu data a změnu ceny.

## Vydání firmwaru – postup

Po výslovném pokynu autora změnit `FW_VERSION`, přidat záznam do
`CHANGELOG.md`, vytvořit Git commit, označit jej tagem verze a vytvořit
odpovídající GitHub release. Soubor `secrets.h` nikdy necommitovat.
