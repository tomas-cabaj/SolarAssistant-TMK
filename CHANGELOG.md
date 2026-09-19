# Historie změn

Tento soubor obsahuje změny od posledního vydání firmwaru. Verze `FW_VERSION`
se změní a na GitHubu vznikne release pouze na výslovný pokyn autora.

## v2.00

- Přidán denní graf teploty měniče a venkovní teploty; vzorky jsou po deseti
  minutách a přetrvají restart zařízení.
- Přidána stránka predikce úspor. Sezónní plán vychází z dodané měsíční tabulky
  jako `zátěž − použitá síť`; cena se počítá podle aktuálního nastavení za kWh.
- Blok Predikce na hlavní obrazovce uvádí měnu u obou částek.
- Měsíční součty se při začátku nového roku resetují a sčítání je saturační,
  aby se hodnoty `uint16_t` nepřetočily.
- Výpadek mezi dvěma denními vzorky je v grafech zvýrazněn červenou spojnicí.
- Přidán trvalý seznam až 16 chyb a výpadků se začátkem, počtem selhání a
  tlačítkem pro smazání seznamu.
- Přidána kontrola mezí dat z API a rozpoznání resetu kumulativních počítadel;
  neplatná data se nezapisují do historie.
- Přidána stránka upozornění: limit SOC, teploty měniče, délka výpadku API a
  barva blikání RGB LED. LED signalizuje jen právě trvající problém.
- Teplotní grafy nyní ukazují denní minimum a maximum včetně času.
- Baterie zobrazuje optimistický večerní SOC z celé zbývající predikce FVE.

## v1.00

- První veřejné vydání firmwaru SolarAssistant-TMK.
