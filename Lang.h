// Vygenerovano skriptem mklang.py - needitovat rucne.
// Prekladova tabulka rozhrani: cestina, anglictina, polstina, nemcina.

#pragma once

enum { LANG_CZ = 0, LANG_EN, LANG_PL, LANG_DE, LANG_N };

extern uint8_t lang;                 // aktualni jazyk, uklada se do NVS
#define TR(id) STRINGS[id][lang]

enum {
  T_APP,
  T_S_BATT,
  T_S_SOLAR,
  T_S_GRID,
  T_S_WEATH,
  T_S_INV,
  T_S_CHART,
  T_S_SET,
  T_S_SET2,
  T_INVERTER,
  T_SOLARPV,
  T_GRID,
  T_BATTERY,
  T_FORECAST,
  T_PRODUCED,
  T_REMAINING,
  T_LOAD,
  T_BATT_CHG,
  T_BATT_DIS,
  T_CHARGING,
  T_DISCHARGING,
  T_VOLTAGE,
  T_CURRENT,
  T_POWER,
  T_CAPACITY,
  T_BATT_LEFT,
  T_PV_NOW,
  T_DAY_PROG,
  T_PROD_TODAY,
  T_LEFT_TODAY,
  T_PV_FC,
  T_IRRAD,
  T_PANEL_V,
  T_PANEL_A,
  T_GRID_IMP,
  T_GRID_NONE,
  T_HOUSE_LOAD,
  T_INV_LOAD,
  T_GRID_V,
  T_FREQ,
  T_INV_OUT,
  T_OUT_HZ,
  T_IMPORTED,
  T_EXPORTED,
  T_SELF_USE,
  T_CLOUDS,
  T_WIND,
  T_PANEL_T,
  T_INV_TEMP,
  T_POWER_USE,
  T_MAX_POWER,
  T_APPARENT,
  T_BUS_V,
  T_MAX_CHG_A,
  T_ABSORPTION,
  T_FLOAT,
  T_PV_AND_LOAD,
  T_PV,
  T_BATT_POWER,
  T_CHG_ABOVE,
  T_SOC,
  T_WAIT_TIME,
  T_NEED_TIME,
  T_BOARD_IP,
  T_SIGNAL,
  T_UPTIME,
  T_FREE_MEM,
  T_REFRESH,
  T_WIFI_NET,
  T_OTA_NAME,
  T_DUMP,
  T_TO_SERIAL,
  T_TEST_CONN,
  T_TGT_PORTS,
  T_NET_SCAN,
  T_NOTHING,
  T_NOT_CONN,
  T_WIFI_CONN,
  T_WIFI_OK,
  T_WIFI_ERR,
  T_CALIB,
  T_CALIB_HINT,
  T_OTA_RUN,
  T_OTA_DONE,
  T_OTA_ERR,
  T_AUTHOR,
  T_NO_WIFI,
  T_IN_SEC,
  T_OLD_MIN,
  T_FETCHING,
  T_TESTING,
  T_SCANNING,
  T_DUMPED,
  T_SCAN_DONE,
  T_TEST_DONE,
  T_W_CLEAR,
  T_W_PARTLY,
  T_W_OVERCAST,
  T_W_FOG,
  T_W_DRIZZLE,
  T_W_RAIN,
  T_W_SNOW,
  T_W_SHOWERS,
  T_W_SNOWSH,
  T_W_STORM,
  T_DAY,
  T_NIGHT,
  T_LANGUAGE,
  T_SLEEP,
  T_BRIGHT,
  T_RANGE,
  T_LED_GREEN,
  T_LED_ORANGE,
  T_ROTATION,
  T_OFF,
  T_TAP_HINT,
  T_VERSION,
  T_MINUTES,
  T_USAGE,
  T_PEAK_TODAY,
  T_MIN_TODAY,
  T_MAX_TODAY,
  T_ALERT_SOC,
  T_ALERT_TEMP,
  T_S_ERRORS,
  T_S_SET3,
  T_ERR_FETCH,
  T_ERR_API,
  T_ERR_COUNTER,
  T_NO_ERRORS,
  T_CLEAR_ERRORS,
  T_FAILURES,
  T_MIN_MAX,
  T_SOC_EVENING,
  T_ALERT_SOC_LIMIT,
  T_ALERT_TEMP_LIMIT,
  T_ALERT_OFFLINE,
  T_ERR_LED,
  T_RED,
  T_BLUE,
  T_ORANGE,
  T_PURPLE,
  T_TO_SLEEP,
  T_S_RUNTIME,
  T_S_TEMP,
  T_S_HISTORY,
  T_S_SAVINGS,
  T_S_FORECAST,
  T_S_ABOUT,
  T_STATE,
  T_SUNRISE,
  T_SUNSET,
  T_TO_SUNSET,
  T_TO_FULL,
  T_TO_EMPTY,
  T_AT_LOAD,
  T_IDLE,
  T_MONTH,
  T_YEAR,
  T_SAVED,
  T_PLAN,
  T_ACTUAL,
  T_YEAR_PLAN,
  T_OUT_TEMP,
  T_TODAY,
  T_PRICE,
  T_CURRENCY,
  T_PRODUCTION,
  T_CONSUMPT,
  T_FROM_SOLAR,
  T_LED_BLINK,
  T_BOARD,
  T_FIRMWARE,
  T_LAST7,
  T_NO_DATA,
  T_DAY_LEN,
  T_NIGHT_LEN,
  T_CHARGED_TD,
  T_DISCH_TD,
  T_CHARGED,
  T_DISCHARGED,
  T_CONTACT,
  T_SELF_SUFF,
  T_FROM_GRID,
  T_VS_YEST,
  T_VS_LAST_M,
  T_WEEK,
  T_DAYS31,
  T_MONTHS12,
  T_PREDICTED,
  T_TAP_PERIOD,
  STR_N
};

