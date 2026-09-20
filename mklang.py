# -*- coding: utf-8 -*-
"""Vygeneruje Lang.h a nahradi ceske literaly v skeci za TR(ID)."""
import io
import os

NL = chr(10)
DQ = chr(34)
D = os.path.dirname(os.path.abspath(__file__))

# id, CZ, EN, PL, DE
T = [
 ("ROI", "NÁVRATNOST", "PAYBACK", "ZWROT", "AMORTISATION"),
 ("ROI_INV", "Počáteční investice", "Initial investment", "Koszt inwestycji", "Investition"),
 ("ROI_ENERGY", "Využitá energie", "Used energy", "Zużyta energia", "Genutzte Energie"),
 ("ROI_STEP", "Krok", "Step", "Krok", "Schritt"),
 ("ROI_PAID", "Vráceno", "Recovered", "Zwrócono", "Erwirtschaftet"),
 ("ROI_START", "Provoz od (den / měsíc / rok)", "Start (day / month / year)", "Start (dzień / miesiąc / rok)", "Start (Tag / Monat / Jahr)"),
 ("ROI_EST", "Odhad do splacení", "Time to payback", "Czas do zwrotu", "Zeit bis Amortisation"),
 ("ROI_YEARS", "let", "years", "lat", "Jahre"),
 ("ROI_DONE", "Splaceno", "Paid back", "Spłacono", "Amortisiert"),
 ("ROI_DATE", "Nastavte datum", "Set start date", "Ustaw datę", "Startdatum setzen"),
 ("ROI_WAIT", "Chybí data / čas", "Need data / time", "Brak danych / czasu", "Daten / Zeit fehlen"),
 ("ROI_SAVE_ERR", "Chyba uložení", "Save failed", "Błąd zapisu", "Speichern fehlgeschlagen"),
 ("APP",        "SolarAssistant-TMK","SolarAssistant-TMK","SolarAssistant-TMK","SolarAssistant-TMK"),
 ("S_BATT",     "BATERIE","BATTERY","BATERIA","BATTERIE"),
 ("S_SOLAR",    "SOLÁR","SOLAR","SOLAR","SOLAR"),
 ("S_GRID",     "SÍŤ A ZÁTĚŽ","GRID & LOAD","SIEĆ I OBCIĄŻ.","NETZ & LAST"),
 ("S_WEATH",    "POČASÍ","WEATHER","POGODA","WETTER"),
 ("S_INV",      "MĚNIČ","INVERTER","FALOWNIK","WECHSELRICHT."),
 ("S_CHART",    "GRAFY","CHARTS","WYKRESY","DIAGRAMME"),
 ("S_SET",      "NASTAVENÍ","SETTINGS","USTAWIENIA","OPTIONEN"),
 ("S_SET2",     "NASTAVENÍ 2","SETTINGS 2","USTAWIENIA 2","OPTIONEN 2"),

 ("INVERTER",   "Měnič","Inverter","Falownik","Wechselr."),
 ("SOLARPV",    "Solární PV","Solar PV","Fotowolt.","Solar PV"),
 ("GRID",       "Síť","Grid","Sieć","Netz"),
 ("BATTERY",    "Baterie","Battery","Bateria","Batterie"),
 ("FORECAST",   "Predikce","Forecast","Prognoza","Prognose"),
 ("PRODUCED",   "Vyrobeno","Produced","Wyprodukowano","Erzeugt"),
 ("REMAINING",  "Zbývá","Remaining","Pozostało","Verbleibend"),
 ("LOAD",       "Zátěž","Load","Obciążenie","Last"),
 ("BATT_CHG",   "Baterie nabíjí","Battery charging","Bateria ładuje","Batterie lädt"),
 ("BATT_DIS",   "Baterie vybíjí","Battery discharging","Bateria rozładowuje","Batterie entlädt"),

 ("CHARGING",   "NABÍJÍ SE","CHARGING","ŁADOWANIE","LÄDT"),
 ("DISCHARGING","VYBÍJÍ SE","DISCHARGING","ROZŁADOWANIE","ENTLÄDT"),
 ("VOLTAGE",    "Napětí","Voltage","Napięcie","Spannung"),
 ("CURRENT",    "Proud","Current","Prąd","Strom"),
 ("POWER",      "Výkon","Power","Moc","Leistung"),
 ("CAPACITY",   "Kapacita","Capacity","Pojemność","Kapazität"),
 ("BATT_LEFT",  "Zbývá v baterii","Battery energy","Energia w baterii","Energie in Batterie"),

 ("PV_NOW",     "aktuální výkon FVE","current PV power","bieżąca moc PV","aktuelle PV-Leist."),
 ("DAY_PROG",   "Průběh dne","Day progress","Postęp dnia","Tagesverlauf"),
 ("PROD_TODAY", "Dnes vyrobeno","Produced today","Wyprod. dziś","Heute erzeugt"),
 ("LEFT_TODAY", "Zbývá dnes","Remaining today","Pozostało dziś","Heute verbleibend"),
 ("PV_FC",      "Predikce FVE","PV forecast","Prognoza PV","PV-Prognose"),
 ("IRRAD",      "Osvit","Irradiance","Nasłonecznienie","Einstrahlung"),
 ("PANEL_V",    "Napětí panelů","Panel voltage","Napięcie paneli","Panelspannung"),
 ("PANEL_A",    "Proud panelů","Panel current","Prąd paneli","Panelstrom"),

 ("GRID_IMP",   "odběr ze sítě","grid import","pobór z sieci","Netzbezug"),
 ("GRID_NONE",  "bez odběru","no import","brak poboru","kein Bezug"),
 ("HOUSE_LOAD", "zátěž domu","house load","obciążenie domu","Hauslast"),
 ("INV_LOAD",   "Zatížení měniče","Inverter load","Obciążenie falownika","Wechselrichterlast"),
 ("GRID_V",     "Napětí sítě","Grid voltage","Napięcie sieci","Netzspannung"),
 ("FREQ",       "Frekvence","Frequency","Częstotliwość","Frequenz"),
 ("INV_OUT",    "Výstup měniče","Inverter output","Wyjście falownika","WR-Ausgang"),
 ("OUT_HZ",     "Výstup Hz","Output Hz","Wyjście Hz","Ausgang Hz"),
 ("IMPORTED",   "Odebráno","Imported","Pobrano","Bezogen"),
 ("EXPORTED",   "Dodáno","Exported","Oddano","Eingespeist"),
 ("SELF_USE",   "Vlastní spotřeba měniče","Inverter self-consumption","Zużycie własne falownika","Eigenverbrauch WR"),

 ("CLOUDS",     "oblačnost","cloud cover","zachmurzenie","Bewölkung"),
 ("WIND",       "Vítr","Wind","Wiatr","Wind"),
 ("PANEL_T",    "Teplota panelů","Panel temp","Temp. paneli","Paneltemperatur"),

 ("INV_TEMP",   "teplota měniče","inverter temp","temp. falownika","WR-Temperatur"),
 ("POWER_USE",  "Využití výkonu","Power usage","Wykorzystanie mocy","Auslastung"),
 ("MAX_POWER",  "Max výkon","Max power","Moc maks.","Max. Leistung"),
 ("APPARENT",   "Zdánlivý výkon","Apparent power","Moc pozorna","Scheinleistung"),
 ("BUS_V",      "Bus napětí","Bus voltage","Napięcie szyny","Bus-Spannung"),
 ("MAX_CHG_A",  "Max nabíj. proud","Max charge curr.","Maks. prąd ład.","Max. Ladestrom"),
 ("ABSORPTION", "Absorpce","Absorption","Absorpcja","Absorption"),
 ("FLOAT",      "Udržovací","Float","Podtrzymanie","Erhaltung"),

 ("PV_AND_LOAD","FVE a zátěž","PV and load","PV i obciążenie","PV und Last"),
 ("PV",         "FVE","PV","PV","PV"),
 ("BATT_POWER", "Výkon baterie","Battery power","Moc baterii","Batterieleistung"),
 ("CHG_ABOVE",  "nad osou nabíjení, pod osou vybíjení","above axis charging, below discharging","nad osią ładowanie, pod rozładowanie","über Achse Laden, darunter Entladen"),
 ("SOC",        "Stav nabití","State of charge","Stan naładowania","Ladezustand"),
 ("WAIT_TIME",  "čekám na čas z internetu","waiting for internet time","czekam na czas z internetu","warte auf Internetzeit"),
 ("NEED_TIME",  "bez něj nelze sestavit denní graf","the daily chart needs it","bez niego nie ma wykresu dnia","ohne ihn kein Tagesdiagramm"),

 ("BOARD_IP",   "IP adresa desky","Board IP","IP płytki","Board-IP"),
 ("SIGNAL",     "Signál","Signal","Sygnał","Signal"),
 ("UPTIME",     "Čas běhu","Uptime","Czas pracy","Laufzeit"),
 ("FREE_MEM",   "Volná paměť","Free memory","Wolna pamięć","Freier Speicher"),
 ("REFRESH",    "Obnova","Refresh","Odświeżanie","Aktualisierung"),
 ("WIFI_NET",   "Wi-Fi síť","Wi-Fi network","Sieć Wi-Fi","WLAN"),
 ("OTA_NAME",   "OTA název","OTA name","Nazwa OTA","OTA-Name"),
 ("DUMP",       "VÝPIS HODNOT","DUMP VALUES","ZRZUT WARTOŚCI","WERTE AUSGEBEN"),
 ("TO_SERIAL",  "do Serialu","to Serial","do Serial","an Seriell"),
 ("TEST_CONN",  "TEST SPOJENÍ","LINK TEST","TEST POŁĄCZENIA","VERBINDUNGSTEST"),
 ("TGT_PORTS",  "porty cíle","target ports","porty celu","Ziel-Ports"),
 ("RESTART",    "RESTART ZAŘÍZENÍ","RESTART DEVICE","RESTART","NEUSTART"),
 ("RESTART_AGAIN", "Klepněte znovu pro restart","Tap again to restart","Dotknij ponownie, aby zrestartować","Zum Neustart erneut tippen"),
 ("NOTHING",    "zatím nic","nothing yet","jeszcze nic","noch nichts"),
 ("NOT_CONN",   "nepřipojeno","not connected","brak połączenia","nicht verbunden"),

 ("WIFI_CONN",  "Připojuji WiFi","Connecting Wi-Fi","Łączę z Wi-Fi","Verbinde WLAN"),
 ("WIFI_OK",    "WiFi OK","Wi-Fi OK","Wi-Fi OK","WLAN OK"),
 ("WIFI_ERR",   "WiFi chyba","Wi-Fi error","Błąd Wi-Fi","WLAN-Fehler"),
 ("CALIB",      "KALIBRACE DOTYKU","TOUCH CALIBRATION","KALIBRACJA DOTYKU","TOUCH-KALIBRIERUNG"),
 ("CALIB_HINT", "dotkněte se šipky v každém rohu","touch the arrow in each corner","dotknij strzałki w każdym rogu","Pfeil in jeder Ecke berühren"),
 ("OTA_RUN",    "AKTUALIZACE FIRMWARU","FIRMWARE UPDATE","AKTUALIZACJA FIRMWARE","FIRMWARE-UPDATE"),
 ("OTA_DONE",   "HOTOVO, restartuji","DONE, restarting","GOTOWE, restart","FERTIG, Neustart"),
 ("OTA_ERR",    "CHYBA AKTUALIZACE","UPDATE ERROR","BŁĄD AKTUALIZACJI","UPDATE-FEHLER"),
 ("AUTHOR",     "Cabaj Tomáš","Cabaj Tomáš","Cabaj Tomáš","Cabaj Tomáš"),

 # dynamicke hlasky v hlavicce
 ("NO_WIFI",    "bez WiFi","no Wi-Fi","brak Wi-Fi","kein WLAN"),
 ("IN_SEC",     "za %d s","in %d s","za %d s","in %d s"),
 ("OLD_MIN",    "stará %lu min","%lu min old","sprzed %lu min","%lu min alt"),
 ("FETCHING",   "stahuji data...","fetching data...","pobieram dane...","lade Daten..."),
 ("TESTING",    "testuji spojení...","testing link...","testuję połączenie...","teste Verbindung..."),
 ("SCANNING",   "skenuji síť, čekejte...","scanning network, wait...","skanuję sieć, czekaj...","scanne Netz, warten..."),
 ("DUMPED",     "vypsáno %lu s po startu","dumped %lu s after boot","zrzut %lu s po starcie","ausgegeben %lu s nach Start"),
 ("TEST_DONE",  "test proveden, výsledek v Serialu","test done, result in Serial","test wykonany, wynik w Serial","Test fertig, Ergebnis seriell"),

 # pocasi
 ("W_CLEAR",    "jasno","clear","bezchmurnie","klar"),
 ("W_PARTLY",   "polojasno","partly cloudy","częściowe zachmurzenie","heiter"),
 ("W_OVERCAST", "zataženo","overcast","pochmurno","bedeckt"),
 ("W_FOG",      "mlha","fog","mgła","Nebel"),
 ("W_DRIZZLE",  "mrholení","drizzle","mżawka","Niesel"),
 ("W_RAIN",     "déšť","rain","deszcz","Regen"),
 ("W_SNOW",     "sněžení","snow","śnieg","Schnee"),
 ("W_SHOWERS",  "přeháňky","showers","przelotne opady","Schauer"),
 ("W_SNOWSH",   "sněhové přeháňky","snow showers","opady śniegu","Schneeschauer"),
 ("W_STORM",    "bouřka","thunderstorm","burza","Gewitter"),
 ("DAY",        "den","day","dzień","Tag"),
 ("NIGHT",      "noc","night","noc","Nacht"),

 # nastaveni 2
 ("LANGUAGE",   "Jazyk","Language","Język","Sprache"),
 ("SLEEP",      "Zhasnout","Sleep","Wygaszanie","Abschalten"),
 ("BRIGHT",     "Jas","Brightness","Jasność","Helligkeit"),
 ("RANGE",      "Rozsah ukazatelů","Gauge range","Zakres wskaźników","Anzeigebereich"),
 ("LED_GREEN",  "LED zelená do","LED green to","LED zielona do","LED grün bis"),
 ("LED_ORANGE", "LED oranžová do","LED orange to","LED pomarańcz. do","LED orange bis"),
 ("ROTATION",   "Otočení","Rotation","Obrót","Drehung"),
 ("OFF",        "vypnuto","off","wyłączone","aus"),
 ("TAP_HINT",   "Klepnutím na řádek změníte hodnotu","Tap a row to change the value","Dotknij wiersza, aby zmienić","Zeile antippen zum Ändern"),
 ("VERSION",    "Verze","Version","Wersja","Version"),
 ("MINUTES",    "min","min","min","Min"),

 # denni extremy, upozorneni a doplnky
 ("USAGE",      "využití","usage","użycie","Nutzung"),
 ("PEAK_TODAY", "Špička dnes","Peak today","Szczyt dziś","Spitze heute"),
 ("MIN_TODAY",  "Minimum dnes","Minimum today","Minimum dziś","Minimum heute"),
 ("MIN_SHORT",  "Min dnes","Min today","Min dziś","Min heute"),
 ("MAX_TODAY",  "Maximum dnes","Maximum today","Maksimum dziś","Maximum heute"),
 ("ALERT_SOC",  "NÍZKÝ STAV BATERIE","LOW BATTERY","NISKI STAN BATERII","BATTERIE SCHWACH"),
 ("ALERT_TEMP", "VYSOKÁ TEPLOTA MĚNIČE","INVERTER OVERHEATING","WYSOKA TEMP. FALOWNIKA","WR ÜBERHITZT"),
 ("S_ERRORS",   "CHYBY A VÝPADKY","ERRORS & OUTAGES","BŁĘDY I AWARIE","FEHLER & AUSFÄLLE"),
 ("S_SET3",     "UPOZORNĚNÍ","ALERTS","ALERTY","WARNUNGEN"),
 ("S_COMPARE",  "DNES A VČERA","TODAY & YESTERDAY","DZIŚ I WCZORAJ","HEUTE & GESTERN"),
 ("ERR_FETCH",   "VÝPADEK SPOJENÍ","CONNECTION OUTAGE","AWARIA POŁĄCZENIA","VERBINDUNGSAUSFALL"),
 ("ERR_API",     "NEPLATNÁ DATA API","INVALID API DATA","BŁĘDNE DANE API","UNGÜLTIGE API-DATEN"),
 ("ERR_COUNTER", "RESET POČÍTADLA","COUNTER RESET","RESET LICZNIKA","ZÄHLER-RESET"),
 ("ERR_GRID",   "DLOUHÝ ODBĚR ZE SÍTĚ","LONG GRID IMPORT","DŁUGI POBÓR Z SIECI","LANGER NETZBEZUG"),
 ("NO_ERRORS",   "Bez zaznamenaných chyb","No recorded errors","Brak zapisanych błędów","Keine gespeicherten Fehler"),
 ("CLEAR_ERRORS","SMAZAT SEZNAM CHYB","CLEAR ERROR LIST","WYCZYŚĆ LISTĘ BŁĘDÓW","FEHLERLISTE LÖSCHEN"),
 ("FAILURES",    "selhání","failures","błędy","Fehler"),
 ("MIN_MAX",     "min/max čas","min/max time","min/max czas","Min/Max Zeit"),
 ("SOC_EVENING", "SOC večer","SOC evening","SOC wieczorem","SOC am Abend"),
 ("YESTERDAY",  "Včera","Yesterday","Wczoraj","Gestern"),
 ("MAX_LOAD",   "Max zátěže","Max load","Maks. obciąż.","Maximallast"),
 ("COMPARE_GRAPH", "Výroba a spotřeba","Production and consumption","Produkcja i zużycie","Erzeugung und Verbrauch"),
 ("ALERT_SOC_LIMIT", "SOC pod","SOC below","SOC poniżej","SOC unter"),
 ("ALERT_TEMP_LIMIT","Teplota nad","Temperature above","Temperatura powyżej","Temperatur über"),
 ("ALERT_OFFLINE", "Výpadek po","Outage after","Awaria po","Ausfall nach"),
 ("ERR_LED",     "LED při chybě","LED on error","LED przy błędzie","LED bei Fehler"),
 ("GRID_LIMIT",  "Odběr ze sítě nad","Grid import above","Pobór z sieci powyżej","Netzbezug über"),
 ("GRID_TIME",   "Po dobu","For","Przez","Für"),
 ("NVS_FREE",    "NVS volné položky","NVS free entries","Wolne wpisy NVS","NVS freie Einträge"),
 ("RED",         "červená","red","czerwona","rot"),
 ("BLUE",        "modrá","blue","niebieska","blau"),
 ("ORANGE",      "oranžová","orange","pomarańczowa","orange"),
 ("PURPLE",      "fialová","purple","fioletowa","lila"),
 ("TO_SLEEP",   "do uspání","to sleep","do uśpienia","bis Ruhe"),

 # nove stranky a jejich obsah
 ("S_RUNTIME",  "DOBĚH BATERIE","RUNTIME","CZAS PRACY","LAUFZEIT"),
 ("S_TEMP",     "TEPLOTY","TEMPERATURES","TEMPERATURY","TEMPERATUREN"),
 ("S_HISTORY",  "HISTORIE","HISTORY","HISTORIA","VERLAUF"),
 ("S_SAVINGS",  "ÚSPORY","SAVINGS","OSZCZĘDNOŚCI","ERSPARNIS"),
 ("S_FORECAST", "PREDIKCE ÚSPOR","SAVINGS FORECAST","PROGNOZA OSZCZĘDNOŚCI","ERSPARNISPROGNOSE"),
 ("S_ABOUT",    "O APLIKACI","ABOUT","O PROGRAMIE","ÜBER"),
 ("STATE",      "Stav","State","Stan","Status"),
 ("SUNRISE",    "Východ","Sunrise","Wschód","Aufgang"),
 ("SUNSET",     "Západ","Sunset","Zachód","Untergang"),
 ("TO_SUNSET",  "Do západu","To sunset","Do zachodu","Bis Untergang"),
 ("TO_FULL",    "Plná za","Full in","Pełna za","Voll in"),
 ("TO_EMPTY",   "Vydrží","Lasts","Wystarczy","Reicht"),
 ("AT_LOAD",    "při zátěži","at load","przy obciążeniu","bei Last"),
 ("IDLE",       "klid","idle","spoczynek","Ruhe"),
 ("MONTH",      "Měsíc","Month","Miesiąc","Monat"),
 ("YEAR",       "Rok","Year","Rok","Jahr"),
 ("SAVED",      "Ušetřeno","Saved","Zaoszcz.","Gespart"),
 ("PLAN",       "Plán","Plan","Plan","Plan"),
 ("ACTUAL",     "Skutečnost","Actual","Rzeczywistość","Tatsächlich"),
 ("PLAN_VS_ACTUAL", "Plán vs. Skutečnost","Plan vs. Actual","Plan vs. rzeczywistość","Plan vs. Ist"),
 ("YEAR_PLAN",  "Roční plán","Year plan","Plan roczny","Jahresplan"),
 ("OUT_TEMP",   "Venkovní teplota","Outside temperature","Temperatura zewn.","Außentemperatur"),
 ("TODAY",      "Dnes","Today","Dziś","Heute"),
 ("PRICE",      "Cena kWh","Price kWh","Cena kWh","Preis kWh"),
 ("CURRENCY",   "Měna","Currency","Waluta","Währung"),
 ("PRODUCTION", "Výroba","Production","Produkcja","Erzeugung"),
 ("CONSUMPT",   "Spotřeba","Consumption","Zużycie","Verbrauch"),
 ("FROM_SOLAR", "Ze solaru","From solar","Z solaru","Vom Solar"),
 ("LED_BLINK",  "LED bliká nad","LED blinks above","LED miga powyżej","LED blinkt über"),
 ("BOARD",      "Deska","Board","Płytka","Platine"),
 ("FIRMWARE",   "Firmware","Firmware","Firmware","Firmware"),
 ("LAST7",      "Posledních 7 dní","Last 7 days","Ostatnie 7 dni","Letzte 7 Tage"),
 ("NO_DATA",    "zatím bez dat","no data yet","brak danych","noch keine Daten"),
 ("DAY_LEN",    "Délka dne","Day length","Długość dnia","Taglänge"),
 ("NIGHT_LEN",  "Délka noci","Night length","Długość nocy","Nachtlänge"),
 ("CHARGED_TD",  "Nabito dnes","Charged today","Naładow. dziś","Heute geladen"),
 ("DISCH_TD",    "Vybito dnes","Dischg. today","Rozład. dziś","Heute entladen"),
 ("CHARGED",    "Nabito","Charged","Naładowano","Geladen"),
 ("DISCHARGED", "Vybito","Discharged","Rozładowano","Entladen"),
 ("CONTACT",    "Kontakt","Contact","Kontakt","Kontakt"),
 ("SELF_SUFF",  "Soběstačnost","Self-sufficiency","Samowystarcz.","Autarkie"),
 ("FROM_GRID",  "ze sítě","from grid","z sieci","aus dem Netz"),
 ("VS_YEST",    "proti včerejšku","vs yesterday","do wczoraj","ggü. gestern"),
 ("VS_LAST_M",  "proti min. měsíci","vs last month","do ub. miesiąca","ggü. Vormonat"),
 ("WEEK",       "Týden","Week","Tydzień","Woche"),
 ("DAYS31",     "31 dní","31 days","31 dni","31 Tage"),
 ("MONTHS12",   "12 měsíců","12 months","12 miesięcy","12 Monate"),
 ("PREDICTED",  "předpoklad","predicted","prognoza","Prognose"),
 ("TAP_PERIOD", "změna klepnutím","tap to change","dotknij, aby zmienić","antippen ändert"),
]

