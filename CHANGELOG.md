# Historie změn

Tento soubor obsahuje změny od posledního vydání firmwaru. Verze `FW_VERSION`
se změní a na GitHubu vznikne release pouze na výslovný pokyn autora.

## v2.03

- Vysoké napětí FV je pouze stavová informace: při překročení nastaveného
  prahu se zobrazí ikona na hlavní stránce. Nezapisuje se jako chyba,
  nespouští blikání LED ani červené zvýraznění.
- Komentáře firmwaru byly aktualizovány a doplněny česko-anglické popisy
  hlavních funkcí, smyček a částí programu.

- Hlavní přehled pod ukazatelem predikce zobrazuje roční úsporu ze stránky Úspory.
- Teplotní grafy si uchovávají včerejší měnič a venkovní teplotu po 1 °C jako
  úsporné osmibitové vzorky a kreslí je šedou linkou.
- NVS ukládání je sloučené do verzovaných bloků pro denní historii, souhrny,
  nastavení, chyby a včerejší data. Staré klíče zůstávají jako bezpečný fallback,
  takže migrace nemaže žádná uložená data.
- Animace půlkruhových ukazatelů je vypnutá; po načtení dat proběhne jen jedno
  překreslení bez několikanásobného blikání.
- Ze stránky Nastavení odstraněn pomalý sken sítě včetně tlačítka a obsluhy;
  restart zařízení nyní zabírá celý spodní řádek.
- Odhad návratnosti je na jednom vodorovném řádku a je centrovaný jako celek.
- Nadpisy hodnotových bloků jsou šedé a zarovnané vlevo; jejich hodnoty zůstávají
  uprostřed.
- Datum „Provoz od“ na stránce Návratnost používá správné centrování, takže rok
  už nepřesahuje do tlačítka `+`.
- Opraveno pořadí bajtů bitmap RGB565; ikony už nemají růžové pozadí.
- Navigace už nekreslí bílé vodorovné linky nad tlačítky.
- Číselné hodnoty i oblouky ukazatelů se po načtení aktualizují najednou.
- Blok odhadu návratnosti je nižší; nadpis je vlevo a odhad je velkým fontem
  uprostřed.
- Hodnoty investice, energie a finanční souhrny používají mezeru jako oddělovač
  tisíců, například `10 000 Kč`.
- Graf Historie má vlevo svislou stupnici v kWh a pomocné vodorovné čáry.
- Částka v bloku „Ušetřeno“ na stránce Historie používá velký font.
- Graf Úspory má svislou stupnici, pomocné čáry a po klepnutí zobrazí denní hodnotu.
- Graf Predikce úspor má svislou stupnici v Kč a po klepnutí zobrazí vybraný měsíc,
  plán i skutečnost.
- Stránky mají tematickou ikonku v záhlaví; na hlavním přehledu zůstává volné místo
  pro název SolarAssistant-TMK.
- Půlkruhové ukazatele jsou kresleny souvislými vyplněnými segmenty bez
  prázdných pixelů; jejich aktualizace nemění živé hodnoty historie a upozornění.
- Hlavní přehled používá přesné bitmapové motivy z dodané sady 4×4 místo
  ručně překreslených ikon. Bitmapy RGB565 zůstávají ve flash paměti.
- Spodní navigace má nový tmavý plastický styl, jemný lesk, výrazné bílé
  šipky a bitmapovou ikonu domu; aktivní tlačítko zůstává žlutě zvýrazněné.
- Hodnoty v dlaždicích jsou vystředěné; upraveno zarovnání stránek Predikce,
  Dnes a včera, Upozornění, O aplikaci a Návratnost.
- Dnes a včera nyní seskupuje sloupce FVE a spotřeby podle dne. Sloupce Dnes
  a Včera jsou odsazeny od okraje a popisek maxima je zkrácen na „Max zátěže“.
- Na O aplikaci se NVS zobrazuje společně s volnou RAM. Kontakt již neobsahuje
  technický řádek NVS.
- Nastavení ukazuje podrobný čas běhu a přidává dvoukrokové tlačítko restartu.
- Úvodní kontaktní stránka zůstává během připojování viditelná; mění se pouze
  spodní stavový blok Wi-Fi a načítání dat.