const char* const STRINGS[STR_N][LANG_N] = {
  { "Solar Assistant", "Solar Assistant", "Solar Assistant", "Solar Assistant" },   // T_APP
  { "BATERIE", "BATTERY", "BATERIA", "BATTERIE" },   // T_S_BATT
  { "SOLÁR", "SOLAR", "SOLAR", "SOLAR" },   // T_S_SOLAR
  { "SÍŤ A ZÁTĚŽ", "GRID & LOAD", "SIEĆ I OBCIĄŻ.", "NETZ & LAST" },   // T_S_GRID
  { "POČASÍ", "WEATHER", "POGODA", "WETTER" },   // T_S_WEATH
  { "MĚNIČ", "INVERTER", "FALOWNIK", "WECHSELRICHT." },   // T_S_INV
  { "GRAFY", "CHARTS", "WYKRESY", "DIAGRAMME" },   // T_S_CHART
  { "NASTAVENÍ", "SETTINGS", "USTAWIENIA", "OPTIONEN" },   // T_S_SET
  { "NASTAVENÍ 2", "SETTINGS 2", "USTAWIENIA 2", "OPTIONEN 2" },   // T_S_SET2
  { "Měnič", "Inverter", "Falownik", "Wechselr." },   // T_INVERTER
  { "Solární PV", "Solar PV", "Fotowolt.", "Solar PV" },   // T_SOLARPV
  { "Síť", "Grid", "Sieć", "Netz" },   // T_GRID
  { "Baterie", "Battery", "Bateria", "Batterie" },   // T_BATTERY
  { "Predikce", "Forecast", "Prognoza", "Prognose" },   // T_FORECAST
  { "Vyrobeno", "Produced", "Wyprodukowano", "Erzeugt" },   // T_PRODUCED
  { "Zbývá", "Remaining", "Pozostało", "Verbleibend" },   // T_REMAINING
  { "Zátěž", "Load", "Obciążenie", "Last" },   // T_LOAD
  { "Baterie nabíjí", "Battery charging", "Bateria ładuje", "Batterie lädt" },   // T_BATT_CHG
  { "Baterie vybíjí", "Battery discharging", "Bateria rozładowuje", "Batterie entlädt" },   // T_BATT_DIS
  { "NABÍJÍ SE", "CHARGING", "ŁADOWANIE", "LÄDT" },   // T_CHARGING
  { "VYBÍJÍ SE", "DISCHARGING", "ROZŁADOWANIE", "ENTLÄDT" },   // T_DISCHARGING
  { "Napětí", "Voltage", "Napięcie", "Spannung" },   // T_VOLTAGE
  { "Proud", "Current", "Prąd", "Strom" },   // T_CURRENT
  { "Výkon", "Power", "Moc", "Leistung" },   // T_POWER
  { "Kapacita", "Capacity", "Pojemność", "Kapazität" },   // T_CAPACITY
  { "Zbývá v baterii", "Battery energy", "Energia w baterii", "Energie in Batterie" },   // T_BATT_LEFT
  { "aktuální výkon FVE", "current PV power", "bieżąca moc PV", "aktuelle PV-Leist." },   // T_PV_NOW
  { "Průběh dne", "Day progress", "Postęp dnia", "Tagesverlauf" },   // T_DAY_PROG
  { "Dnes vyrobeno", "Produced today", "Wyprod. dziś", "Heute erzeugt" },   // T_PROD_TODAY
  { "Zbývá dnes", "Remaining today", "Pozostało dziś", "Heute verbleibend" },   // T_LEFT_TODAY
  { "Predikce FVE", "PV forecast", "Prognoza PV", "PV-Prognose" },   // T_PV_FC
  { "Osvit", "Irradiance", "Nasłonecznienie", "Einstrahlung" },   // T_IRRAD
  { "Napětí panelů", "Panel voltage", "Napięcie paneli", "Panelspannung" },   // T_PANEL_V
  { "Proud panelů", "Panel current", "Prąd paneli", "Panelstrom" },   // T_PANEL_A
  { "odběr ze sítě", "grid import", "pobór z sieci", "Netzbezug" },   // T_GRID_IMP
  { "bez odběru", "no import", "brak poboru", "kein Bezug" },   // T_GRID_NONE
  { "zátěž domu", "house load", "obciążenie domu", "Hauslast" },   // T_HOUSE_LOAD
  { "Zatížení měniče", "Inverter load", "Obciążenie falownika", "Wechselrichterlast" },   // T_INV_LOAD
  { "Napětí sítě", "Grid voltage", "Napięcie sieci", "Netzspannung" },   // T_GRID_V
  { "Frekvence", "Frequency", "Częstotliwość", "Frequenz" },   // T_FREQ
  { "Výstup měniče", "Inverter output", "Wyjście falownika", "WR-Ausgang" },   // T_INV_OUT
  { "Výstup Hz", "Output Hz", "Wyjście Hz", "Ausgang Hz" },   // T_OUT_HZ
  { "Odebráno", "Imported", "Pobrano", "Bezogen" },   // T_IMPORTED
  { "Dodáno", "Exported", "Oddano", "Eingespeist" },   // T_EXPORTED
  { "Vlastní spotřeba měniče", "Inverter self-consumption", "Zużycie własne falownika", "Eigenverbrauch WR" },   // T_SELF_USE
  { "oblačnost", "cloud cover", "zachmurzenie", "Bewölkung" },   // T_CLOUDS
  { "Vítr", "Wind", "Wiatr", "Wind" },   // T_WIND
  { "Teplota panelů", "Panel temp", "Temp. paneli", "Paneltemperatur" },   // T_PANEL_T
  { "teplota měniče", "inverter temp", "temp. falownika", "WR-Temperatur" },   // T_INV_TEMP
  { "Využití výkonu", "Power usage", "Wykorzystanie mocy", "Auslastung" },   // T_POWER_USE
  { "Max výkon", "Max power", "Moc maks.", "Max. Leistung" },   // T_MAX_POWER
  { "Zdánlivý výkon", "Apparent power", "Moc pozorna", "Scheinleistung" },   // T_APPARENT
  { "Bus napětí", "Bus voltage", "Napięcie szyny", "Bus-Spannung" },   // T_BUS_V
  { "Max nabíj. proud", "Max charge curr.", "Maks. prąd ład.", "Max. Ladestrom" },   // T_MAX_CHG_A
  { "Absorpce", "Absorption", "Absorpcja", "Absorption" },   // T_ABSORPTION
  { "Udržovací", "Float", "Podtrzymanie", "Erhaltung" },   // T_FLOAT
  { "FVE a zátěž", "PV and load", "PV i obciążenie", "PV und Last" },   // T_PV_AND_LOAD
  { "FVE", "PV", "PV", "PV" },   // T_PV
  { "Výkon baterie", "Battery power", "Moc baterii", "Batterieleistung" },   // T_BATT_POWER
  { "nad osou nabíjení, pod osou vybíjení", "above axis charging, below discharging", "nad osią ładowanie, pod rozładowanie", "über Achse Laden, darunter Entladen" },   // T_CHG_ABOVE
  { "Stav nabití", "State of charge", "Stan naładowania", "Ladezustand" },   // T_SOC
  { "čekám na čas z internetu", "waiting for internet time", "czekam na czas z internetu", "warte auf Internetzeit" },   // T_WAIT_TIME
  { "bez něj nelze sestavit denní graf", "the daily chart needs it", "bez niego nie ma wykresu dnia", "ohne ihn kein Tagesdiagramm" },   // T_NEED_TIME
  { "IP adresa desky", "Board IP", "IP płytki", "Board-IP" },   // T_BOARD_IP
  { "Signál", "Signal", "Sygnał", "Signal" },   // T_SIGNAL
  { "Čas běhu", "Uptime", "Czas pracy", "Laufzeit" },   // T_UPTIME
  { "Volná paměť", "Free memory", "Wolna pamięć", "Freier Speicher" },   // T_FREE_MEM
  { "Obnova", "Refresh", "Odświeżanie", "Aktualisierung" },   // T_REFRESH
  { "Wi-Fi síť", "Wi-Fi network", "Sieć Wi-Fi", "WLAN" },   // T_WIFI_NET
  { "OTA název", "OTA name", "Nazwa OTA", "OTA-Name" },   // T_OTA_NAME
  { "VÝPIS HODNOT", "DUMP VALUES", "ZRZUT WARTOŚCI", "WERTE AUSGEBEN" },   // T_DUMP
  { "do Serialu", "to Serial", "do Serial", "an Seriell" },   // T_TO_SERIAL
  { "TEST SPOJENÍ", "LINK TEST", "TEST POŁĄCZENIA", "VERBINDUNGSTEST" },   // T_TEST_CONN
  { "porty cíle", "target ports", "porty celu", "Ziel-Ports" },   // T_TGT_PORTS
  { "SKEN SÍTĚ  (trvá až minutu)", "NETWORK SCAN  (up to a minute)", "SKAN SIECI  (do minuty)", "NETZ-SCAN  (bis 1 Min)" },   // T_NET_SCAN
  { "zatím nic", "nothing yet", "jeszcze nic", "noch nichts" },   // T_NOTHING
  { "nepřipojeno", "not connected", "brak połączenia", "nicht verbunden" },   // T_NOT_CONN
  { "Připojuji WiFi", "Connecting Wi-Fi", "Łączę z Wi-Fi", "Verbinde WLAN" },   // T_WIFI_CONN
  { "WiFi OK", "Wi-Fi OK", "Wi-Fi OK", "WLAN OK" },   // T_WIFI_OK
  { "WiFi chyba", "Wi-Fi error", "Błąd Wi-Fi", "WLAN-Fehler" },   // T_WIFI_ERR
  { "KALIBRACE DOTYKU", "TOUCH CALIBRATION", "KALIBRACJA DOTYKU", "TOUCH-KALIBRIERUNG" },   // T_CALIB
  { "dotkněte se šipky v každém rohu", "touch the arrow in each corner", "dotknij strzałki w każdym rogu", "Pfeil in jeder Ecke berühren" },   // T_CALIB_HINT
  { "AKTUALIZACE FIRMWARU", "FIRMWARE UPDATE", "AKTUALIZACJA FIRMWARE", "FIRMWARE-UPDATE" },   // T_OTA_RUN
  { "HOTOVO, restartuji", "DONE, restarting", "GOTOWE, restart", "FERTIG, Neustart" },   // T_OTA_DONE
  { "CHYBA AKTUALIZACE", "UPDATE ERROR", "BŁĄD AKTUALIZACJI", "UPDATE-FEHLER" },   // T_OTA_ERR
  { "Cabaj Tomáš", "Cabaj Tomáš", "Cabaj Tomáš", "Cabaj Tomáš" },   // T_AUTHOR
  { "bez WiFi", "no Wi-Fi", "brak Wi-Fi", "kein WLAN" },   // T_NO_WIFI
  { "za %d s", "in %d s", "za %d s", "in %d s" },   // T_IN_SEC
  { "stará %lu min", "%lu min old", "sprzed %lu min", "%lu min alt" },   // T_OLD_MIN
  { "stahuji data...", "fetching data...", "pobieram dane...", "lade Daten..." },   // T_FETCHING
  { "testuji spojení...", "testing link...", "testuję połączenie...", "teste Verbindung..." },   // T_TESTING
  { "skenuji síť, čekejte...", "scanning network, wait...", "skanuję sieć, czekaj...", "scanne Netz, warten..." },   // T_SCANNING
  { "vypsáno %lu s po startu", "dumped %lu s after boot", "zrzut %lu s po starcie", "ausgegeben %lu s nach Start" },   // T_DUMPED
  { "sken hotov, nalezeno %d zařízení", "scan done, %d devices found", "skan gotowy, znaleziono %d", "Scan fertig, %d gefunden" },   // T_SCAN_DONE
  { "test proveden, výsledek v Serialu", "test done, result in Serial", "test wykonany, wynik w Serial", "Test fertig, Ergebnis seriell" },   // T_TEST_DONE
  { "jasno", "clear", "bezchmurnie", "klar" },   // T_W_CLEAR
  { "polojasno", "partly cloudy", "częściowe zachmurzenie", "heiter" },   // T_W_PARTLY
  { "zataženo", "overcast", "pochmurno", "bedeckt" },   // T_W_OVERCAST
  { "mlha", "fog", "mgła", "Nebel" },   // T_W_FOG
  { "mrholení", "drizzle", "mżawka", "Niesel" },   // T_W_DRIZZLE
  { "déšť", "rain", "deszcz", "Regen" },   // T_W_RAIN
  { "sněžení", "snow", "śnieg", "Schnee" },   // T_W_SNOW
  { "přeháňky", "showers", "przelotne opady", "Schauer" },   // T_W_SHOWERS
  { "sněhové přeháňky", "snow showers", "opady śniegu", "Schneeschauer" },   // T_W_SNOWSH
  { "bouřka", "thunderstorm", "burza", "Gewitter" },   // T_W_STORM
  { "den", "day", "dzień", "Tag" },   // T_DAY
  { "noc", "night", "noc", "Nacht" },   // T_NIGHT
  { "Jazyk", "Language", "Język", "Sprache" },   // T_LANGUAGE
  { "Zhasnout", "Sleep", "Wygaszanie", "Abschalten" },   // T_SLEEP
  { "Jas", "Brightness", "Jasność", "Helligkeit" },   // T_BRIGHT
  { "Rozsah ukazatelů", "Gauge range", "Zakres wskaźników", "Anzeigebereich" },   // T_RANGE
  { "LED zelená do", "LED green to", "LED zielona do", "LED grün bis" },   // T_LED_GREEN
  { "LED oranžová do", "LED orange to", "LED pomarańcz. do", "LED orange bis" },   // T_LED_ORANGE
  { "Otočení", "Rotation", "Obrót", "Drehung" },   // T_ROTATION
  { "vypnuto", "off", "wyłączone", "aus" },   // T_OFF
  { "Klepnutím na řádek změníte hodnotu", "Tap a row to change the value", "Dotknij wiersza, aby zmienić", "Zeile antippen zum Ändern" },   // T_TAP_HINT
  { "Verze", "Version", "Wersja", "Version" },   // T_VERSION
  { "min", "min", "min", "Min" },   // T_MINUTES
  { "využití", "usage", "użycie", "Nutzung" },   // T_USAGE
  { "Špička dnes", "Peak today", "Szczyt dziś", "Spitze heute" },   // T_PEAK_TODAY
  { "Minimum dnes", "Minimum today", "Minimum dziś", "Minimum heute" },   // T_MIN_TODAY
  { "Maximum dnes", "Maximum today", "Maksimum dziś", "Maximum heute" },   // T_MAX_TODAY
  { "NÍZKÝ STAV BATERIE", "LOW BATTERY", "NISKI STAN BATERII", "BATTERIE SCHWACH" },   // T_ALERT_SOC
  { "VYSOKÁ TEPLOTA MĚNIČE", "INVERTER OVERHEATING", "WYSOKA TEMP. FALOWNIKA", "WR ÜBERHITZT" },   // T_ALERT_TEMP
  { "CHYBY A VÝPADKY", "ERRORS & OUTAGES", "BŁĘDY I AWARIE", "FEHLER & AUSFÄLLE" },   // T_S_ERRORS
  { "UPOZORNĚNÍ", "ALERTS", "ALERTY", "WARNUNGEN" },   // T_S_SET3
  { "VÝPADEK SPOJENÍ", "CONNECTION OUTAGE", "AWARIA POŁĄCZENIA", "VERBINDUNGSAUSFALL" },   // T_ERR_FETCH
  { "NEPLATNÁ DATA API", "INVALID API DATA", "BŁĘDNE DANE API", "UNGÜLTIGE API-DATEN" },   // T_ERR_API
  { "RESET POČÍTADLA", "COUNTER RESET", "RESET LICZNIKA", "ZÄHLER-RESET" },   // T_ERR_COUNTER
  { "Bez zaznamenaných chyb", "No recorded errors", "Brak zapisanych błędów", "Keine gespeicherten Fehler" },   // T_NO_ERRORS
  { "SMAZAT SEZNAM CHYB", "CLEAR ERROR LIST", "WYCZYŚĆ LISTĘ BŁĘDÓW", "FEHLERLISTE LÖSCHEN" },   // T_CLEAR_ERRORS
  { "selhání", "failures", "błędy", "Fehler" },   // T_FAILURES
  { "min/max čas", "min/max time", "min/max czas", "Min/Max Zeit" },   // T_MIN_MAX
  { "SOC večer", "SOC evening", "SOC wieczorem", "SOC am Abend" },   // T_SOC_EVENING
  { "SOC pod", "SOC below", "SOC poniżej", "SOC unter" },   // T_ALERT_SOC_LIMIT
  { "Teplota nad", "Temperature above", "Temperatura powyżej", "Temperatur über" },   // T_ALERT_TEMP_LIMIT
  { "Výpadek po", "Outage after", "Awaria po", "Ausfall nach" },   // T_ALERT_OFFLINE
  { "LED při chybě", "LED on error", "LED przy błędzie", "LED bei Fehler" },   // T_ERR_LED
  { "červená", "red", "czerwona", "rot" },   // T_RED
  { "modrá", "blue", "niebieska", "blau" },   // T_BLUE
  { "oranžová", "orange", "pomarańczowa", "orange" },   // T_ORANGE
  { "fialová", "purple", "fioletowa", "lila" },   // T_PURPLE
  { "do uspání", "to sleep", "do uśpienia", "bis Ruhe" },   // T_TO_SLEEP
  { "DOBĚH BATERIE", "RUNTIME", "CZAS PRACY", "LAUFZEIT" },   // T_S_RUNTIME
  { "TEPLOTY", "TEMPERATURES", "TEMPERATURY", "TEMPERATUREN" },   // T_S_TEMP
  { "HISTORIE", "HISTORY", "HISTORIA", "VERLAUF" },   // T_S_HISTORY
  { "ÚSPORY", "SAVINGS", "OSZCZĘDNOŚCI", "ERSPARNIS" },   // T_S_SAVINGS
  { "PREDIKCE ÚSPOR", "SAVINGS FORECAST", "PROGNOZA OSZCZĘDNOŚCI", "ERSPARNISPROGNOSE" },   // T_S_FORECAST
  { "O APLIKACI", "ABOUT", "O PROGRAMIE", "ÜBER" },   // T_S_ABOUT
  { "Stav", "State", "Stan", "Status" },   // T_STATE
  { "Východ", "Sunrise", "Wschód", "Aufgang" },   // T_SUNRISE
  { "Západ", "Sunset", "Zachód", "Untergang" },   // T_SUNSET
  { "Do západu", "To sunset", "Do zachodu", "Bis Untergang" },   // T_TO_SUNSET
  { "Plná za", "Full in", "Pełna za", "Voll in" },   // T_TO_FULL
  { "Vydrží", "Lasts", "Wystarczy", "Reicht" },   // T_TO_EMPTY
  { "při zátěži", "at load", "przy obciążeniu", "bei Last" },   // T_AT_LOAD
  { "klid", "idle", "spoczynek", "Ruhe" },   // T_IDLE
  { "Měsíc", "Month", "Miesiąc", "Monat" },   // T_MONTH
  { "Rok", "Year", "Rok", "Jahr" },   // T_YEAR
  { "Ušetřeno", "Saved", "Zaoszcz.", "Gespart" },   // T_SAVED
  { "Plán", "Plan", "Plan", "Plan" },   // T_PLAN
  { "Skutečnost", "Actual", "Rzeczywistość", "Tatsächlich" },   // T_ACTUAL
  { "Roční plán", "Year plan", "Plan roczny", "Jahresplan" },   // T_YEAR_PLAN
  { "Venkovní teplota", "Outside temperature", "Temperatura zewn.", "Außentemperatur" },   // T_OUT_TEMP
  { "Dnes", "Today", "Dziś", "Heute" },   // T_TODAY
  { "Cena kWh", "Price kWh", "Cena kWh", "Preis kWh" },   // T_PRICE
  { "Měna", "Currency", "Waluta", "Währung" },   // T_CURRENCY
  { "Výroba", "Production", "Produkcja", "Erzeugung" },   // T_PRODUCTION
  { "Spotřeba", "Consumption", "Zużycie", "Verbrauch" },   // T_CONSUMPT
  { "Ze solaru", "From solar", "Z solaru", "Vom Solar" },   // T_FROM_SOLAR
  { "LED bliká nad", "LED blinks above", "LED miga powyżej", "LED blinkt über" },   // T_LED_BLINK
  { "Deska", "Board", "Płytka", "Platine" },   // T_BOARD
  { "Firmware", "Firmware", "Firmware", "Firmware" },   // T_FIRMWARE
  { "Posledních 7 dní", "Last 7 days", "Ostatnie 7 dni", "Letzte 7 Tage" },   // T_LAST7
  { "zatím bez dat", "no data yet", "brak danych", "noch keine Daten" },   // T_NO_DATA
  { "Délka dne", "Day length", "Długość dnia", "Taglänge" },   // T_DAY_LEN
  { "Délka noci", "Night length", "Długość nocy", "Nachtlänge" },   // T_NIGHT_LEN
  { "Nabito dnes", "Charged today", "Naładow. dziś", "Heute geladen" },   // T_CHARGED_TD
  { "Vybito dnes", "Dischg. today", "Rozład. dziś", "Heute entladen" },   // T_DISCH_TD
  { "Nabito", "Charged", "Naładowano", "Geladen" },   // T_CHARGED
  { "Vybito", "Discharged", "Rozładowano", "Entladen" },   // T_DISCHARGED
  { "Kontakt", "Contact", "Kontakt", "Kontakt" },   // T_CONTACT
  { "Soběstačnost", "Self-sufficiency", "Samowystarcz.", "Autarkie" },   // T_SELF_SUFF
  { "ze sítě", "from grid", "z sieci", "aus dem Netz" },   // T_FROM_GRID
  { "proti včerejšku", "vs yesterday", "do wczoraj", "ggü. gestern" },   // T_VS_YEST
  { "proti min. měsíci", "vs last month", "do ub. miesiąca", "ggü. Vormonat" },   // T_VS_LAST_M
  { "Týden", "Week", "Tydzień", "Woche" },   // T_WEEK
  { "31 dní", "31 days", "31 dni", "31 Tage" },   // T_DAYS31
  { "12 měsíců", "12 months", "12 miesięcy", "12 Monate" },   // T_MONTHS12
  { "předpoklad", "predicted", "prognoza", "Prognose" },   // T_PREDICTED
  { "změna klepnutím", "tap to change", "dotknij, aby zmienić", "antippen ändert" },   // T_TAP_PERIOD
};

const char* const LANG_NAME[LANG_N] = { "Čeština", "English", "Polski", "Deutsch" };