LANGS = ["CZ", "EN", "PL", "DE"]

# ---------------------------------------------------------------- Lang.h ---
L = []
L.append("// Vygenerovano skriptem mklang.py - needitovat rucne.")
L.append("// Prekladova tabulka rozhrani: cestina, anglictina, polstina, nemcina.")
L.append("")
L.append("#pragma once")
L.append("")
L.append("enum { LANG_CZ = 0, LANG_EN, LANG_PL, LANG_DE, LANG_N };")
L.append("")
L.append("extern uint8_t lang;                 // aktualni jazyk, uklada se do NVS")
L.append("#define TR(id) STRINGS[id][lang]")
L.append("")
L.append("enum {")
for t in T:
    L.append("  T_%s," % t[0])
L.append("  STR_N")
L.append("};")
L.append("")
L.append("const char* const STRINGS[STR_N][LANG_N] = {")
for t in T:
    cells = ", ".join(DQ + x.replace(DQ, chr(92) + DQ) + DQ for x in t[1:])
    L.append("  { %s },   // T_%s" % (cells, t[0]))
L.append("};")
L.append("")
L.append("const char* const LANG_NAME[LANG_N] = "
         "{ " + DQ + "Čeština" + DQ + ", " + DQ + "English" + DQ + ", "
         + DQ + "Polski" + DQ + ", " + DQ + "Deutsch" + DQ + " };")
L.append("")

io.open(os.path.join(D, "Lang.h"), "w", encoding="utf-8", newline=NL).write(NL.join(L))
print("Lang.h: %d retezcu x %d jazyky" % (len(T), len(LANGS)))

# ------------------------------------------------- sada znaku pro font ---
chars = set()
for t in T:
    for v in t[1:]:
        chars.update(v)
chars.update(LANG_NAME_EXTRA := "ČeštinaEnglishPolskiDeutsch")
# Znaky, ktere nejsou v prekladove tabulce, ale zobrazuji se z kodu:
# symboly men z pole OPT_CURR, stupen a druha mocnina u jednotek.
chars.update("Kč€ zł $ °²")
extra = sorted(c for c in chars if ord(c) > 0x7E)
io.open(os.path.join(D, "glyphs.txt"),
        "w", encoding="utf-8", newline=NL).write("".join(extra))
# vypis jen poctu, konzole ve Windows neumi vsechny znaky
print("znaku nad ASCII:", len(extra))