- Stránky jsou seřazeny podle toku informací; Nastavení a O aplikaci jsou na
  konci. Dotykové odkazy používají pojmenované indexy místo čísel.
- Hlavička úvodní strany používá název `SolarAssistant-TMK`; místo odpočtu
  obnovy zobrazuje vpravo aktuální čas.
- Opravena kompatibilita `GaugeMetric` s automatickými prototypy Arduino IDE;
  pomocné signatury používají základní typ `uint8_t`.
- Klepnutí do čárových grafů výkonu, SOC a teplot vybere nejbližší uložený
  desetiminutový vzorek a zobrazí svislý kurzor, čas a hodnotu.

- Počasí má větší vektorové ikony podle stejného kódu jako slovní popis:
  slunce/měsíc, oblačnost, mlha, mrholení, déšť, sníh, přeháňky a bouřka.
- Doplněn barevný poměr dne a noci s procenty; zachovány časy i délky.
  Neznámý čas má neutrální ukazatel, polární den/noc plných 100/0 %.
- Sjednoceny karty podle hlavní obrazovky: rádius, neutrální obrys, světlé
  popisky a barevné hodnoty. Významové barvy tlačítek a upozornění zůstávají.
- Refaktoring společného vykreslování do UiStyle.h, ikon do WeatherIcons.h
  a jedné tabulky názvů a vykreslení obrazovek. Bez změn API či formátu NVS.
- Hlavička měří název, hodiny a stav; při nedostatku místa použije dva řádky,
  aby se dlouhé názvy a překlady nepřekrývaly.

- Přidána samostatná stránka Návratnost: ruční investice, využitá energie,
  datum provozu, prstencový graf a zbývající doba do splacení. Výpočet používá
  aktuální cenu z Nastavení. Hodnoty se ukládají do NVS při změně; chyba
  zápisu je viditelná. Tlačítka mají volitelný krok 1 až 10 000.

- Přidána stránka Dnes a včera: živé porovnání výroby, spotřeby, úspory,
  maxima zátěže a teplotních extrémů, včetně sloupcového grafu FVE a zátěže.
- Do grafu SOC přidána přerušovaná optimistická predikce do západu slunce.
- Přidán nastavitelný limit dlouhého odběru ze sítě; výchozí je 2,0 kW po
  30 minutách a problém se zapisuje do historie chyb.
- Historie výpadků nyní u uzavřeného výpadku ukládá začátek, konec, délku a
  počet selhání.
- Stránka O aplikaci ukazuje kapacitu flash a počet volných položek NVS.
- Opravena prohozená zelená a modrá složka RGB LED na desce.
- Opraven spodní řádek stránky Baterie: „Min dnes“ a optimistický večerní SOC
  jsou zarovnány na jednu řádku podle změřené šířky hodnot, takže se nepřekrývají.
- Součty Měsíc a Rok na stránce Úspory nyní průběžně zahrnují i dnešní data;
  do NVS se dnešek stále uloží až při uzavření dne, takže se nezapočítá dvakrát.
- Historie 12 měsíců nyní do posledního sloupce a souhrnných bloků přidává i
  průběžná data aktuálního měsíce.
- Firmware a volná paměť byly odstraněny ze stránky Nastavení; údaje zůstávají
  na stránce O aplikaci spolu s kapacitou flash a volnými položkami NVS.

## v2.02

- Datum pořízení bateriové skupiny je vycentrované mezi názvem skupiny a cykly.
- Každá skupina zobrazuje vlastní odhad roku výměny.
- Přidána ovládací tlačítka `-1/+1`, přepínač kroku `1x/10x` a `-10/+10`.
- Opraveno rozložení stránky Měnič, aby min/max nepřekrýval navigační tečky.
- Do grafu SOC přidána modrá křivka napětí baterie s vlastní stupnicí.

## v2.01

- Hodnota vybraného měsíce na stránce Predikce úspor se zobrazuje uvnitř grafu,
  takže už nepřepisuje jeho nadpis ani horní měřítko.
- Stránky dostaly tematické ikonky v záhlaví, pokud je pro ně dost místa.

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
