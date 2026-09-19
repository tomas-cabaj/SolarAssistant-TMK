/* ===========================================================================
 *  SolarAssistant-TMK  -  graficky monitor fotovoltaiky
 *                         graphical photovoltaic monitor
 *
 *  Deska / Board : ESP32-3248S035R  (3,5" CYD, radic ST7796)
 *  Displej       : 320 x 480 nastojato / 320 x 480 portrait
 *  Autor / Author: Cabaj Tomas, 2026        github.com/tomas-cabaj
 *  Licence       : MIT, viz LICENSE / see LICENSE
 *
 *  ---------------------------------------------------------------------
 *  CZ: Cte data ze Solar Assistant pres jeho REST API a zobrazuje je na
 *      sedmnacti obrazovkach. Prepina se tlacitky dole: [<] [domu] [>],
 *      na uvodni strance se da kliknout primo na kterykoli blok.
 *
 *  EN: Reads data from Solar Assistant over its REST API and shows it on
 *      seventeen screens. Switched by the buttons at the bottom:
 *      [<] [home] [>]; on the overview any block can be tapped directly.
 *  ---------------------------------------------------------------------
 *
 *   0  PREHLED / OVERVIEW     karty, predikce, ukazatele
 *   1  BATERIE / BATTERY      SOC, napeti, proud, kapacita
 *   2  DOBEH   / RUNTIME      za jak dlouho bude plna nebo prazdna
 *   3  SOLAR   / SOLAR        vykon, prubeh dne, panely
 *   4  SIT     / GRID+LOAD    odber, zatez, frekvence, energie
 *   5  POCASI  / WEATHER      oblacnost, teplota, vychod a zapad slunce
 *   6  MENIC   / INVERTER     teplota, vykony, nabijeci napeti
 *   7  GRAFY   / CHARTS       dnesni den 0-24 h
 *   8  TEPLOTY / TEMPERATURES denni grafy teplot
 *   9  HISTORIE/ HISTORY      poslednich 7 dni
 *  10  USPORY  / SAVINGS      den, mesic, rok a kolik to usetrilo
 *  11  PREDIKCE / FORECAST    planovane a skutecne uspory
 *  12  NASTAVENI / SETTINGS   stav site a diagnosticke nastroje
 *  13  NASTAVENI 2 / SETTINGS 2  jazyk a uzivatelske volby
 *  14  CHYBY / ERRORS         zaznam chyb a vypadku
 *  15  UPOZORNENI / ALERTS    limity a LED chyb
 *  16  O APLIKACI / ABOUT     verze, deska, kontakt
 *
 *  ---------------------------------------------------------------------
 *  CZ: Rozhrani je ve ctyrech jazycich (CZ/EN/PL/DE), tabulka je v Lang.h.
 *      Diakritiku umoznuje vlastni smooth font FontUi.h, protoze vestavena
 *      pisma TFT_eSPI obsahuji jen ASCII. Cisla zustavaji na vestavenych
 *      pismech, kde diakritika neni potreba.
 *
 *      Nastaveni, denni historie i mesicni souhrny se ukladaji do NVS,
 *      takze prezijou restart i odpojeni napajeni.
 *
 *  EN: The interface comes in four languages (CZ/EN/PL/DE), the table
 *      lives in Lang.h. Diacritics come from the custom smooth font
 *      FontUi.h, because the built-in TFT_eSPI fonts are ASCII only.
 *      Numbers stay on the built-in fonts, where diacritics are not needed.
 *
 *      Settings, daily history and monthly totals are stored in NVS, so
 *      they survive a reboot and a power cut.
 *  ---------------------------------------------------------------------
 *
 *  Knihovny / Libraries : TFT_eSPI (Bodmer), ArduinoJson (6 nebo 7)
 *  Deska v IDE / Board  : ESP32 Dev Module
 *  Partition Scheme     : Huge APP (3MB)   <-- jinak se to nevejde
 *  Serial               : 115200 Bd
 *
 *  CZ: Pred prekladem zkopirujte User_Setup_CYD.h pres User_Setup.h
 *      v knihovne TFT_eSPI. Skec to kontroluje a bez toho se neprelozi.
 *  EN: Before building, copy User_Setup_CYD.h over User_Setup.h inside
 *      the TFT_eSPI library. The sketch checks this and will not compile
 *      without it.
 * ===========================================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoOTA.h>
#include <time.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include "FontUi.h"
#include "Lang.h"
#include "secrets.h"

#if !defined(TFT_BL) || (TFT_BL != 27)
  #error "Stary User_Setup.h - zkopirujte aktualni User_Setup_CYD.h pres User_Setup.h."
#endif

// ================= OTA (aktualizace pres WiFi / firmware over Wi-Fi) ==============================
// V Arduino IDE se deska objevi v Tools > Port jako sitovy port.
// EN: In Arduino IDE the board shows up under Tools > Port as a network port.
// Heslo nechte prazdne pro aktualizaci bez hesla.
// EN: Leave the password empty for updates without one.
#define FW_VERSION   "2.00"

// ================= NASTAVENI / SETTINGS ================================================
// Vychozi hodnoty. Vse nize se da zmenit na strance NASTAVENI 2 a uklada
// EN: Defaults. Everything below can be changed on the SETTINGS 2 screen and is
// se do NVS, takze prezije restart. / in NVS, so it survives a reboot.
// pod kolik procent hlasit nizky stav baterie
// EN: below how many percent to report a low battery
// nad kolik stupnu hlasit prehrivani menice
// EN: above how many degrees to report inverter overheating
// po kolika neuspesnych nactenich restartovat
// EN: how many failed fetches before rebooting
#define FAIL_REBOOT   30

// po jake dobe bez novych dat je hlasit jako stara
// EN: how long without new data before reporting it as stale

// cas z internetu; retezec pasma resi i prechod na letni cas
// EN: time from the internet; the zone string handles daylight saving too
// Poloha pro vypocet vychodu a zapadu slunce. Vychozi hodnoty jsou Praha,
// EN: Position for the sunrise and sunset calculation. Defaults to Prague,
// upravte podle mista instalace (kladna sirka = sever, delka = vychod).
// EN: adjust to the installation site (positive latitude = north, longitude = east).
#define GEO_LAT     50.08f
#define GEO_LON     14.44f

#define NTP_TZ      "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.google.com"
#define SCR_W         320
#define SCR_H         480

// ---- RGB LED na desce: signalizace zateze / on-board load LED ---------------------------------
// do CFG_GREEN      zelena   - mala zatez
// EN: up to CFG_GREEN   green    - light load
// do CFG_ORANGE     oranzova - stredni zatez
// EN: up to CFG_ORANGE  orange   - medium load
// nad CFG_ORANGE    cervena  - velka zatez
// EN: above CFG_ORANGE  red      - heavy load
// bez dat                modra    - neni spojeni se Solar Assistant
// EN: no data                blue     - no link to Solar Assistant
#define LED_R            4
#define LED_G           16
#define LED_B           17
// 0-255; plny jas je v setmele mistnosti oslnujici
// EN: 0-255; full brightness is dazzling in a dim room
#define LED_BRIGHT      70

// LEDC: Arduino ESP32 core 2.x a 3.x maji jine API
// EN: LEDC: Arduino ESP32 core 2.x and 3.x have different APIs
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  #define PWM_ATTACH(pin, ch)       ledcAttach((pin), 5000, 8)
  #define PWM_WRITE(pin, ch, duty)  ledcWrite((pin), (duty))
#else
  #define PWM_ATTACH(pin, ch)       do { ledcSetup((ch), 5000, 8); ledcAttachPin((pin), (ch)); } while (0)
  #define PWM_WRITE(pin, ch, duty)  ledcWrite((ch), (duty))
#endif

TFT_eSPI tft = TFT_eSPI();
Preferences prefs;

// ===========================================================================
// PISMA / FONTS
// Vestavena pisma TFT_eSPI (2/4/6/7) neobsahuji diakritiku. Texty se proto
// EN: Built-in TFT_eSPI fonts (2/4/6/7) carry no diacritics. Text is therefore
//  kresli smooth fontem FontUi / drawn with the FontUi smooth font, numbers
// zustavaji na vestavenych pismech, kde diakritika neni potreba.
// EN: stay on the built-in fonts, where diacritics are not needed.
//
// TFT_eSPI umi mit nactene jen JEDNO smooth pismo a dokud je nactene,
// EN: TFT_eSPI can only have ONE smooth font loaded, and while it is loaded
// ignoruje cislo pisma v drawString(). Proto se mezi nimi prepina.
// EN: it ignores the font number in drawString(). Hence the switching.
// ===========================================================================
bool czLoaded = false;

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

// ================= BARVY / COLOURS ====================================================
#define C_BG      0x0000        // cerna / black
#define C_CARD    0x18E3        // tmave seda karta / dark grey card
#define C_LINE    0x39E7        // obrysy / outlines
#define C_DIM     0x8410        // seda popiska / grey label
#define C_TXT     0xFFFF
#define C_PV      0xFD20        // oranzovozluta / amber
#define C_LOAD    0x2D7F        // modra / blue
#define C_BATT    0x2666        // zelena / green
#define C_GRID    0xF800        // cervena / red
#define C_WEATH   0x07FF        // azurova / cyan
#define C_TRACK   0x2124        // draha ukazatele / gauge track
// tmave cervena - hlavicka pri starych datech
// EN: dark red - header on stale data
#define C_STALE   0x6000
#define C_ALERT   0x8000        // cervena - hlavicka pri upozorneni / red - header during an alert

// ================= ROZVRZENI / LAYOUT ================================================
#define HDR_H       40
#define NAV_Y      432
#define NAV_H       44
#define CONT_Y      42          // zacatek obsahu / start of the content area
#define CONT_H     (NAV_Y - CONT_Y - 4)

#define SCREENS     17

// ================= HODNOTY Z API / API VALUES ============================================
float v_pv_power = 0, v_load_power = 0, v_grid_power = 0, v_batt_power = 0;
float v_soc = 0, v_batt_voltage = 0, v_batt_current = 0, v_batt_capacity = 0;
float v_batt_energy_in = 0, v_batt_energy_out = 0;
float v_pv_energy = 0, v_load_energy = 0;
float v_grid_energy_in = 0, v_grid_energy_out = 0;
float v_grid_voltage = 0, v_grid_freq = 0;
float v_ac_out_voltage = 0, v_ac_out_freq = 0;
float v_load_pct = 0, v_system_power = 0, v_bus_voltage = 0;

float w_cloud = 0, w_irradiance = 0, w_is_day = 0, w_temp = 0;
float w_pv_generated = 0, w_pv_progress = 0, w_pv_remaining = 0;
float w_pv_predicted = 0, w_pv_temp_pred = 0, w_code = 0, w_wind = 0;

float i_temp = 0, i_max_va = 0, i_pv_voltage = 0, i_pv_current = 0;
float i_load_va = 0, i_float_v = 0, i_absorption_v = 0, i_max_charge_a = 0;

// tabulka prirazeni: topic -> promenna / mapping table: topic -> variable
struct Metric { const char* topic; float* var; };

const Metric METRICS[] = {
  { "total/pv_power",                    &v_pv_power },
  { "total/load_power",                  &v_load_power },
  { "total/grid_power",                  &v_grid_power },
  { "total/battery_power",               &v_batt_power },
  { "total/battery_state_of_charge",     &v_soc },
  { "total/battery_voltage",             &v_batt_voltage },
  { "total/battery_current",             &v_batt_current },
  { "total/battery_capacity",            &v_batt_capacity },
  { "total/battery_energy_in",           &v_batt_energy_in },
  { "total/battery_energy_out",          &v_batt_energy_out },
  { "total/pv_energy",                   &v_pv_energy },
  { "total/load_energy",                 &v_load_energy },
  { "total/grid_energy_in",              &v_grid_energy_in },
  { "total/grid_energy_out",             &v_grid_energy_out },
  { "total/grid_voltage",                &v_grid_voltage },
  { "total/grid_frequency",              &v_grid_freq },
  { "total/ac_output_voltage",           &v_ac_out_voltage },
  { "total/ac_output_frequency",         &v_ac_out_freq },
  { "total/load_percentage",             &v_load_pct },
  { "total/system_power",                &v_system_power },
  { "total/bus_voltage",                 &v_bus_voltage },

  { "weather/cloud_cover",               &w_cloud },
  { "weather/global_tilted_irradiance",  &w_irradiance },
  { "weather/is_day",                    &w_is_day },
  { "weather/outside_temperature",       &w_temp },
  { "weather/pv_energy_generated",       &w_pv_generated },
  { "weather/pv_energy_progress",        &w_pv_progress },
  { "weather/pv_energy_remaining_today", &w_pv_remaining },
  { "weather/pv_power_predicted",        &w_pv_predicted },
  { "weather/pv_temperature_predicted",  &w_pv_temp_pred },
  { "weather/weather_code",              &w_code },
  { "weather/wind_speed",                &w_wind },

  { "inverter_1/temperature",                     &i_temp },
  { "inverter_1/max_ac_output_apparent_power",    &i_max_va },
  { "inverter_1/pv_voltage",                      &i_pv_voltage },
  { "inverter_1/pv_current",                      &i_pv_current },
  { "inverter_1/load_apparent_power",             &i_load_va },
  { "inverter_1/battery_float_charge_voltage",    &i_float_v },
  { "inverter_1/battery_absorption_charge_voltage", &i_absorption_v },
  { "inverter_1/max_charge_current",              &i_max_charge_a },
};
const int METRIC_N = sizeof(METRICS) / sizeof(METRICS[0]);

// ================= STAV / STATE =====================================================
bool     wifiOk = false, dataOk = false;
// vypsat hodnoty do Serialu (tlacitko v nastaveni)
// EN: dump values to Serial (button in settings)
bool     dumpRequest = false;

// ---- uzivatelske nastaveni / user settings (SETTINGS 2), stored in NVS ---------------
const uint32_t OPT_FETCH[]  = { 10000, 20000, 30000, 60000 };
const uint32_t OPT_SLEEP[]  = { 0, 60000, 300000, 900000, 1800000 };
const uint8_t  OPT_BRIGHT[] = { 64, 128, 192, 255 };
const uint16_t OPT_RANGE[]  = { 1000, 2000, 3000, 5000 };
const uint16_t OPT_GREEN[]  = { 400, 600, 800, 1000 };
const uint16_t OPT_ORANGE[] = { 1500, 2000, 2500, 3000 };
const uint8_t  OPT_ROT[]    = { 0, 2 };
const uint16_t OPT_BLINK[] = { 0, 1500, 2000, 2500, 3000, 4000 };   // 0 = nikdy / 0 = never
const uint16_t OPT_PRICE[] = { 10, 20, 25, 30, 50, 100, 200, 300,   // cena * 100 / price * 100
                               400, 500, 600, 700, 800, 1000, 1200, 1500 };
const char* const OPT_CURR[] = { "Kč", "€", "zł", "$" };
const uint8_t  OPT_ALERT_SOC[]  = { 10, 15, 20, 25, 30 };
const uint8_t  OPT_ALERT_TEMP[] = { 55, 60, 65, 70, 75, 80 };
const uint32_t OPT_STALE[]      = { 60000, 75000, 120000, 180000, 300000 };
const uint8_t  OPT_ERR_LED[]    = { 0, 1, 2, 3, 4 };  // vyp., cervena, modra, oranz., fialova

#define OPT_N(a) (int)(sizeof(a) / sizeof(a[0]))

uint8_t iFetch = 1, iSleep = 2, iBright = 3, iRange = 1;
uint8_t iGreen = 2, iOrange = 1, iRot = 0;
uint8_t iBlink = 0, iPrice = 10, iCurr = 0;
uint8_t iAlertSoc = 2, iAlertTemp = 3, iStale = 1, iErrLed = 1;

#define CFG_FETCH   OPT_FETCH[iFetch]
#define CFG_SLEEP   OPT_SLEEP[iSleep]
#define CFG_BRIGHT  OPT_BRIGHT[iBright]
#define CFG_RANGE   OPT_RANGE[iRange]
#define CFG_GREEN   OPT_GREEN[iGreen]
#define CFG_ORANGE  OPT_ORANGE[iOrange]
#define CFG_ROT     OPT_ROT[iRot]
#define CFG_BLINK   OPT_BLINK[iBlink]
#define CFG_PRICE   (OPT_PRICE[iPrice] / 100.0f)
#define CFG_CURR    OPT_CURR[iCurr]
#define CFG_ALERT_SOC  OPT_ALERT_SOC[iAlertSoc]
#define CFG_ALERT_TEMP OPT_ALERT_TEMP[iAlertTemp]
#define CFG_STALE      OPT_STALE[iStale]
#define CFG_ERR_LED    OPT_ERR_LED[iErrLed]
// jazyk rozhrani, uklada se do NVS
// EN: interface language, stored in NVS
uint8_t  lang = LANG_CZ;
bool     screenOn = true;              // sviti podsviceni? / is the backlight on?
// posledni dotek, kvuli zhasinani
// EN: last touch, for the sleep timer
uint32_t lastTouch = 0;
// stavova hlaska na strance nastaveni
// EN: status message on the settings screen
char     setMsg[48] = "";
int      failCount = 0;
uint32_t lastFetch = 0, lastOkFetch = 0;

// Nula je platny cas, proto se prvni uspesne nacteni hlida samostatnym
// priznakem a ne porovnanim lastOkFetch s nulou.
// EN: Zero is a valid timestamp, so the first successful fetch is tracked by
//     its own flag instead of comparing lastOkFetch against zero.
bool     haveFetch = false;

// Poslednich 16 udalosti zustava v NVS i po restartu. Jeden zaznam ma jen
// typ, pocet opakovani a cas, aby diagnostika neplytvala pameti.
// EN: The latest 16 events stay in NVS across restarts. Each record keeps
//     only type, repeat count and time, so diagnostics stay memory-efficient.
#define ERR_LOG_N 16
enum { ERR_FETCH = 1, ERR_API, ERR_COUNTER, ERR_SOC, ERR_TEMP };
struct ErrorEntry { uint8_t type, count, hour, minute; uint16_t yday, durationMin; };
ErrorEntry errorLog[ERR_LOG_N];
uint8_t errCount = 0, errPos = 0;
uint8_t activeErrors = 0;
bool apiInvalid = false;

// millis() pretece po 49 dnech. Rozdily dvou casu to prezijou samy, ale doba
// behu ne - proto se preteceni pocitaji zvlast.
// EN: millis() wraps after 49 days. Differences of two stamps survive that on
//     their own, the uptime does not - hence a separate wrap counter.
uint32_t msLast = 0, msWraps = 0;
int      screen = 0;
bool     needFullRedraw = true;

// rychlost zmeny SOC v %/hod / rate of SOC change in %/h
float    socRate = 0;
float    socPrev = -1;
uint32_t socPrevMs = 0;

// ================= HISTORIE / HISTORY =================================================
// Prumery po 10 minutach pro cely den, index = (hodina*60 + minuta) / 10.
// EN: Ten minute averages for the whole day, index = (hour*60 + minute) / 10.
// Pole se maze o pulnoci, graf tedy ukazuje prubeh dnesniho dne.
// EN: The array is cleared at midnight, so the chart shows today only.
#define DAY_N 144
int16_t dPv[DAY_N], dLoad[DAY_N], dBatt[DAY_N];
int16_t dInvTemp[DAY_N], dOutTemp[DAY_N];
uint8_t dSoc[DAY_N];
bool    dHas[DAY_N];
int     curSlot = -1, curDay = -1;

// denni maximum vykonu baterie v obou smerech, kvuli meritku grafu
// EN: daily peak battery power in both directions, for the chart scale
int16_t maxBattPwr = 0;

// ---- denni a mesicni souhrny / daily and monthly totals ---------------
// Hodnoty jsou v desetinach jednotky (kWh*10, mena*10), aby se vesly
// EN: Values are in tenths of a unit (kWh*10, currency*10) so they fit
// do dvou bajtu a cely zaznam do NVS.
// EN: into two bytes and the whole record into NVS.
#define HIST_DAYS 31
uint16_t hdPv[HIST_DAYS], hdLoad[HIST_DAYS], hdSave[HIST_DAYS];
uint16_t hdBIn[HIST_DAYS], hdBOut[HIST_DAYS], hdGrid[HIST_DAYS];
uint8_t  hdDayNum[HIST_DAYS];

// Zobrazene obdobi na strance HISTORIE: 0 = tyden, 1 = 31 dni, 2 = 12 mesicu.
// Prepina se klepnutim na graf, po restartu zacina zase u tydne.
// EN: Period shown on the HISTORY screen: 0 = week, 1 = 31 days, 2 = 12 months.
//     Tapping the chart switches it; after a restart it starts at the week again.
uint8_t  histRange = 0;
int      hdCount = 0, hdPos = 0;

uint16_t hmPv[12], hmLoad[12], hmSave[12];
uint16_t hmBIn[12], hmBOut[12], hmGrid[12];
int      sumYear = -1;

// Sezonni plan vlastni spotreby po mesicich v desetinach kWh.
// EN: Seasonal plan of self-consumed energy by month, in tenths of kWh.
// Vychazi z dodane tabulky: zatez minus energie odebrana ze site.
// EN: Based on the supplied table: load minus energy imported from the grid.
const uint16_t PLAN_OWN_KWH10[12] = {
   55, 153, 721, 658, 828, 924, 1040, 1080, 579, 311, 220, 99
};

// Stav kumulativnich pocitadel o pulnoci.
// EN: State of the cumulative counters at midnight.
//
// CZ: API vraci total/*_energy jako pocitadla od instalace, ne denni
//     hodnoty. Jedina skutecne denni polozka je weather/pv_energy_generated.
//     Denni spotreba se proto musi pocitat jako rozdil proti pulnoci.
// EN: The API returns total/*_energy as counters since installation, not
//     daily figures. The only truly daily item is weather/pv_energy_generated.
//     Daily consumption therefore has to be the difference since midnight.
float baseLoad = 0, baseGridIn = 0, baseBattIn = 0, baseBattOut = 0;
bool  baseValid = false;
int   lastMon = -1;

// vychod a zapad slunce v minutach od pulnoci
// EN: sunrise and sunset in minutes from midnight
int sunRise = -1, sunSet = -1;

bool    timeOk = false;
char    clockStr[8] = "--:--";
bool    histLoaded = false;

// denni extremy, nuluji se o pulnoci spolu s historii
// EN: daily extremes, cleared at midnight together with the history
int16_t peakPv = 0, maxLoad = 0;
uint8_t minSoc = 100;
int16_t minInvTemp = 32767, maxInvTemp = -32768, minOutTemp = 32767, maxOutTemp = -32768;
int16_t minInvSlot = -1, maxInvSlot = -1, minOutSlot = -1, maxOutSlot = -1;



// ===========================================================================
// RGB LED - signalizace zateze / RGB LED - load signalling
// ===========================================================================

uint8_t errorBit(uint8_t type) { return (uint8_t)(1U << (type - 1)); }

const char* errorName(uint8_t type) {
  switch (type) {
    case ERR_FETCH:   return TR(T_ERR_FETCH);
    case ERR_API:     return TR(T_ERR_API);
    case ERR_COUNTER: return TR(T_ERR_COUNTER);
    case ERR_SOC:     return TR(T_ALERT_SOC);
    default:          return TR(T_ALERT_TEMP);
  }
}

void errorSave() {
  prefs.putBytes("errLog", errorLog, sizeof(errorLog));
  prefs.putUChar("errCnt", errCount);
  prefs.putUChar("errPos", errPos);
}

void errorLoad() {
  prefs.getBytes("errLog", errorLog, sizeof(errorLog));
  errCount = prefs.getUChar("errCnt", 0);
  errPos = prefs.getUChar("errPos", 0);
  if (errCount > ERR_LOG_N) errCount = 0;
  if (errPos >= ERR_LOG_N) errPos = 0;
}

void errorClear() {
  memset(errorLog, 0, sizeof(errorLog));
  errCount = errPos = 0;
  errorSave();
}

void errorStart(uint8_t type) {
  uint8_t bit = errorBit(type);
  if (activeErrors & bit) return;
  activeErrors |= bit;
  struct tm t;
  ErrorEntry& entry = errorLog[errPos];
  entry.type = type; entry.count = 1; entry.yday = 0; entry.durationMin = 0; entry.hour = 255; entry.minute = 255;
  if (getLocalTime(&t, 5)) {
    entry.yday = (uint16_t)t.tm_yday;
    entry.hour = (uint8_t)t.tm_hour;
    entry.minute = (uint8_t)t.tm_min;
  }
  errPos = (errPos + 1) % ERR_LOG_N;
  if (errCount < ERR_LOG_N) errCount++;
  errorSave();
}

void errorStop(uint8_t type) {
  uint8_t bit = errorBit(type);
  if (!(activeErrors & bit)) return;
  activeErrors &= (uint8_t)~bit;
  if ((type == ERR_FETCH || type == ERR_API) && errCount > 0 && haveFetch) {
    ErrorEntry& entry = errorLog[(errPos + ERR_LOG_N - 1) % ERR_LOG_N];
    if (entry.type == type) {
      uint32_t mins = (millis() - lastOkFetch + 59999UL) / 60000UL;
      entry.durationMin = mins > 65535UL ? 65535U : (uint16_t)mins;
      errorSave();
    }
  }
}

void errorRepeat(uint8_t type) {
  if (!(activeErrors & errorBit(type)) || errCount == 0) return;
  ErrorEntry& entry = errorLog[(errPos + ERR_LOG_N - 1) % ERR_LOG_N];
  if (entry.type == type && entry.count < 255) { entry.count++; errorSave(); }
}

// LED na desce je zapojena jako aktivni v LOW, proto se strida obraci
// EN: the on-board LED is active LOW, so the duty cycle is inverted
void setLed(uint8_t r, uint8_t g, uint8_t b) {
  PWM_WRITE(LED_R, 0, 255 - (r * LED_BRIGHT / 255));
  PWM_WRITE(LED_G, 1, 255 - (g * LED_BRIGHT / 255));
  PWM_WRITE(LED_B, 2, 255 - (b * LED_BRIGHT / 255));
}

// Upozorneni: nizky stav baterie nebo prehrivajici se menic.
// EN: Alert: low battery or an overheating inverter.
// Projevi se cervenym ramem kolem obsahu, cervenou hlavickou, blikajici
// EN: Shows as a red frame around the content, a red header, a blinking
// diodou a textem na prislusne strance. / LED and text on the relevant screen.
bool alertActive() {
  return dataOk && (v_soc < CFG_ALERT_SOC || i_temp > CFG_ALERT_TEMP);
}

const char* alertText() {
  return v_soc < CFG_ALERT_SOC ? TR(T_ALERT_SOC) : TR(T_ALERT_TEMP);
}

// uroven zateze: 0 mala, 1 stredni, 2 velka, 3 bez dat
// EN: load level: 0 light, 1 medium, 2 heavy, 3 no data
int loadLevel() {
  if (!dataOk) return 3;
  if (v_load_power <= CFG_GREEN)  return 0;
  if (v_load_power <= CFG_ORANGE) return 1;
  return 2;
}

const char* loadLevelName() {
  switch (loadLevel()) {
    case 0:  return "malá";
    case 1:  return "střední";
    case 2:  return "velká";
    default: return "bez dat";
  }
}

// barva ukazatele zateze - zamerne stejna jako barva diody
// EN: load gauge colour - deliberately the same as the LED
uint16_t loadColor() {
  switch (loadLevel()) {
    case 0:  return C_BATT;
    case 1:  return C_PV;
    case 2:  return C_GRID;
    default: return C_LOAD;
  }
}

void updateLoadLed() {
  if (activeErrors && CFG_ERR_LED > 0) {
    bool on = (millis() / 500) % 2;
    switch (CFG_ERR_LED) {
      case 1: setLed(on ? 255 : 0, 0, 0); break;
      case 2: setLed(0, 0, on ? 255 : 0); break;
      case 3: setLed(on ? 255 : 0, on ? 110 : 0, 0); break;
      default:setLed(on ? 255 : 0, 0, on ? 255 : 0); break;
    }
    return;
  }
  // pri upozorneni dioda blika cervene
  // EN: during an alert the LED blinks red
  if (alertActive()) {
    setLed((millis() / 500) % 2 ? 255 : 0, 0, 0);
    return;
  }
  // blikani pri velke zatezi, prah se nastavuje v NASTAVENI 2
  // EN: blinking on heavy load, the threshold is set on SETTINGS 2
  if (CFG_BLINK > 0 && dataOk && v_load_power > CFG_BLINK) {
    setLed((millis() / 400) % 2 ? 255 : 0, 0, 0);
    return;
  }
  switch (loadLevel()) {
    case 0:  setLed(0, 255, 0);    break;   // zelena / green
    case 1:  setLed(255, 110, 0);  break;   // oranzova / orange
    case 2:  setLed(255, 0, 0);    break;   // cervena / red
    default: setLed(0, 0, 255);    break;   // modra - chybi data / blue - data missing
  }
}

// ===========================================================================
// IKONY  (kreslene vektorove, velikost ~30 px)
// EN: ICONS  (vector drawn, about 30 px)
// ===========================================================================
void icoSun(int x, int y, uint16_t c) {
  tft.fillCircle(x + 15, y + 15, 7, c);
  for (int a = 0; a < 360; a += 45) {
    float r = a * DEG_TO_RAD;
    tft.drawLine(x + 15 + cos(r) * 10, y + 15 + sin(r) * 10,
                 x + 15 + cos(r) * 14, y + 15 + sin(r) * 14, c);
  }
}

void icoCloud(int x, int y, uint16_t c) {
  tft.fillCircle(x + 10, y + 18, 6, c);
  tft.fillCircle(x + 18, y + 15, 8, c);
  tft.fillCircle(x + 24, y + 19, 5, c);
  tft.fillRect(x + 10, y + 19, 15, 5, c);
}

void icoSunCloud(int x, int y, uint16_t c) {
  tft.fillCircle(x + 20, y + 10, 6, C_PV);
  icoCloud(x, y + 3, c);
}

void icoBattery(int x, int y, uint16_t c) {
  tft.drawRoundRect(x + 4, y + 4, 18, 24, 3, c);
  tft.fillRect(x + 10, y + 1, 6, 3, c);
  // blesk / lightning bolt
  tft.fillTriangle(x + 15, y + 8, x + 9, y + 18, x + 13, y + 18, c);
  tft.fillTriangle(x + 11, y + 24, x + 17, y + 14, x + 13, y + 14, c);
}

void icoPlug(int x, int y, uint16_t c) {
  tft.drawRoundRect(x + 6, y + 10, 18, 12, 3, c);
  tft.drawLine(x + 10, y + 4, x + 10, y + 10, c);
  tft.drawLine(x + 20, y + 4, x + 20, y + 10, c);
  tft.drawLine(x + 24, y + 16, x + 28, y + 16, c);
}

void icoInverter(int x, int y, uint16_t c) {
  tft.drawRoundRect(x + 5, y + 3, 20, 26, 3, c);
  tft.fillRect(x + 9, y + 8, 12, 7, c);
  tft.drawLine(x + 9, y + 20, x + 21, y + 20, c);
  tft.drawLine(x + 9, y + 24, x + 21, y + 24, c);
}

// fotovoltaicky panel se stojanem / photovoltaic panel with a stand
void icoPanel(int x, int y, uint16_t c) {
  tft.drawRect(x + 2, y + 5, 26, 16, c);
  tft.drawFastVLine(x + 10, y + 5, 16, c);
  tft.drawFastVLine(x + 19, y + 5, 16, c);
  tft.drawFastHLine(x + 2, y + 13, 26, c);
  tft.drawLine(x + 15, y + 21, x + 15, y + 27, c);
  tft.drawFastHLine(x + 8, y + 27, 15, c);
}

// prihradovy stozar vysokeho napeti / lattice transmission tower
void icoPylon(int x, int y, uint16_t c) {
  tft.drawLine(x + 5, y + 28, x + 13, y + 5, c);
  tft.drawLine(x + 25, y + 28, x + 17, y + 5, c);
  tft.drawFastHLine(x + 13, y + 5, 5, c);
  tft.drawFastHLine(x + 4, y + 10, 22, c);
  tft.drawFastHLine(x + 2, y + 16, 26, c);
  tft.drawLine(x + 9, y + 18, x + 21, y + 18, c);
  tft.drawLine(x + 10, y + 23, x + 20, y + 23, c);
  tft.drawLine(x + 11, y + 10, x + 19, y + 16, c);
  tft.drawLine(x + 19, y + 10, x + 11, y + 16, c);
}

void icoHouse(int x, int y, uint16_t c) {
  // sirka domu je licha, aby dvere sedly presne na stred
  // EN: the house width is odd so the door lands exactly in the centre
  tft.fillTriangle(x + 4, y + 15, x + 15, y + 4, x + 26, y + 15, c);
  tft.drawRect(x + 8, y + 15, 15, 13, c);
  tft.fillRect(x + 13, y + 20, 5, 8, c);
}

void icoTemp(int x, int y, uint16_t c) {
  tft.drawRoundRect(x + 11, y + 3, 8, 18, 4, c);
  tft.fillCircle(x + 15, y + 23, 6, c);
  tft.fillRect(x + 14, y + 10, 3, 12, c);
}

void icoWind(int x, int y, uint16_t c) {
  tft.drawLine(x + 4, y + 10, x + 18, y + 10, c);
  tft.drawCircle(x + 20, y + 8, 4, c);
  tft.drawLine(x + 4, y + 17, x + 22, y + 17, c);
  tft.drawCircle(x + 24, y + 20, 5, c);
}

void icoClock(int x, int y, uint16_t c) {
  tft.drawCircle(x + 15, y + 15, 12, c);
  tft.drawLine(x + 15, y + 15, x + 15, y + 8, c);
  tft.drawLine(x + 15, y + 15, x + 20, y + 17, c);
}

void icoBolt(int x, int y, uint16_t c) {
  tft.fillTriangle(x + 18, y + 3, x + 8, y + 17, x + 15, y + 17, c);
  tft.fillTriangle(x + 12, y + 28, x + 22, y + 13, x + 15, y + 13, c);
}

// ===========================================================================
// ZAKLADNI GRAFICKE PRVKY / BASIC DRAWING PRIMITIVES
// ===========================================================================

// prstenec: uhly ve stupnich, 0 = vpravo, roste po smeru hodinovych rucicek
// EN: ring: angles in degrees, 0 = right, growing clockwise
void arcRing(int cx, int cy, int rIn, int rOut, float aFrom, float aTo, uint16_t col) {
  if (aTo <= aFrom) return;
  for (float a = aFrom; a <= aTo; a += 0.7f) {
    float r = a * DEG_TO_RAD;
    float c = cos(r), s = sin(r);
    tft.drawLine(cx + c * rIn, cy + s * rIn, cx + c * rOut, cy + s * rOut, col);
  }
}

// Carkova stupnice po obvodu ukazatele. Carky jsou po 5 %, kazda pata
// EN: Tick scale around the gauge. Ticks every 5 %, every fifth one
// (tedy 0 / 25 / 50 / 75 / 100 %) je delsi a svetlejsi.
// EN: (0 / 25 / 50 / 75 / 100 %) is longer and lighter.
// Kresli se VNE prstence, protoze uvnitr je misto na hodnotu.
// EN: Drawn OUTSIDE the ring, the inside is reserved for the value.
#define TICK_N     20        // 20 dilku = 21 carek / 20 divisions = 21 ticks
#define TICK_MAJOR  5        // kazda pata carka je vyrazna / every fifth tick stands out

void gaugeTicks(int cx, int cy, int r) {
  for (int i = 0; i <= TICK_N; i++) {
    float a = 180.0f + 180.0f * i / TICK_N;
    float rad = a * DEG_TO_RAD;
    float c = cos(rad), si = sin(rad);

    bool big = (i % TICK_MAJOR == 0);
    int r1 = r + 2;
    int r2 = r + (big ? 7 : 4);

    tft.drawLine(cx + c * r1, cy + si * r1,
                 cx + c * r2, cy + si * r2,
                 big ? C_DIM : C_TRACK);
  }
}

// Ukazatel s jednotkou, ktera se kresli ceskym fontem. Vestavena pisma
// obsahuji jen ASCII, takze znak stupne by v nich chybel. Hodnota zustava
// velkym pismem, jednotka se pripoji hned za ni.
// EN: Gauge with a unit drawn in the Czech font. The built-in fonts are ASCII
//     only, so the degree sign would be missing. The value keeps the large
//     font and the unit is appended right after it.
void halfGaugeU(int cx, int cy, int r, int thick, float val, float maxVal,
                uint16_t col, const char* valText, const char* unit,
                const char* label) {
  float f = maxVal > 0 ? constrain(val / maxVal, 0.0f, 1.0f) : 0;

  arcRing(cx, cy, r - thick, r, 180, 360, C_TRACK);
  if (f > 0.004f)
    arcRing(cx, cy, r - thick, r, 180, 180 + 180 * f, col);
  gaugeTicks(cx, cy, r);

  czOff();
  int wv = tft.textWidth(valText, 4);
  czOn();
  int wu = tft.textWidth(unit);

  int x0 = cx - (wv + 3 + wu) / 2;
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_TXT, C_BG);
  tAs(valText, x0, cy - 29, 4);
  tCz(unit, x0 + wv + 3, cy - 24);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_DIM, C_BG);
  tCz(label, cx, cy + 12);
  tft.setTextDatum(TL_DATUM);
}

// pulkruhovy ukazatel jako v predloze: draha + barevna vyplnena cast
// EN: half-circle gauge as in the reference: track plus a coloured filled part
void halfGauge(int cx, int cy, int r, int thick, float val, float maxVal,
               uint16_t col, const char* valText, const char* label) {
  float f = maxVal > 0 ? constrain(val / maxVal, 0.0f, 1.0f) : 0;

  arcRing(cx, cy, r - thick, r, 180, 360, C_TRACK);
  if (f > 0.004f)
    arcRing(cx, cy, r - thick, r, 180, 180 + 180 * f, col);
  gaugeTicks(cx, cy, r);

  // hodnota uvnitr oblouku, popisek pod nim
  // EN: value inside the arc, label below it
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TXT, C_BG);
  tAs(valText, cx, cy - 16, 4);
  tft.setTextColor(C_DIM, C_BG);
  tCz(label, cx, cy + 12);
  tft.setTextDatum(TL_DATUM);
}

// vodorovny pruh s vyplni / horizontal bar with a fill
void bar(int x, int y, int w, int h, float frac, uint16_t col) {
  frac = constrain(frac, 0.0f, 1.0f);
  tft.fillRoundRect(x, y, w, h, h / 2, C_TRACK);
  int fw = (int)(w * frac);
  if (fw <= 0) return;
  // i maly podil ma zaobleny konec, ne hranaty
  // EN: even a small share gets a rounded end, not a square one
  if (fw < h) fw = h;
  tft.fillRoundRect(x, y, fw, h, h / 2, col);
}

// karta s ikonou vlevo a texty vpravo (predloha)
// EN: card with an icon on the left and text on the right (per the reference)
void card(int x, int y, int w, int h, int icon, uint16_t iconCol,
          const char* title, const char* l1, const char* l2) {
  tft.fillRoundRect(x, y, w, h, 8, C_CARD);
  tft.drawRoundRect(x, y, w, h, 8, C_LINE);

  // podklad pod ikonu / backdrop behind the icon
  tft.fillRoundRect(x + 6, y + 6, 38, 38, 6, C_BG);
  int ix = x + 10, iy = y + 10;
  switch (icon) {
    case 0: icoInverter(ix, iy, iconCol); break;
    case 1: icoSunCloud(ix, iy, iconCol); break;
    case 2: icoPlug(ix, iy, iconCol);     break;
    case 3: icoBattery(ix, iy, iconCol);  break;
    case 4: icoHouse(ix, iy, iconCol);    break;
    case 5: icoTemp(ix, iy, iconCol);     break;
    case 6: icoWind(ix, iy, iconCol);     break;
    case 7: icoCloud(ix, iy, iconCol);    break;
    case 8: icoBolt(ix, iy, iconCol);     break;
    case 9: icoClock(ix, iy, iconCol);    break;
    case 10: icoPanel(ix, iy, iconCol);   break;
    case 11: icoPylon(ix, iy, iconCol);   break;
  }

  int tx = x + 50;
  tft.setTextColor(C_TXT, C_CARD);
  tCz(title, tx, y + 6);
  tft.setTextColor(iconCol, C_CARD);
  tCz(l1, tx, y + 23);
  tft.setTextColor(C_DIM, C_CARD);
  tCz(l2, tx, y + 39);
}

// dlazdice s dlouhou hodnotou - hodnota se kresli mensim ceskym pismem
// EN: tile with a long value - the value uses the smaller Czech font
void statBoxSmall(int x, int y, int w, int h, const char* label,
                  const char* value, uint16_t col) {
  tft.fillRoundRect(x, y, w, h, 6, C_CARD);
  tft.drawRoundRect(x, y, w, h, 6, col);
  tft.setTextColor(C_DIM, C_CARD);
  tCz(label, x + 7, y + 4);
  tft.setTextColor(col, C_CARD);
  tCz(value, x + 7, y + 24);
}

// mala dlazdice: popisek nahore, hodnota dole
// EN: small tile: label on top, value below
void statBox(int x, int y, int w, int h, const char* label,
          const char* value, uint16_t col) {
  tft.fillRoundRect(x, y, w, h, 6, C_CARD);
  tft.drawRoundRect(x, y, w, h, 6, col);
  tft.setTextColor(C_DIM, C_CARD);
  tCz(label, x + 7, y + 4);
  tft.setTextColor(col, C_CARD);
  tAs(value, x + 7, y + 20, 4);
}

// ===========================================================================
// FORMATOVANI / FORMATTING
// ===========================================================================
char fbuf[8][24];
int  fidx = 0;

const char* fmt(const char* format, float value) {
  fidx = (fidx + 1) % 8;
  snprintf(fbuf[fidx], 24, format, value);
  return fbuf[fidx];
}

// vykon se znamenkem, pro ukazatele kde je smer toku podstatny
// EN: signed power, for gauges where the direction of flow matters
// formatovani dvou celych cisel do sdileneho bufferu
// EN: formats two integers into the shared buffer
const char* fmt2(const char* f, uint32_t a, uint32_t b) {
  fidx = (fidx + 1) % 8;
  snprintf(fbuf[fidx], 24, f, (unsigned long)a, (unsigned long)b);
  return fbuf[fidx];
}

const char* fmtSigned(float w) {
  fidx = (fidx + 1) % 8;
  if (fabsf(w) >= 1000) snprintf(fbuf[fidx], 24, "%+.2f kW", w / 1000.0f);
  else                  snprintf(fbuf[fidx], 24, "%+d W", (int)roundf(w));
  return fbuf[fidx];
}

const char* fmtPower(float w) {
  fidx = (fidx + 1) % 8;
  if (fabsf(w) >= 1000) snprintf(fbuf[fidx], 24, "%.2f kW", w / 1000.0f);
  else                  snprintf(fbuf[fidx], 24, "%d W", (int)roundf(w));
  return fbuf[fidx];
}

// Castka vzdy pouziva aktualne zvolenou menu. Vlastni formatter drzi jeden
// argument fmt() mimo cestu a zachovava stejny kruhovy buffer.
// EN: Money always uses the selected currency. Its own formatter keeps the
//     one-argument fmt() contract and shares the same circular buffer.
const char* fmtMoney(float value) {
  fidx = (fidx + 1) % 8;
  snprintf(fbuf[fidx], sizeof(fbuf[fidx]), "%.0f %s", value, CFG_CURR);
  return fbuf[fidx];
}

const char* weatherText(int code) {
  if (code == 0)   return TR(T_W_CLEAR);
  if (code <= 2)   return TR(T_W_PARTLY);
  if (code == 3)   return TR(T_W_OVERCAST);
  if (code <= 48)  return TR(T_W_FOG);
  if (code <= 57)  return TR(T_W_DRIZZLE);
  if (code <= 67)  return TR(T_W_RAIN);
  if (code <= 77)  return TR(T_W_SNOW);
  if (code <= 82)  return TR(T_W_SHOWERS);
  if (code <= 86)  return TR(T_W_SNOWSH);
  return TR(T_W_STORM);
}

// ===========================================================================
// OBRAZOVKA 0 - PREHLED / SCREEN 0 - OVERVIEW
// ===========================================================================
void scrOverview() {
  // ---- karty / cards ----
  static char invUse[24];
  snprintf(invUse, sizeof(invUse), "%s %.0f %%", TR(T_USAGE), v_load_pct);
  card(6,    44, 150, 56, 0,  C_LOAD,  TR(T_INVERTER),
       fmt("%.1f °C", i_temp), invUse);

  // Napeti a proud panelu. Bez mezer kolem lomitka, jinak se to do karty
  // EN: Panel voltage and current. No spaces around the slash or it will not fit
  // nevejde; nad 100 V se u napeti vypousti desetinne misto.
  // EN: into the card; above 100 V the voltage drops its decimal.
  static char pvUI[24];
  if (i_pv_voltage >= 100)
    snprintf(pvUI, sizeof(pvUI), "%.0fV/%.1fA", i_pv_voltage, i_pv_current);
  else
    snprintf(pvUI, sizeof(pvUI), "%.1fV/%.1fA", i_pv_voltage, i_pv_current);
  card(164,  44, 150, 56, 10, C_PV,    TR(T_SOLARPV),
       fmtPower(v_pv_power), pvUI);

  // napeti a frekvence site; bez mezer kolem lomitka, jinak se to do karty nevejde
  // EN: grid voltage and frequency; no spaces around the slash or it will not fit
  static char gridVF[24];
  snprintf(gridVF, sizeof(gridVF), "%.0fV/%.1fHz", v_grid_voltage, v_grid_freq);
  card(6,   104, 150, 56, 11, C_GRID,  TR(T_GRID),
       gridVF, fmtPower(v_grid_power));

  card(164, 104, 150, 56, 3,  C_BATT,  TR(T_BATTERY),
       fmt("%.1f V", v_batt_voltage), fmt("%.0f %%", v_soc));

  // proud baterie hned za procenty, zeleny pri nabijeni, cerveny pri vybijeni
  // EN: battery current right after the percentage, green charging, red discharging
  static char curBuf[16];
  snprintf(curBuf, sizeof(curBuf), "%+.0fA",
           v_batt_power >= 0 ? v_batt_current : -v_batt_current);
  czOn();
  int wSoc = tft.textWidth(fmt("%.0f %%", v_soc));
  tft.setTextColor(v_batt_power >= 0 ? C_BATT : C_GRID, C_CARD);
  tCz(curBuf, 164 + 50 + wSoc + 8, 104 + 39);

  // ---- predikce vyroby na cely den / whole-day production forecast ----
  tft.fillRoundRect(6, 164, 308, 64, 8, C_CARD);
  tft.drawRoundRect(6, 164, 308, 64, 8, C_PV);

  // nadpis vlevo, prubeh v procentech vpravo
  // EN: title on the left, progress in percent on the right
  tft.setTextColor(C_DIM, C_CARD);
  tCz(TR(T_FORECAST), 14, 168);

  // kolik dnesni vyroba usetrila v penezich, na stredu radku s nadpisem
  // EN: how much today's production saved in money, centred on the title row
  // Vlevo uz usetrena castka, vpravo predpoklad za cely den. Ten vychazi
  // z predikce vyroby: co se jeste vyrobi, to se taky spotrebuje nebo ulozi.
  // EN: The amount saved so far, then the estimate for the whole day. It comes
  //     from the production forecast: whatever is still to be made gets used
  //     or stored.
  static char savBuf[32];
  float savedAll = savedToday() + w_pv_remaining * CFG_PRICE;
  snprintf(savBuf, sizeof(savBuf), "%s %.0f %s / %.0f %s",
           TR(T_TODAY), savedToday(), CFG_CURR, savedAll, CFG_CURR);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_BATT, C_CARD);
  tCz(savBuf, 160, 177);
  tft.setTextDatum(TL_DATUM);

  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_PV, C_CARD);
  tCz(fmt("%.0f %%", w_pv_progress), 306, 168);
  tft.setTextDatum(TL_DATUM);

  // vyrobeno vlevo / produced, on the left
  tft.setTextColor(C_TXT, C_CARD);
  tCz(fmt("%.2f kWh", w_pv_generated), 14, 188);
  tft.setTextColor(C_DIM, C_CARD);
  tCz(TR(T_PRODUCED), 14, 206);

  // zbyva vpravo / remaining, on the right
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_TXT, C_CARD);
  tCz(fmt("%.2f kWh", w_pv_remaining), 306, 188);
  tft.setTextColor(C_DIM, C_CARD);
  tCz(TR(T_REMAINING), 306, 206);
  tft.setTextDatum(TL_DATUM);

  // posuvnik mezi obema hodnotami / slider between the two values
  bar(104, 192, 112, 12, w_pv_progress / 100.0f, C_PV);



  // ---- pulkruhove ukazatele / half-circle gauges ----
  halfGauge(84,  300, 62, 13, v_load_power, CFG_RANGE, C_LOAD,
            fmtPower(v_load_power), TR(T_LOAD));
  halfGauge(236, 300, 62, 13, v_pv_power, CFG_RANGE, C_PV,
            fmtPower(v_pv_power), TR(T_SOLARPV));
  halfGauge(84,  394, 62, 13, fabsf(v_grid_power), CFG_RANGE,
            v_grid_power > 1 ? C_GRID : C_DIM,
            fmtPower(fabsf(v_grid_power)), TR(T_GRID));
  // vykon se znamenkem: kladny se nabiji, zaporny vybiji
  // EN: signed power: positive is charging, negative is discharging
  halfGauge(236, 394, 62, 13, fabsf(v_batt_power), (CFG_RANGE / 2),
            v_batt_power >= 0 ? C_BATT : C_GRID,
            fmtSigned(v_batt_power),
            v_batt_power >= 0 ? TR(T_BATT_CHG) : TR(T_BATT_DIS));
}

// ===========================================================================
// OBRAZOVKA 1 - BATERIE / SCREEN 1 - BATTERY
// ===========================================================================
void scrBattery() {
  const int cx = 160, cy = 208, r = 118, th = 22;
  // zeleny prstenec pri nabijeni, cerveny pri vybijeni
  // EN: green ring while charging, red while discharging
  uint16_t col = v_batt_power >= 0 ? C_BATT : C_GRID;
  // pod 20 % zustane cislo cervene i pri nabijeni, aby varovani neslo prehlednout
  // EN: below 20 % the number stays red even while charging, so the warning cannot be missed
  uint16_t numCol = v_soc < 20 ? C_GRID : col;

  arcRing(cx, cy, r - th, r, 180, 360, C_TRACK);
  arcRing(cx, cy, r - th, r, 180, 180 + 180 * constrain(v_soc / 100.0f, 0.0f, 1.0f), col);
  gaugeTicks(cx, cy, r);

  // Velke cislo s malym procentem vedle. Vestavene pismo 7 ma jen cislice,
  // EN: Large number with a small percent sign. Built-in font 7 has digits only,
  // procento se proto kresli ceskym fontem. Aby byla dvojice vycentrovana,
  // EN: so the percent sign uses the Czech font. To centre the pair,
  // zmeri se sirka cisla a od ni se odvodi zacatek.
  // EN: the number width is measured and the start derived from it.
  const char* socStr = fmt("%.0f", v_soc);
  czOff();
  int wNum = tft.textWidth(socStr, 7);
  // sirka maleho procenta
  // EN: width of the small percent sign
  const int wPct = 12;
  int x0 = cx - (wNum + 5 + wPct) / 2;

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(numCol, C_BG);
  tAs(socStr, x0, 134, 7);
  tft.setTextColor(C_DIM, C_BG);
  tCz("%", x0 + wNum + 5, 158);

  // rychlost zmeny SOC / rate of SOC change
  char rbuf[24];
  snprintf(rbuf, sizeof(rbuf), "%+.1f %%/hod", socRate);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(socRate >= 0 ? C_BATT : C_GRID, C_BG);
  // vnitrek prstence je v teto vysce 184 px
  // EN: the ring interior is 184 px at this height
  tAs(rbuf, cx, 196, 4);

  // pri nizkem stavu baterie je misto smeru toku varovani
  // EN: on a low battery the warning replaces the flow direction
  if (v_soc < CFG_ALERT_SOC) {
    tft.setTextColor(C_GRID, C_BG);
    tCz(TR(T_ALERT_SOC), cx, 220);
  } else {
    tft.setTextColor(C_TXT, C_BG);
    tCz(v_batt_power >= 0 ? TR(T_CHARGING) : TR(T_DISCHARGING), cx, 220);
  }
  tft.setTextDatum(TL_DATUM);

  const int sy = 232, sh = 50, sg = 5;
  statBox(6,   sy,           150, sh, TR(T_VOLTAGE),    fmt("%.1f V", v_batt_voltage), C_TXT);
  statBox(164, sy,           150, sh, TR(T_CURRENT),    fmt("%.1f A", v_batt_current), C_TXT);
  statBox(6,   sy+sh+sg,     150, sh, TR(T_POWER),      fmtPower(fabsf(v_batt_power)), C_BATT);
  statBox(164, sy+sh+sg,     150, sh, TR(T_BATT_LEFT),
          fmt("%.2f kWh", v_batt_capacity * v_soc / 100.0f), C_TXT);
  statBox(6,   sy+2*(sh+sg), 150, sh, TR(T_CHARGED_TD), fmt("%.2f kWh", dayBattIn()), C_BATT);
  statBox(164, sy+2*(sh+sg), 150, sh, TR(T_DISCH_TD),   fmt("%.2f kWh", dayBattOut()), C_PV);

  tft.setTextColor(C_DIM, C_BG);
  tCz(TR(T_MIN_TODAY), 6, 396);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TXT, C_BG);
  tCz(fmt("%.0f %%", (float)minSoc), 112, 396);
  // Optimisticky odhad: cela zbyvajici predikovana vyroba muze do baterie.
  float forecastSoc = v_batt_capacity > 0 ? min(100.0f, v_soc + w_pv_remaining / v_batt_capacity * 100.0f) : v_soc;
  tft.setTextColor(C_DIM, C_BG); tCz(TR(T_SOC_EVENING), 206, 396);
  tft.setTextColor(C_BATT, C_BG); tCz(fmt("%.0f %%", forecastSoc), 306, 396);
  tft.setTextDatum(TL_DATUM);
}

// ===========================================================================
// OBRAZOVKA 2 - SOLAR / SCREEN 2 - SOLAR
// ===========================================================================
void scrSolar() {
  halfGauge(160, 160, 100, 20, v_pv_power, CFG_RANGE, C_PV,
            fmtPower(v_pv_power), TR(T_PV_NOW));

  // prubeh vyroby za den / production progress over the day
  tft.setTextColor(C_DIM, C_BG);
  tCz(TR(T_DAY_PROG), 6, 186);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_PV, C_BG);
  tAs(fmt("%.0f %%", w_pv_progress), 314, 186, 2);
  tft.setTextDatum(TL_DATUM);
  bar(6, 204, 308, 14, w_pv_progress / 100.0f, C_PV);

  const int sy = 228, sh = 50, sg = 5;
  statBox(6,   sy,           150, sh, TR(T_PROD_TODAY), fmt("%.2f kWh", w_pv_generated), C_PV);
  statBox(164, sy,           150, sh, TR(T_LEFT_TODAY),    fmt("%.2f kWh", w_pv_remaining), C_TXT);
  statBox(6,   sy+sh+sg,     150, sh, TR(T_FORECAST),      fmtPower(w_pv_predicted), C_WEATH);
  statBox(164, sy+sh+sg,     150, sh, TR(T_IRRAD),         fmt("%.0f W/m2", w_irradiance), C_WEATH);
  statBox(6,   sy+2*(sh+sg), 150, sh, TR(T_PANEL_V), fmt("%.1f V", i_pv_voltage), C_TXT);
  statBox(164, sy+2*(sh+sg), 150, sh, TR(T_PANEL_A),  fmt("%.1f A", i_pv_current), C_TXT);

  tft.setTextColor(C_DIM, C_BG);
  tCz(TR(T_PEAK_TODAY), 6, 396);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_PV, C_BG);
  tCz(fmtPower((float)peakPv), 314, 396);
  tft.setTextDatum(TL_DATUM);

}

// ===========================================================================
// OBRAZOVKA 3 - SIT A ZATEZ / SCREEN 3 - GRID AND LOAD
// ===========================================================================
void scrGridLoad() {
  halfGauge(84,  150, 68, 15, fabsf(v_grid_power), CFG_RANGE,
            v_grid_power > 1 ? C_GRID : C_DIM,
            fmtPower(fabsf(v_grid_power)),
            v_grid_power > 0 ? TR(T_GRID_IMP) : TR(T_GRID_NONE));
  halfGauge(236, 150, 68, 15, v_load_power, CFG_RANGE, C_LOAD,
            fmtPower(v_load_power), TR(T_HOUSE_LOAD));

  // zatizeni menice / inverter load
  tft.setTextColor(C_DIM, C_BG);
  tCz(TR(T_INV_LOAD), 6, 178);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_LOAD, C_BG);
  tAs(fmt("%.0f %%", v_load_pct), 314, 178, 2);
  tft.setTextDatum(TL_DATUM);
  bar(6, 196, 308, 14, v_load_pct / 100.0f,
      v_load_pct > 80 ? C_GRID : C_LOAD);

  const int sy = 216, sh = 48, sg = 6;
  statBox(6,   sy,           150, sh, TR(T_GRID_V),  fmt("%.0f V", v_grid_voltage), C_TXT);
  statBox(164, sy,           150, sh, TR(T_FREQ),    fmt("%.1f Hz", v_grid_freq), C_TXT);
  statBox(6,   sy+sh+sg,     150, sh, TR(T_INV_OUT),fmt("%.0f V", v_ac_out_voltage), C_TXT);
  statBox(164, sy+sh+sg,     150, sh, TR(T_OUT_HZ),    fmt("%.1f Hz", v_ac_out_freq), C_TXT);
  statBox(6,   sy+2*(sh+sg), 150, sh, TR(T_IMPORTED),     fmt("%.2f kWh", v_grid_energy_in), C_GRID);
  statBox(164, sy+2*(sh+sg), 150, sh, TR(T_EXPORTED),       fmt("%.2f kWh", v_grid_energy_out), C_BATT);
  // vlastni spotreba uz je jen radek textu, na ctvrtou dlazdici neni misto
  // EN: self-consumption is only a text row now, there is no room for a fourth tile
  int fy = sy + 3 * (sh + sg) + 4;
  tft.setTextColor(C_DIM, C_BG);
  tCz(TR(T_SELF_USE), 6, fy);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_TXT, C_BG);
  tCz(fmtPower(v_system_power), 314, fy);
  tft.setTextDatum(TL_DATUM);

  tft.setTextColor(C_DIM, C_BG);
  tCz(TR(T_MAX_TODAY), 6, 400);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_LOAD, C_BG);
  tCz(fmtPower((float)maxLoad), 314, 400);
  tft.setTextDatum(TL_DATUM);

}

// ===========================================================================
// OBRAZOVKA 4 - POCASI / SCREEN 4 - WEATHER
// ===========================================================================
void scrWeather() {
  int code = (int)w_code;

  // velka karta s aktualnim pocasim, obsah na stred
  // EN: large card with the current weather, content centred
  tft.fillRoundRect(6, 48, 308, 92, 8, C_CARD);
  tft.drawRoundRect(6, 48, 308, 92, 8, C_WEATH);

  if (w_cloud < 25)      icoSun(28, 74, C_PV);
  else if (w_cloud < 70) icoSunCloud(28, 74, C_DIM);
  else                   icoCloud(28, 74, C_DIM);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TXT, C_CARD);
  tCz(weatherText(code), 190, 62);
  // teplota velkym pismem, znak stupne ceskym fontem hned za ni
  // EN: temperature in the large font, the degree sign in the Czech font
  const char* tv = fmt("%.1f", w_temp);
  czOff();
  int wtv = tft.textWidth(tv, 6);
  czOn();
  int wdeg = tft.textWidth("°C");

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_WEATH, C_CARD);
  tAs(tv, 190 - (wtv + 4 + wdeg) / 2, 78, 6);
  tCz("°C", 190 - (wtv + 4 + wdeg) / 2 + wtv + 4, 92);
  tft.setTextDatum(MC_DATUM);

  static char stateBuf[32];
  snprintf(stateBuf, sizeof(stateBuf), "%s: %s", TR(T_STATE),
           w_is_day > 0.5f ? TR(T_DAY) : TR(T_NIGHT));
  tft.setTextColor(C_DIM, C_CARD);
  tCz(stateBuf, 190, 124);
  tft.setTextDatum(TL_DATUM);

  halfGauge(160, 240, 82, 17, w_cloud, 100, C_DIM,
            fmt("%.0f %%", w_cloud), TR(T_CLOUDS));

  const int sy = 262, sh = 48, sg = 5;
  statBox(6,   sy,           150, sh, TR(T_WIND),    fmt("%.1f km/h", w_wind), C_WEATH);
  statBox(164, sy,           150, sh, TR(T_IRRAD),   fmt("%.0f W/m2", w_irradiance), C_PV);
  statBox(6,   sy+sh+sg,     150, sh, TR(T_SUNRISE), hhmm(sunRise), C_PV);
  statBox(164, sy+sh+sg,     150, sh, TR(T_SUNSET),  hhmm(sunSet), C_LOAD);

  // delka dne a noci se odvodi z vychodu a zapadu
  // EN: day and night length follow from sunrise and sunset
  int dayLen = (sunRise >= 0 && sunSet > sunRise) ? sunSet - sunRise : -1;
  int nightLen = dayLen >= 0 ? 1440 - dayLen : -1;
  statBox(6,   sy+2*(sh+sg), 150, sh, TR(T_DAY_LEN),   hhmm(dayLen), C_PV);
  statBox(164, sy+2*(sh+sg), 150, sh, TR(T_NIGHT_LEN), hhmm(nightLen), C_DIM);
}

// ===========================================================================
// OBRAZOVKA 5 - MENIC / SCREEN 5 - INVERTER
// ===========================================================================
void scrInverter() {
  uint16_t tc = i_temp > 70 ? C_GRID : (i_temp > 55 ? C_PV : C_BATT);
  halfGaugeU(160, 160, 100, 20, i_temp, 100, tc,
             fmt("%.1f", i_temp), "°C", TR(T_INV_TEMP));

  if (i_temp > CFG_ALERT_TEMP) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_GRID, C_BG);
    tCz(TR(T_ALERT_TEMP), SCR_W / 2, 176);
    tft.setTextDatum(TL_DATUM);
  }

  tft.setTextColor(C_DIM, C_BG);
  tCz(TR(T_POWER_USE), 6, 186);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_LOAD, C_BG);
  tAs(fmt("%.0f %%", v_load_pct), 314, 186, 2);
  tft.setTextDatum(TL_DATUM);
  bar(6, 204, 308, 14, v_load_pct / 100.0f, v_load_pct > 80 ? C_GRID : C_LOAD);

  const int sy = 228, sh = 50, sg = 5;
  statBox(6,   sy,           150, sh, TR(T_MAX_POWER),     fmt("%.0f VA", i_max_va), C_TXT);
  statBox(164, sy,           150, sh, TR(T_APPARENT),fmt("%.0f VA", i_load_va), C_TXT);
  statBox(6,   sy+sh+sg,     150, sh, TR(T_BUS_V),    fmt("%.0f V", v_bus_voltage), C_TXT);
  statBox(164, sy+sh+sg,     150, sh, TR(T_MAX_CHG_A),fmt("%.0f A", i_max_charge_a), C_BATT);
  statBox(6,   sy+2*(sh+sg), 150, sh, TR(T_ABSORPTION),      fmt("%.2f V", i_absorption_v), C_BATT);
  statBox(164, sy+2*(sh+sg), 150, sh, TR(T_FLOAT),     fmt("%.2f V", i_float_v), C_BATT);

  int by = sy + 3 * (sh + sg);
  tft.setTextColor(C_DIM, C_BG);
  tCz(TR(T_SELF_USE), 6, by);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_TXT, C_BG);
  tAs(fmtPower(v_system_power), 314, by, 2);
  tft.setTextDatum(TL_DATUM);
}

// ===========================================================================
// OBRAZOVKA 6 - GRAFY za 24 hodin / SCREEN 6 - 24 HOUR CHARTS
// ===========================================================================
// levy okraj grafu, vlevo od nej je stupnice
// EN: left edge of the chart, the scale sits to its left
#define GR_L     30
#define GR_WIDTH 288          // sirka ramecku grafu / width of the chart frame
#define PLOT_X0  31           // prvni pixel dat uvnitr grafu / first data pixel inside the chart
// pixelu na jeden desetiminutovy vzorek
// EN: pixels per ten minute sample
#define PLOT_STEP 2

void plotDay(int gy, int gh, int16_t* data, int maxV, uint16_t col, bool bipolar) {
  int prevX = -1, prevY = -1;
  bool gap = false;
  for (int i = 0; i < DAY_N; i++) {
    if (!dHas[i]) { if (prevX >= 0) gap = true; continue; }
    int x = PLOT_X0 + i * PLOT_STEP;
    int y;
    if (bipolar) {
      int mid = gy + gh / 2;
      int h = (int)((long)constrain((int)data[i], -maxV, maxV) * (gh / 2 - 2) / maxV);
      y = mid - h;
    } else {
      int h = (int)((long)constrain((int)data[i], 0, maxV) * (gh - 2) / maxV);
      y = gy + gh - 1 - h;
    }
    if (prevX >= 0) tft.drawLine(prevX, prevY, x, y, gap ? C_GRID : col);
    else            tft.drawPixel(x, y, col);
    prevX = x; prevY = y;
    gap = false;
  }
}

void plotDaySoc(int gy, int gh, uint16_t col) {
  int prevX = -1, prevY = -1;
  bool gap = false;
  for (int i = 0; i < DAY_N; i++) {
    if (!dHas[i]) { if (prevX >= 0) gap = true; continue; }
    int x = PLOT_X0 + i * PLOT_STEP;
    int y = gy + gh - 1 - (int)((long)dSoc[i] * (gh - 2) / 100);
    if (prevX >= 0) tft.drawLine(prevX, prevY, x, y, gap ? C_GRID : col);
    else            tft.drawPixel(x, y, col);
    prevX = x; prevY = y;
    gap = false;
  }
}

// Graf hodnot v desetinach stupne s pevnym rozsahem. Pevna osa neumozni,
// aby stejna teplota vypadala pri dalsim dni jako jina.
// EN: Plot tenths of a degree on a fixed range, so equal temperatures keep
//     the same visual meaning from one day to the next.
void plotDayRange(int gy, int gh, int16_t* data, int minV, int maxV, uint16_t col) {
  if (maxV <= minV) return;
  int prevX = -1, prevY = -1;
  bool gap = false;
  for (int i = 0; i < DAY_N; i++) {
    if (!dHas[i]) { if (prevX >= 0) gap = true; continue; }
    int x = PLOT_X0 + i * PLOT_STEP;
    int v = constrain((int)data[i], minV, maxV);
    int y = gy + gh - 1 - (int)((long)(v - minV) * (gh - 2) / (maxV - minV));
    if (prevX >= 0) tft.drawLine(prevX, prevY, x, y, gap ? C_GRID : col);
    else            tft.drawPixel(x, y, col);
    prevX = x; prevY = y;
    gap = false;
  }
}

// ramecek, vodorovne vodici linky, delici cary po 6 hodinach a znacka "ted"
// EN: frame, horizontal guides, dividers every 6 hours and a "now" marker
void graphFrame(int gy, int gh, const char* title, const char* right, uint16_t col) {
  tft.setTextColor(C_DIM, C_BG);
  tCz(title, GR_L, gy - 18);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(col, C_BG);
  tCz(right, 314, gy - 18);
  tft.setTextDatum(TL_DATUM);

  tft.fillRect(GR_L, gy, GR_WIDTH, gh, C_BG);
  tft.drawRect(GR_L, gy, GR_WIDTH, gh, C_LINE);

  for (int k = 1; k < 4; k++) {
    int y = gy + gh * k / 4;
    for (int x = GR_L + 3; x < GR_L + GR_WIDTH - 2; x += 6) tft.drawPixel(x, y, C_CARD);
  }
  for (int h = 6; h < 24; h += 6) {
    int x = PLOT_X0 + (h * 6) * PLOT_STEP;
    for (int y = gy + 3; y < gy + gh - 2; y += 5) tft.drawPixel(x, y, C_CARD);
  }
  if (curSlot >= 0) {
    int x = PLOT_X0 + curSlot * PLOT_STEP;
    tft.drawFastVLine(x, gy + 1, gh - 2, C_LINE);
  }
}

// Svisla stupnice vlevo od grafu - tri popisky u horni, stredni a dolni cary.
// EN: Vertical scale left of the chart - three labels at the top, middle and bottom lines.
void yAxis(int gy, int gh, const char* top, const char* mid, const char* bot) {
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(C_DIM, C_BG);
  tCz(top, GR_L - 1, gy + 8);
  tCz(mid, GR_L - 1, gy + gh / 2);
  tCz(bot, GR_L - 1, gy + gh - 8);
  tft.setTextDatum(TL_DATUM);
}

void timeAxis(int y) {
  tft.setTextColor(C_DIM, C_BG);
  tft.setTextDatum(MC_DATUM);
  for (int h = 0; h <= 24; h += 6) {
    int x = PLOT_X0 + (h * 6) * PLOT_STEP;
    tCz(fmt("%.0f", (float)h), constrain(x, GR_L, GR_L + GR_WIDTH - 8), y);
  }
  tft.setTextDatum(TL_DATUM);
}

void scrGraphs() {
  if (!timeOk) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_DIM, C_BG);
    tCz(TR(T_WAIT_TIME), 160, 200);
    tCz(TR(T_NEED_TIME), 160, 224);
    tft.setTextDatum(TL_DATUM);
    return;
  }

  // spolecne meritko pro FVE a zatez / shared scale for PV and load
  // Meritko podle nejvyssi hodnoty, ktera se dnes objevila - jak v historii,
  // EN: Scale follows the highest value seen today - both in the history
  // tak v dennich extremech merenych z okamzitych hodnot.
  // EN: and in the daily extremes measured from instantaneous values.
  int maxPower = 500;
  if (peakPv  > maxPower) maxPower = peakPv;
  if (maxLoad > maxPower) maxPower = maxLoad;
  for (int i = 0; i < DAY_N; i++) {
    if (!dHas[i]) continue;
    if (dPv[i]   > maxPower) maxPower = dPv[i];
    if (dLoad[i] > maxPower) maxPower = dLoad[i];
  }
  maxPower = ((maxPower + 199) / 200) * 200;

  graphFrame(68, 90, TR(T_PV_AND_LOAD), fmt("max %.0f W", (float)maxPower), C_TXT);
  plotDay(68, 90, dPv,   maxPower, C_PV,   false);
  plotDay(68, 90, dLoad, maxPower, C_LOAD, false);
  yAxis(68, 90, fmt("%.0fk", maxPower / 1000.0f),
                fmt("%.0fk", maxPower / 2000.0f), "0");
  tft.setTextColor(C_PV, C_BG);   tCz(TR(T_PV), GR_L + 4, 160);
  tft.setTextColor(C_LOAD, C_BG); tCz(TR(T_LOAD), GR_L + 44, 160);

  // vykon baterie kolem nuly / battery power around zero
  int maxB = 300;
  if (maxBattPwr > maxB) maxB = maxBattPwr;
  for (int i = 0; i < DAY_N; i++) {
    if (!dHas[i]) continue;
    int a = dBatt[i] < 0 ? -dBatt[i] : dBatt[i];
    if (a > maxB) maxB = a;
  }
  maxB = ((maxB + 99) / 100) * 100;

  graphFrame(202, 90, TR(T_BATT_POWER), fmt("+-%.0f W", (float)maxB), C_BATT);
  tft.drawFastHLine(GR_L + 1, 202 + 45, GR_WIDTH - 2, C_LINE);
  plotDay(202, 90, dBatt, maxB, C_BATT, true);
  yAxis(202, 90, fmt("+%.0fk", maxB / 1000.0f), "0",
                 fmt("-%.0fk", maxB / 1000.0f));
  tft.setTextColor(C_DIM, C_BG);
  tCz(TR(T_CHG_ABOVE), GR_L, 294);

  // stav nabiti / state of charge
  graphFrame(336, 60, TR(T_SOC), fmt("%.0f %%", v_soc), C_BATT);
  plotDaySoc(336, 60, C_BATT);
  yAxis(336, 60, "100", "50", "0");

  timeAxis(406);      // popisky 0 / 6 / 12 / 18 / 24 hodin / labels 0 / 6 / 12 / 18 / 24 hours
}

// ===========================================================================
// OBRAZOVKA - TEPLOTY / SCREEN - TEMPERATURES
// ===========================================================================
void scrTemperatures() {
  if (!timeOk) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_DIM, C_BG);
    tCz(TR(T_WAIT_TIME), 160, 200);
    tCz(TR(T_NEED_TIME), 160, 224);
    tft.setTextDatum(TL_DATUM);
    return;
  }

  char invExt[64], outExt[64];
  if (minInvSlot >= 0) snprintf(invExt, sizeof(invExt), "%.1f/%02d:%02d  %.1f/%02d:%02d",
      minInvTemp / 10.0f, minInvSlot / 6, (minInvSlot % 6) * 10,
      maxInvTemp / 10.0f, maxInvSlot / 6, (maxInvSlot % 6) * 10);
  else snprintf(invExt, sizeof(invExt), "%s", TR(T_NO_DATA));
  if (minOutSlot >= 0) snprintf(outExt, sizeof(outExt), "%.1f/%02d:%02d  %.1f/%02d:%02d",
      minOutTemp / 10.0f, minOutSlot / 6, (minOutSlot % 6) * 10,
      maxOutTemp / 10.0f, maxOutSlot / 6, (maxOutSlot % 6) * 10);
  else snprintf(outExt, sizeof(outExt), "%s", TR(T_NO_DATA));

  tft.setTextColor(C_DIM, C_BG); tCz(TR(T_MIN_MAX), 6, 47);
  tft.setTextDatum(TR_DATUM); tft.setTextColor(C_PV, C_BG); tCz(invExt, 314, 47); tft.setTextDatum(TL_DATUM);
  graphFrame(82, 120, TR(T_INV_TEMP), fmt("%.1f °C", i_temp), C_PV);
  plotDayRange(82, 120, dInvTemp, 0, 1000, C_PV);
  yAxis(82, 120, "100", "50", "0");

  tft.setTextColor(C_DIM, C_BG); tCz(TR(T_MIN_MAX), 6, 237);
  tft.setTextDatum(TR_DATUM); tft.setTextColor(C_WEATH, C_BG); tCz(outExt, 314, 237); tft.setTextDatum(TL_DATUM);
  graphFrame(272, 120, TR(T_OUT_TEMP), fmt("%.1f °C", w_temp), C_WEATH);
  plotDayRange(272, 120, dOutTemp, -200, 400, C_WEATH);
  yAxis(272, 120, "40", "10", "-20");
  timeAxis(412);
}

// ===========================================================================
// OBRAZOVKA - DOBEH BATERIE / SCREEN - BATTERY RUNTIME
// ===========================================================================
void scrRuntime() {
  float h = battHours();
  bool  chg = v_batt_power >= 0;
  uint16_t col = chg ? C_BATT : (v_soc < CFG_ALERT_SOC ? C_GRID : C_PV);

  // velky cas ve tvaru h:mm, sedmisegmentove pismo ma dvojtecku
  // EN: large time as h:mm, the seven segment font has a colon
  tft.setTextDatum(MC_DATUM);
  if (h < 0) {
    tft.setTextColor(C_DIM, C_BG);
    tAs("--:--", SCR_W / 2, 110, 7);
    tft.setTextColor(C_DIM, C_BG);
    tCz(TR(T_IDLE), SCR_W / 2, 156);
  } else {
    char buf[12];
    snprintf(buf, sizeof(buf), "%d:%02d", (int)h, (int)((h - (int)h) * 60));
    tft.setTextColor(col, C_BG);
    tAs(buf, SCR_W / 2, 110, 7);
    tft.setTextColor(C_TXT, C_BG);
    tCz(chg ? TR(T_TO_FULL) : TR(T_TO_EMPTY), SCR_W / 2, 156);
  }
  tft.setTextDatum(TL_DATUM);

  // pruh stavu nabiti / state of charge bar
  bar(30, 180, 260, 16, v_soc / 100.0f, col);

  const int sy = 210, sh = 52, sg = 6;
  statBox(6,   sy,          150, sh, TR(T_SOC),      fmt("%.0f %%", v_soc), col);
  statBox(164, sy,          150, sh, TR(T_POWER),    fmtPower(fabsf(v_batt_power)), col);
  statBox(6,   sy+sh+sg,    150, sh, TR(T_CURRENT),  fmt("%.1f A", v_batt_current), C_TXT);
  statBox(164, sy+sh+sg,    150, sh, TR(T_CAPACITY), fmt("%.1f kWh", v_batt_capacity), C_TXT);
  statBox(6,   sy+2*(sh+sg),150, sh, TR(T_BATT_LEFT),
          fmt("%.2f kWh", v_batt_capacity * v_soc / 100.0f), C_TXT);
  statBox(164, sy+2*(sh+sg),150, sh, TR(T_LOAD), fmtPower(v_load_power), C_LOAD);

  // odhad pri aktualni zatezi domu, nezavisle na tom, co dela menic
  // EN: estimate at the current house load, regardless of what the inverter does
  int by = sy + 3 * (sh + sg);
  float hLoad = (v_load_power > 20 && v_batt_capacity > 0)
                ? (v_batt_capacity * v_soc / 100.0f) / (v_load_power / 1000.0f) : -1;
  tft.setTextColor(C_DIM, C_BG);
  tCz(TR(T_AT_LOAD), 6, by);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_TXT, C_BG);
  tCz(hoursMin(hLoad), 314, by);
  tft.setTextDatum(TL_DATUM);
}

// ===========================================================================
// OBRAZOVKA - HISTORIE POSLEDNICH 7 DNI / SCREEN - LAST 7 DAYS
// ===========================================================================
void scrHistory() {
  // nazev obdobi vlevo, napoveda k prepinani vpravo
  // EN: period name on the left, the switching hint on the right
  tft.setTextColor(C_DIM, C_BG);
  tCz(histRange == 0 ? TR(T_LAST7)
      : histRange == 1 ? TR(T_DAYS31) : TR(T_MONTHS12), 6, 48);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_LINE, C_BG);
  tCz(TR(T_TAP_PERIOD), 314, 48);
  tft.setTextDatum(TL_DATUM);

  // mesicni prehled ma data i pred prvni pulnoci, denni jeste ne
  // EN: the monthly view has data even before the first midnight, the daily one not yet
  if (hdCount == 0 && histRange != 2) {
    tft.setTextDatum(MC_DATUM);
    tCz(TR(T_NO_DATA), SCR_W / 2, 200);
    tft.setTextDatum(TL_DATUM);
    return;
  }

  // kolik sloupcu se kresli a odkud se berou hodnoty
  // EN: how many bars are drawn and where the values come from
  int n = histRange == 0 ? (hdCount < 7 ? hdCount : 7)
        : histRange == 1 ? hdCount : 12;
  if (n < 1) n = 1;
  int first = (hdPos - n + 2 * HIST_DAYS) % HIST_DAYS;

  // spolecne meritko a soucty v jednom pruchodu
  // EN: shared scale and totals in a single pass
  long sPv = 0, sLd = 0, sSv = 0, sBi = 0, sBo = 0;
  int maxV = 10;
  for (int i = 0; i < n; i++) {
    int idx = histRange == 2 ? i : (first + i) % HIST_DAYS;
    uint16_t pv = histRange == 2 ? hmPv[idx]   : hdPv[idx];
    uint16_t ld = histRange == 2 ? hmLoad[idx] : hdLoad[idx];
    if (pv > maxV) maxV = pv;
    if (ld > maxV) maxV = ld;
    sPv += pv;
    sLd += ld;
    sSv += histRange == 2 ? hmSave[idx] : hdSave[idx];
    sBi += histRange == 2 ? hmBIn[idx]  : hdBIn[idx];
    sBo += histRange == 2 ? hmBOut[idx] : hdBOut[idx];
  }

  const int gx = 6, gy = 74, gw = 308, gh = 140;
  tft.drawRect(gx, gy, gw, gh, C_LINE);
  for (int k = 1; k < 4; k++) {
    int y = gy + gh * k / 4;
    for (int x = gx + 3; x < gx + gw - 2; x += 6) tft.drawPixel(x, y, C_CARD);
  }


  // Sirka sloupce vychazi z poctu dnu. U 31 dnu zbyva na dvojici sloupcu
  // devet bodu, proto se pocita a hlida minimum.
  // EN: The bar width follows the number of days. With 31 days a pair of bars
  //     gets nine pixels, so the width is computed and kept above a minimum.
  int step = gw / n;
  int bw   = step / 2 - 2;
  if (bw < 1) bw = 1;

  // u hustych grafu se popisky nevejdou, kresli se jen kazdy druhy nebo ctvrty
  // EN: dense charts have no room for every label, only every 2nd or 4th
  int every = n <= 8 ? 1 : (n <= 16 ? 2 : 4);

  for (int i = 0; i < n; i++) {
    int idx = histRange == 2 ? i : (first + i) % HIST_DAYS;
    uint16_t pv = histRange == 2 ? hmPv[idx]   : hdPv[idx];
    uint16_t ld = histRange == 2 ? hmLoad[idx] : hdLoad[idx];
    int label   = histRange == 2 ? idx + 1     : hdDayNum[idx];
    int x = gx + i * step;

    int hPv = (int)((long)pv * (gh - 4) / maxV);
    int hLd = (int)((long)ld * (gh - 4) / maxV);

    tft.fillRect(x + 2,      gy + gh - hPv - 1, bw, hPv, C_PV);
    tft.fillRect(x + 3 + bw, gy + gh - hLd - 1, bw, hLd, C_LOAD);

    if (i % every == 0) {
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(C_DIM, C_BG);
      tCz(fmt("%.0f", (float)label), x + step / 2, gy + gh + 12);
      tft.setTextDatum(TL_DATUM);
    }
  }

  tft.setTextColor(C_PV, C_BG);   tCz(TR(T_PRODUCTION), 12, 236);
  tft.setTextColor(C_LOAD, C_BG); tCz(TR(T_CONSUMPT), 170, 236);

  // horni okraj meritka patri k legende, nahore uz je napoveda
  // EN: the top of the scale goes with the legend, the hint is up there
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_DIM, C_BG);
  tCz(fmt("%.1f kWh", maxV / 10.0f), 314, 236);
  tft.setTextDatum(TL_DATUM);

  const int sy = 260, sh = 50, sg = 5;
  statBox(6,   sy,           150, sh, TR(T_PRODUCTION), fmt("%.1f kWh", sPv / 10.0f), C_PV);
  statBox(164, sy,           150, sh, TR(T_CONSUMPT),   fmt("%.1f kWh", sLd / 10.0f), C_LOAD);
  statBox(6,   sy+sh+sg,     150, sh, TR(T_CHARGED),    fmt("%.1f kWh", sBi / 10.0f), C_BATT);
  statBox(164, sy+sh+sg,     150, sh, TR(T_DISCHARGED), fmt("%.1f kWh", sBo / 10.0f), C_PV);

  // Castka se kresli ceskym fontem - v ASCII pismu by "Kc" ani "zl" nebylo
  // videt, protoze hacek a prehlasku neobsahuje.
  // EN: The amount uses the Czech font - the ASCII font has no diacritics,
  //     so "Kc" or "zl" would not render at all.
  char sv[24];
  snprintf(sv, sizeof(sv), "%.0f %s", sSv / 10.0f, CFG_CURR);
  statBoxSmall(6, sy + 2 * (sh + sg), 308, sh, TR(T_SAVED), sv, C_BATT);
}

// ===========================================================================
// OBRAZOVKA - USPORY / SCREEN - SAVINGS
// ===========================================================================
void scrSavings() {
  // dnesek / today
  float dPvKwh = dayPv(), dSave = savedToday();

  // aktualni mesic / current month
  long mPv = 0, mLd = 0, mSv = 0;
  if (lastMon >= 0 && lastMon < 12) {
    mPv = hmPv[lastMon]; mLd = hmLoad[lastMon]; mSv = hmSave[lastMon];
  }
  float dLdKwh = dayLoad();
  // rok / year
  long yPv = 0, yLd = 0, ySv = 0;
  for (int i = 0; i < 12; i++) { yPv += hmPv[i]; yLd += hmLoad[i]; ySv += hmSave[i]; }

  // zahlavi tabulky / table header
  const int rowH = 46;
  int y = 52;
  // zahlavi musi byt zarovnane stejne jako hodnoty pod nim, tedy doprava
  // EN: the header must align like the values below it, that is to the right
  // Zahlavi ma barvu sveho sloupce. Seda pisma vedle sebe splyvala,
  // barva je od sebe odlisi lepe nez par bodu mezery navic.
  // EN: Each header takes its column's colour. Grey words ran together;
  //     colour separates them better than a few more pixels of gap.
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_PV, C_BG);   tCz(TR(T_PRODUCTION), 150, y);
  tft.setTextColor(C_LOAD, C_BG); tCz(TR(T_CONSUMPT), 236, y);
  tft.setTextColor(C_BATT, C_BG); tCz(TR(T_SAVED), 306, y);
  tft.setTextDatum(TL_DATUM);
  y += 26;

  const char* labels[3] = { TR(T_TODAY), TR(T_MONTH), TR(T_YEAR) };
  float pv[3] = { dPvKwh, mPv / 10.0f, yPv / 10.0f };
  float ld[3] = { dLdKwh, mLd / 10.0f, yLd / 10.0f };
  float sv[3] = { dSave,  mSv / 10.0f, ySv / 10.0f };

  for (int i = 0; i < 3; i++) {
    int ry = y + i * (rowH + 6);
    tft.fillRoundRect(6, ry, 308, rowH, 6, C_CARD);
    tft.drawRoundRect(6, ry, 308, rowH, 6, C_LINE);

    tft.setTextColor(C_TXT, C_CARD);
    tCz(labels[i], 14, ry + 15);

    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(C_PV, C_CARD);
    tCz(fmt("%.1f kWh", pv[i]), 150, ry + 15);
    tft.setTextColor(C_LOAD, C_CARD);
    tCz(fmt("%.1f kWh", ld[i]), 236, ry + 15);

    char b[24];
    snprintf(b, sizeof(b), "%.0f %s", sv[i], CFG_CURR);
    tft.setTextColor(C_BATT, C_CARD);
    tCz(b, 306, ry + 15);
    tft.setTextDatum(TL_DATUM);
  }

  // Sobestacnost pod tabulkou: pruh ukazuje, kolik spotreby pokryl vlastni
  // zdroj. Barva se meni podle toho, jak dobre na tom dum je.
  // EN: Self-sufficiency below the table: the bar shows how much of the
  //     consumption our own source covered. The colour follows how well we do.
  int uy = y + 3 * (rowH + 6) + 2;
  float ss = selfSufficiency();
  uint16_t ssCol = ss >= 80 ? C_BATT : (ss >= 50 ? C_PV : C_GRID);

  tft.setTextColor(C_DIM, C_BG);
  tCz(TR(T_SELF_SUFF), 6, uy);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(ssCol, C_BG);
  tCz(fmt("%.0f %%", ss), 314, uy);
  tft.setTextDatum(TL_DATUM);
  bar(6, uy + 18, 308, 14, ss / 100.0f, ssCol);

  // Porovnani s vcerejskem. Prvni den provozu se neni s cim porovnavat,
  // proto se radek vynecha.
  // EN: Comparison with yesterday. On the first day there is nothing to
  //     compare with, so the line is skipped.
  float chg = changePct(dSave, yesterdaySave());
  if (chg < 999) {
    tft.setTextColor(C_DIM, C_BG);
    tCz(TR(T_VS_YEST), 6, uy + 38);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(chg >= 0 ? C_BATT : C_GRID, C_BG);
    tCz(fmt("%+.0f %%", chg), 314, uy + 38);
    tft.setTextDatum(TL_DATUM);
  }

  // graf uspor po dnech / savings chart by day
  int gy = uy + 92;
  int gh = 70;
  tft.setTextColor(C_DIM, C_BG);
  tCz(TR(T_SAVED), 6, gy - 21);

  tft.drawRect(6, gy, 308, gh, C_LINE);
  if (hdCount > 0) {
    int n = hdCount < 14 ? hdCount : 14;
    int first = (hdPos - n + HIST_DAYS) % HIST_DAYS;
    int maxV = 10;
    for (int i = 0; i < n; i++) {
      int idx = (first + i) % HIST_DAYS;
      if (hdSave[idx] > maxV) maxV = hdSave[idx];
    }
    int step = 306 / n;
    tft.setTextDatum(MC_DATUM);
    for (int i = 0; i < n; i++) {
      int idx = (first + i) % HIST_DAYS;
      int hh = (int)((long)hdSave[idx] * (gh - 4) / maxV);
      tft.fillRect(7 + i * step + 1, gy + gh - hh - 1, step - 3, hh, C_BATT);

      // popisek dne pod sloupcem, u hustsich grafu jen kazdy druhy
      // EN: day label under the bar, every other one when the chart is dense
      if (n <= 10 || i % 2 == 0) {
        tft.setTextColor(C_DIM, C_BG);
        tCz(fmt("%.0f", (float)hdDayNum[idx]), 7 + i * step + step / 2, gy + gh + 12);
      }
    }
    tft.setTextDatum(TL_DATUM);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(C_DIM, C_BG);
    char b[24];
    snprintf(b, sizeof(b), "%.0f %s", maxV / 10.0f, CFG_CURR);
    tCz(b, 314, gy - 21);
    tft.setTextDatum(TL_DATUM);
  }
}

// ===========================================================================
// OBRAZOVKA - PREDIKCE USPORY / SCREEN - SAVINGS FORECAST
// ===========================================================================
float plannedSave(uint8_t mon) {
  return mon < 12 ? PLAN_OWN_KWH10[mon] * CFG_PRICE / 10.0f : 0;
}

float actualMonthSave(uint8_t mon) {
  float value = mon < 12 ? hmSave[mon] / 10.0f : 0;
  if (mon == lastMon) value += savedToday();
  return value;
}

void scrForecast() {
  int mon = lastMon;
  if (mon < 0 || mon > 11) mon = 0;
  float monthPlan = plannedSave(mon);
  float monthActual = actualMonthSave(mon);
  float yearPlan = 0, yearActual = 0;
  for (int i = 0; i < 12; i++) {
    yearPlan += plannedSave(i);
    yearActual += actualMonthSave(i);
  }

  statBoxSmall(6, 52, 150, 52, TR(T_PLAN), fmtMoney(monthPlan), C_PV);
  statBoxSmall(164, 52, 150, 52, TR(T_ACTUAL), fmtMoney(monthActual), C_BATT);
  statBoxSmall(6, 110, 150, 52, TR(T_YEAR_PLAN), fmtMoney(yearPlan), C_PV);
  statBoxSmall(164, 110, 150, 52, TR(T_SAVED), fmtMoney(yearActual), C_BATT);

  const int gx = 6, gy = 196, gw = 308, gh = 184;
  float maxV = 1;
  for (int i = 0; i < 12; i++) {
    float plan = plannedSave(i), actual = actualMonthSave(i);
    if (plan > maxV) maxV = plan;
    if (actual > maxV) maxV = actual;
  }
  maxV = ceilf(maxV / 50.0f) * 50.0f;
  tft.setTextColor(C_DIM, C_BG);
  tCz(TR(T_S_FORECAST), gx, gy - 22);
  tft.setTextDatum(TR_DATUM);
  tCz(fmtMoney(maxV), gx + gw, gy - 22);
  tft.setTextDatum(TL_DATUM);
  tft.drawRect(gx, gy, gw, gh, C_LINE);

  const int step = gw / 12;
  for (int i = 0; i < 12; i++) {
    int x = gx + i * step;
    int planH = (int)(plannedSave(i) * (gh - 4) / maxV);
    int actualH = (int)(actualMonthSave(i) * (gh - 4) / maxV);
    tft.fillRect(x + 2, gy + gh - planH - 1, 9, planH, C_PV);
    if (actualH > 0) tft.fillRect(x + 12, gy + gh - actualH - 1, 9, actualH, C_BATT);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_DIM, C_BG);
    tCz(fmt("%.0f", (float)(i + 1)), x + step / 2, gy + gh + 12);
  }
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_PV, C_BG);   tCz(TR(T_PLAN), 8, 402);
  tft.setTextColor(C_BATT, C_BG); tCz(TR(T_ACTUAL), 100, 402);
}

// ===========================================================================
// OBRAZOVKA - O APLIKACI / SCREEN - ABOUT
// ===========================================================================
void drawLogo(int y) {
  const uint16_t LOGO_BG   = C_PV;
  const uint16_t LOGO_DARK = 0x21AB;

  tft.fillRoundRect(18, y, 284, 86, 10, LOGO_BG);
  tft.fillRoundRect(34, y + 18, 116, 50, 6, LOGO_DARK);
  tft.fillRect(34, y + 72, 32, 6, C_GRID);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TXT, LOGO_DARK);
  tAs("solar", 92, y + 43, 4);
  tft.setTextColor(LOGO_DARK, LOGO_BG);
  tAs("assistant", 228, y + 43, 4);
  tft.setTextDatum(TL_DATUM);
}

void scrAbout() {
  drawLogo(50);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TXT, C_BG);
  tCz("Cabaj Tomáš  2026", SCR_W / 2, 152);
  tft.setTextDatum(TL_DATUM);

  const int sy = 176, sh = 50, sg = 6;
  statBoxSmall(6,   sy,       150, sh, TR(T_BOARD),    "ESP32-3248S035R", C_TXT);
  statBoxSmall(164, sy,       150, sh, TR(T_FIRMWARE), "v" FW_VERSION, C_PV);
  statBoxSmall(6,   sy+sh+sg, 150, sh, "Flash",
               fmt("%.0f MB", ESP.getFlashChipSize() / 1048576.0f), C_TXT);
  statBoxSmall(164, sy+sh+sg, 150, sh, TR(T_FREE_MEM),
               fmt("%.0f kB", ESP.getFreeHeap() / 1024.0f), C_TXT);

  int y = sy + 2 * (sh + sg) + 6;
  tft.fillRoundRect(6, y, 308, 98, 8, C_CARD);
  tft.drawRoundRect(6, y, 308, 98, 8, C_WEATH);

  tft.setTextColor(C_TXT, C_CARD);
  tCz(TR(T_CONTACT), 14, y + 6);

  // popisek vlevo, hodnota za nim - vejde se na jeden radek
  // EN: label on the left, value after it - fits on a single line
  tft.setTextColor(C_DIM, C_CARD);
  tCz("Web",    14, y + 28);
  tCz("GitHub", 14, y + 50);
  tCz("E-mail", 14, y + 72);

  tft.setTextColor(C_WEATH, C_CARD);
  tCz("www.pcprovas.cz",        82, y + 28);
  tCz("github.com/tomas-cabaj", 82, y + 50);
  tCz("t.cabaj@email.cz",       82, y + 72);
}

// ===========================================================================
// OTA - aktualizace firmwaru pres WiFi / OTA - firmware update over Wi-Fi
// ===========================================================================
void otaScreen(const char* text, uint16_t col) {
  // pri aktualizaci musi byt videt prubeh
  // EN: progress must stay visible during an update
  screenOn = true;
  PWM_WRITE(TFT_BL, 3, 255);
  tft.fillScreen(C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(col, C_BG);
  tCz(text, SCR_W / 2, 200);
  tft.setTextDatum(TL_DATUM);
}

void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOST);
  if (strlen(OTA_PASSWORD) > 0) ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    otaScreen(TR(T_OTA_RUN), C_WEATH);
  });

  ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
    static int last = -1;
    int pct = total ? (int)((uint64_t)done * 100 / total) : 0;
    if (pct == last) return;
    last = pct;

    bar(30, 240, 260, 18, pct / 100.0f, C_WEATH);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_TXT, C_BG);
    tAs(fmt("%.0f %%", (float)pct), SCR_W / 2, 280, 4);
    tft.setTextDatum(TL_DATUM);
  });

  ArduinoOTA.onEnd([]() {
    otaScreen(TR(T_OTA_DONE), C_BATT);
  });

  ArduinoOTA.onError([](ota_error_t e) {
    otaScreen(TR(T_OTA_ERR), C_GRID);
    Serial.printf("OTA chyba %u\n", e);
    delay(3000);
  });

  ArduinoOTA.begin();
  Serial.printf("OTA pripraveno, nazev '%s'%s\n", OTA_HOST,
                strlen(OTA_PASSWORD) > 0 ? ", chraneno heslem" : ", bez hesla");
}

// ===========================================================================
// OBRAZOVKA 8 - NASTAVENI 2 / SCREEN 8 - SETTINGS 2
// Vsechny hodnoty se ukladaji do NVS, takze prezijou restart.
// EN: Every value is stored in NVS, so they survive a reboot.
// Kalibrace dotyku je ulozena zvlast pro kazde otoceni displeje.
// EN: Touch calibration is stored separately for each display rotation.
// ===========================================================================
#define S2_Y0    46
#define S2_H     32
#define S2_STEP  34
#define S2_ROWS  11

void cfgClamp() {
  if (lang    >= LANG_N)           lang    = LANG_CZ;
  if (iFetch  >= OPT_N(OPT_FETCH))  iFetch  = 1;
  if (iSleep  >= OPT_N(OPT_SLEEP))  iSleep  = 2;
  if (iBright >= OPT_N(OPT_BRIGHT)) iBright = 3;
  if (iRange  >= OPT_N(OPT_RANGE))  iRange  = 1;
  if (iGreen  >= OPT_N(OPT_GREEN))  iGreen  = 2;
  if (iOrange >= OPT_N(OPT_ORANGE)) iOrange = 1;
  if (iRot    >= OPT_N(OPT_ROT))    iRot    = 0;
  if (iBlink  >= OPT_N(OPT_BLINK))  iBlink  = 0;
  if (iPrice  >= OPT_N(OPT_PRICE))  iPrice  = 10;
  if (iCurr   >= OPT_N(OPT_CURR))   iCurr   = 0;
  if (iAlertSoc  >= OPT_N(OPT_ALERT_SOC))  iAlertSoc = 2;
  if (iAlertTemp >= OPT_N(OPT_ALERT_TEMP)) iAlertTemp = 3;
  if (iStale     >= OPT_N(OPT_STALE))      iStale = 1;
  if (iErrLed    >= OPT_N(OPT_ERR_LED))    iErrLed = 1;
}

void cfgLoad() {
  lang    = prefs.getUChar("lang",    LANG_CZ);
  iFetch  = prefs.getUChar("iFetch",  1);
  iSleep  = prefs.getUChar("iSleep",  2);
  iBright = prefs.getUChar("iBright", 3);
  iRange  = prefs.getUChar("iRange",  1);
  iGreen  = prefs.getUChar("iGreen",  2);
  iOrange = prefs.getUChar("iOrange", 1);
  iRot    = prefs.getUChar("iRot",    0);
  iBlink  = prefs.getUChar("iBlink",  0);
  iPrice  = prefs.getUChar("iPrice", 10);
  iCurr   = prefs.getUChar("iCurr",   0);
  iAlertSoc  = prefs.getUChar("iASoc", 2);
  iAlertTemp = prefs.getUChar("iATemp", 3);
  iStale     = prefs.getUChar("iStale", 1);
  iErrLed    = prefs.getUChar("iELed", 1);
  cfgClamp();
}

void cfgSave() {
  prefs.putUChar("lang",    lang);
  prefs.putUChar("iFetch",  iFetch);
  prefs.putUChar("iSleep",  iSleep);
  prefs.putUChar("iBright", iBright);
  prefs.putUChar("iRange",  iRange);
  prefs.putUChar("iGreen",  iGreen);
  prefs.putUChar("iOrange", iOrange);
  prefs.putUChar("iRot",    iRot);
  prefs.putUChar("iBlink",  iBlink);
  prefs.putUChar("iPrice",  iPrice);
  prefs.putUChar("iCurr",   iCurr);
  prefs.putUChar("iASoc",  iAlertSoc);
  prefs.putUChar("iATemp", iAlertTemp);
  prefs.putUChar("iStale", iStale);
  prefs.putUChar("iELed",  iErrLed);
}

// text hodnoty pro dany radek / value text for the given row
const char* cfgValue(int row) {
  switch (row) {
    case 0: return LANG_NAME[lang];
    case 1: return fmt("%.0f s", CFG_FETCH / 1000.0f);
    case 2: return CFG_SLEEP == 0 ? TR(T_OFF)
                                  : fmt("%.0f min", CFG_SLEEP / 60000.0f);
    case 3: return fmt("%.0f %%", CFG_BRIGHT * 100.0f / 255.0f);
    case 4: return fmt("%.1f kW", CFG_RANGE / 1000.0f);
    case 5: return fmt("%.0f W", (float)CFG_GREEN);
    case 6: return fmt("%.0f W", (float)CFG_ORANGE);
    case 7: return CFG_ROT == 0 ? "0" : "180";
    case 8: return CFG_BLINK == 0 ? TR(T_OFF) : fmt("%.0f W", (float)CFG_BLINK);
    case 9: return fmt("%.2f", CFG_PRICE);
    default: return CFG_CURR;
  }
}

int cfgLabel(int row) {
  switch (row) {
    case 0:  return T_LANGUAGE;
    case 1:  return T_REFRESH;
    case 2:  return T_SLEEP;
    case 3:  return T_BRIGHT;
    case 4:  return T_RANGE;
    case 5:  return T_LED_GREEN;
    case 6:  return T_LED_ORANGE;
    case 7:  return T_ROTATION;
    case 8:  return T_LED_BLINK;
    case 9:  return T_PRICE;
    default: return T_CURRENCY;
  }
}

void scrSettings2() {
  for (int i = 0; i < S2_ROWS; i++) {
    int y = S2_Y0 + i * S2_STEP;
    tft.fillRoundRect(6, y, 308, S2_H, 6, C_CARD);
    tft.drawRoundRect(6, y, 308, S2_H, 6, i == 0 ? C_PV : C_LINE);

    tft.setTextColor(C_TXT, C_CARD);
    tCz(TR(cfgLabel(i)), 14, y + 8);

    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(C_PV, C_CARD);
    tCz(cfgValue(i), 306, y + 8);
    tft.setTextDatum(TL_DATUM);
  }
}

// posun na dalsi hodnotu v radku a okamzite uplatneni
// EN: advance the row to the next value and apply it at once
void cfgNext(int row) {
  switch (row) {
    case 0: lang    = (lang + 1) % LANG_N;                  break;
    case 1: iFetch  = (iFetch + 1)  % OPT_N(OPT_FETCH);     break;
    case 2: iSleep  = (iSleep + 1)  % OPT_N(OPT_SLEEP);     break;
    case 3: iBright = (iBright + 1) % OPT_N(OPT_BRIGHT);
            PWM_WRITE(TFT_BL, 3, CFG_BRIGHT);               break;
    case 4: iRange  = (iRange + 1)  % OPT_N(OPT_RANGE);     break;
    case 5: iGreen  = (iGreen + 1)  % OPT_N(OPT_GREEN);
            updateLoadLed();                                break;
    case 6: iOrange = (iOrange + 1) % OPT_N(OPT_ORANGE);
            updateLoadLed();                                break;
    case 7:
      iRot = (iRot + 1) % OPT_N(OPT_ROT);
      tft.setRotation(CFG_ROT);
      tft.fillScreen(C_BG);
      // kalibrace dotyku plati jen pro jedno otoceni
      // EN: touch calibration is valid for one rotation only
      loadOrCalibrate();
      break;
    case 8: iBlink = (iBlink + 1) % OPT_N(OPT_BLINK); updateLoadLed(); break;
    case 9: iPrice = (iPrice + 1) % OPT_N(OPT_PRICE);                  break;
    default: iCurr = (iCurr + 1) % OPT_N(OPT_CURR);                    break;
  }
  cfgSave();
  drawScreen();
}

#define S3_Y0  64
#define S3_H   54
#define S3_STEP 62
#define S3_ROWS 4

const char* errLedName() {
  switch (CFG_ERR_LED) {
    case 0: return TR(T_OFF);
    case 1: return TR(T_RED);
    case 2: return TR(T_BLUE);
    case 3: return TR(T_ORANGE);
    default: return TR(T_PURPLE);
  }
}

const char* cfg3Value(int row) {
  switch (row) {
    case 0: return fmt("%.0f %%", (float)CFG_ALERT_SOC);
    case 1: return fmt("%.0f °C", (float)CFG_ALERT_TEMP);
    case 2: return fmt("%.0f min", CFG_STALE / 60000.0f);
    default: return errLedName();
  }
}

int cfg3Label(int row) {
  switch (row) {
    case 0: return T_ALERT_SOC_LIMIT;
    case 1: return T_ALERT_TEMP_LIMIT;
    case 2: return T_ALERT_OFFLINE;
    default: return T_ERR_LED;
  }
}

void scrSettings3() {
  for (int i = 0; i < S3_ROWS; i++) {
    int y = S3_Y0 + i * S3_STEP;
    tft.fillRoundRect(6, y, 308, S3_H, 6, C_CARD);
    tft.drawRoundRect(6, y, 308, S3_H, 6, C_LINE);
    tft.setTextColor(C_TXT, C_CARD); tCz(TR(cfg3Label(i)), 14, y + 9);
    tft.setTextDatum(TR_DATUM); tft.setTextColor(C_PV, C_CARD); tCz(cfg3Value(i), 306, y + 9); tft.setTextDatum(TL_DATUM);
  }
  tft.setTextColor(C_DIM, C_BG); tCz(TR(T_TAP_HINT), 6, 342);
}

void cfg3Next(int row) {
  switch (row) {
    case 0: iAlertSoc = (iAlertSoc + 1) % OPT_N(OPT_ALERT_SOC); break;
    case 1: iAlertTemp = (iAlertTemp + 1) % OPT_N(OPT_ALERT_TEMP); break;
    case 2: iStale = (iStale + 1) % OPT_N(OPT_STALE); break;
    default: iErrLed = (iErrLed + 1) % OPT_N(OPT_ERR_LED); updateLoadLed(); break;
  }
  cfgSave(); drawScreen();
}

void scrErrors() {
  if (errCount == 0) {
    tft.setTextDatum(MC_DATUM); tft.setTextColor(C_BATT, C_BG); tCz(TR(T_NO_ERRORS), 160, 180); tft.setTextDatum(TL_DATUM);
  } else {
    int shown = errCount < 5 ? errCount : 5;
    for (int i = 0; i < shown; i++) {
      ErrorEntry& e = errorLog[(errPos + ERR_LOG_N - 1 - i) % ERR_LOG_N];
      int y = 50 + i * 56;
      tft.fillRoundRect(6, y, 308, 50, 6, C_CARD);
      tft.setTextColor(e.type == ERR_SOC || e.type == ERR_TEMP ? C_GRID : C_PV, C_CARD);
      tCz(errorName(e.type), 14, y + 5);
      char buf[38];
      if (e.hour < 24 && e.durationMin > 0) snprintf(buf, sizeof(buf), "%02u:%02u  %u min / %u", e.hour, e.minute, e.durationMin, e.count);
      else if (e.hour < 24) snprintf(buf, sizeof(buf), "%02u:%02u  %s %u", e.hour, e.minute, TR(T_FAILURES), e.count);
      else snprintf(buf, sizeof(buf), "%s %u", TR(T_FAILURES), e.count);
      tft.setTextColor(C_DIM, C_CARD); tCz(buf, 14, y + 27);
    }
  }
  tft.fillRoundRect(6, 354, 308, 50, 6, C_CARD); tft.drawRoundRect(6, 354, 308, 50, 6, C_GRID);
  tft.setTextDatum(MC_DATUM); tft.setTextColor(C_GRID, C_CARD); tCz(TR(T_CLEAR_ERRORS), 160, 379); tft.setTextDatum(TL_DATUM);
}

// ===========================================================================
// OBRAZOVKA 7 - NASTAVENI / SCREEN 7 - SETTINGS
// ===========================================================================
#define SET_BTN_Y  282
#define SET_BTN_H   52
#define SET_BTN_W  150
#define SET_BTN_X    6         // vypis hodnot / value dump
#define SET_BT2_X  164         // test spojeni / link test
#define SET_BT3_Y  342         // sken site, na celou sirku / network scan, full width
#define SET_BT3_H   42

void scrSettings() {
  const int sh = 50, sg = 6;
  int y = 52;

  // IP adresa je dlouha, proto na celou sirku
  // EN: the IP address is long, hence full width
  statBoxSmall(6, y, 308, sh, TR(T_BOARD_IP),
               wifiOk ? WiFi.localIP().toString().c_str() : TR(T_NOT_CONN), C_TXT);
  y += sh + sg;

  statBox(6,   y, 150, sh, TR(T_SIGNAL),   fmt("%.0f dBm", (float)WiFi.RSSI()), C_TXT);
  statBox(164, y, 150, sh, TR(T_UPTIME), uptimeText(), C_TXT);
  y += sh + sg;

  statBox(6,   y, 150, sh, TR(T_FREE_MEM), fmt("%.0f kB", ESP.getFreeHeap() / 1024.0f), C_TXT);
  statBox(164, y, 150, sh, "Firmware",         "v" FW_VERSION, C_TXT);
  y += sh + sg;

  statBoxSmall(6,   y, 150, sh, TR(T_WIFI_NET), ssid, C_DIM);
  statBoxSmall(164, y, 150, sh, TR(T_OTA_NAME), OTA_HOST, C_DIM);

  // tlacitko pro vypis vsech hodnot do Serialu
  // EN: button that dumps every value to Serial
  tft.fillRoundRect(SET_BTN_X, SET_BTN_Y, SET_BTN_W, SET_BTN_H, 8, C_CARD);
  tft.drawRoundRect(SET_BTN_X, SET_BTN_Y, SET_BTN_W, SET_BTN_H, 8, C_WEATH);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_WEATH, C_CARD);
  tCz(TR(T_DUMP), SET_BTN_X + SET_BTN_W / 2, SET_BTN_Y + 16);
  tft.setTextColor(C_DIM, C_CARD);
  tCz(TR(T_TO_SERIAL), SET_BTN_X + SET_BTN_W / 2, SET_BTN_Y + 34);

  // tlacitko pro test sitoveho spojeni / button for the network link test
  tft.fillRoundRect(SET_BT2_X, SET_BTN_Y, SET_BTN_W, SET_BTN_H, 8, C_CARD);
  tft.drawRoundRect(SET_BT2_X, SET_BTN_Y, SET_BTN_W, SET_BTN_H, 8, C_PV);
  tft.setTextColor(C_PV, C_CARD);
  tCz(TR(T_TEST_CONN), SET_BT2_X + SET_BTN_W / 2, SET_BTN_Y + 16);
  tft.setTextColor(C_DIM, C_CARD);
  tCz(TR(T_TGT_PORTS), SET_BT2_X + SET_BTN_W / 2, SET_BTN_Y + 34);

  // tlacitko pro sken cele podsite / button for scanning the whole subnet
  tft.fillRoundRect(6, SET_BT3_Y, 308, SET_BT3_H, 8, C_CARD);
  tft.drawRoundRect(6, SET_BT3_Y, 308, SET_BT3_H, 8, C_LOAD);
  tft.setTextColor(C_LOAD, C_CARD);
  tCz(TR(T_NET_SCAN), SCR_W / 2, SET_BT3_Y + 21);
  tft.setTextDatum(TL_DATUM);

  // stav posledniho vypisu / status of the last dump
  tft.setTextColor(C_DIM, C_BG);
  tCz(setMsg[0] ? setMsg : TR(T_NOTHING), SCR_W / 2, SET_BT3_Y + SET_BT3_H + 14);
  tft.setTextDatum(TL_DATUM);
}

// ===========================================================================
// VYCHOD A ZAPAD SLUNCE / SUNRISE AND SUNSET
// Zjednoduseny vypocet z deklinace Slunce a rovnice casu, presnost par minut.
// EN: Simplified calculation from solar declination and the equation of time, accurate to minutes.
// Nepotrebuje sit, staci datum a poloha.
// EN: No network needed, the date and position are enough.
// ===========================================================================
void computeSun(int yday, int tzMinutes) {
  const float RAD = 0.0174533f;

  float decl = 23.45f * sinf(RAD * 360.0f * (284 + yday) / 365.0f);
  float B    = RAD * 360.0f * (yday - 81) / 364.0f;
  float eot  = 9.87f * sinf(2 * B) - 7.53f * cosf(B) - 1.5f * sinf(B);

  float cosH = (sinf(RAD * -0.83f) - sinf(RAD * GEO_LAT) * sinf(RAD * decl)) /
               (cosf(RAD * GEO_LAT) * cosf(RAD * decl));

  if (cosH > 1.0f)  { sunRise = sunSet = -1; return; }   // polarni noc / polar night
  if (cosH < -1.0f) { sunRise = 0; sunSet = 1439; return; }  // polarni den / polar day

  // polovina delky dne ve stupnich
  // EN: half the day length in degrees
  float H = acosf(cosH) / RAD;
  float noon = 720.0f - 4.0f * GEO_LON - eot + tzMinutes;

  sunRise = (int)(noon - 4.0f * H);
  sunSet  = (int)(noon + 4.0f * H);
}

// posun mistniho casu vuci UTC v minutach
// EN: local time offset from UTC in minutes
int tzOffsetMinutes() {
  time_t now;
  time(&now);
  struct tm lt, gt;
  localtime_r(&now, &lt);
  gmtime_r(&now, &gt);
  int off = (lt.tm_hour - gt.tm_hour) * 60 + (lt.tm_min - gt.tm_min);
  if (off >  720) off -= 1440;
  if (off < -720) off += 1440;
  return off;
}

// cas v minutach od pulnoci jako "HH:MM"
// EN: time in minutes from midnight as "HH:MM"
const char* hhmm(int minutes) {
  fidx = (fidx + 1) % 8;
  if (minutes < 0) snprintf(fbuf[fidx], 24, "--:--");
  else             snprintf(fbuf[fidx], 24, "%02d:%02d", minutes / 60, minutes % 60);
  return fbuf[fidx];
}

// doba jako "4 h 20 min", pro kratke useky jen minuty
// EN: duration as "4 h 20 min", minutes only for short spans
const char* hoursMin(float hours) {
  fidx = (fidx + 1) % 8;
  if (hours < 0 || hours > 240)   snprintf(fbuf[fidx], 24, "--");
  else if (hours < 1)             snprintf(fbuf[fidx], 24, "%d min", (int)(hours * 60));
  else                            snprintf(fbuf[fidx], 24, "%d h %02d min",
                                           (int)hours, (int)((hours - (int)hours) * 60));
  return fbuf[fidx];
}

// ===========================================================================
// DOBEH BATERIE / BATTERY RUNTIME
// Kladny vysledek = hodin do plneho nabiti, zaporny = do vybiti.
// EN: Positive result = hours to full charge, negative = to empty.
// ===========================================================================
float battHours() {
  float p = fabsf(v_batt_power);
  // klid, nema smysl pocitat
  // EN: idle, no point calculating
  if (p < 20 || v_batt_capacity <= 0) return -1;

  float kwh = v_batt_power >= 0
              ? v_batt_capacity * (100.0f - v_soc) / 100.0f   // do plna / to full
              : v_batt_capacity * v_soc / 100.0f;             // do vybiti / to empty
  return kwh / (p / 1000.0f);
}

// ===========================================================================
// USPORNY REZIM DISPLEJE / DISPLAY SLEEP MODE
// Po CFG_SLEEP necinnosti zhasne podsviceni. Dotykovy radic bezi dal, takze
// EN: After CFG_SLEEP of inactivity the backlight goes off. Touch keeps running, so
// prvni dotek displej jen probudi a uz nic dalsiho neprovede.
// EN: the first touch only wakes the display and does nothing else.
// Stahovani dat, historie i signalizacni dioda bezi bez zmeny.
// EN: Data fetching, history and the signalling LED keep running unchanged.
// ===========================================================================
void screenSleep() {
  if (!screenOn) return;
  screenOn = false;
  PWM_WRITE(TFT_BL, 3, 0);
  Serial.println("displej zhasnut (necinnost)");
}

void screenWake() {
  lastTouch = millis();
  if (screenOn) return;
  screenOn = true;
  PWM_WRITE(TFT_BL, 3, CFG_BRIGHT);
  drawScreen();
  Serial.println("displej probuzen");
}

// ===========================================================================
// HLAVICKA A OVLADACI LISTA / HEADER AND NAVIGATION BAR
// ===========================================================================
const char* screenName(int i) {
  switch (i) {
    case 0:  return TR(T_APP);
    case 1:  return TR(T_S_BATT);
    case 2:  return TR(T_S_RUNTIME);
    case 3:  return TR(T_S_SOLAR);
    case 4:  return TR(T_S_GRID);
    case 5:  return TR(T_S_WEATH);
    case 6:  return TR(T_S_INV);
    case 7:  return TR(T_S_CHART);
    case 8:  return TR(T_S_TEMP);
    case 9:  return TR(T_S_HISTORY);
    case 10: return TR(T_S_SAVINGS);
    case 11: return TR(T_S_FORECAST);
    case 12: return TR(T_S_SET);
    case 13: return TR(T_S_SET2);
    case 14: return TR(T_S_ERRORS);
    case 15: return TR(T_S_SET3);
    default: return TR(T_S_ABOUT);
  }
}

void drawHeader() {
  updateClock();
  // zhasnuto - nema smysl posilat data na displej
  // EN: asleep - no point sending data to the display
  if (!screenOn) return;
  bool stale = dataStale();

  // pri starych datech zcervena cela hlavicka, aby to neslo prehlednout
  // EN: on stale data the whole header turns red so it cannot be missed
  uint16_t hdrBg = stale ? C_STALE : (alertActive() ? C_ALERT : C_CARD);
  tft.fillRect(0, 0, SCR_W, HDR_H, hdrBg);
  tft.drawFastHLine(0, HDR_H, SCR_W, C_LINE);

  tft.setTextColor(C_TXT, hdrBg);
  tCz(screenName(screen), 8, 10);

  // Hodiny uprostred. Kdyby se trefily do nazvu, posunou se doprava, ale
  // EN: Clock in the middle. If it would hit the title it shifts right, but
  // nikdy ne az pod pole s odpoctem vpravo - tam by se prekryly.
  // EN: never as far as the countdown field on the right - they would overlap.
  czOn();
  int wTitle = tft.textWidth(screenName(screen));
  int wClock = tft.textWidth(clockStr);
  int cxMin = 8 + wTitle + 8 + wClock / 2;      // hned za nazvem / right after the title
  int cxMax = SCR_W - 132 - wClock / 2;         // jeste pred odpoctem / still before the countdown
  int cxClock = SCR_W / 2;
  if (cxClock < cxMin) cxClock = cxMin;
  if (cxClock > cxMax) cxClock = cxMax;

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(timeOk ? C_TXT : C_DIM, hdrBg);
  tCz(clockStr, cxClock, 19);
  tft.setTextDatum(TL_DATUM);

  // tecka stavu spojeni / link status dot
  uint16_t dot = !wifiOk ? C_GRID : (stale ? C_GRID : (dataOk ? C_BATT : C_PV));
  tft.fillCircle(SCR_W - 14, 20, 5, dot);

  // odpocet do dalsi obnovy, pri starych datech misto nej jejich stari
  // EN: countdown to the next refresh, replaced by the data age when stale
  uint32_t elapsed = millis() - lastFetch;
  if (elapsed > CFG_FETCH) elapsed = CFG_FETCH;
  int left = (CFG_FETCH - elapsed + 999) / 1000;

  char buf[28];
  if (!wifiOk)      snprintf(buf, sizeof(buf), TR(T_NO_WIFI));
  else if (stale)   snprintf(buf, sizeof(buf), TR(T_OLD_MIN),
                             haveFetch ? (millis() - lastOkFetch) / 60000 : 0);
  else              snprintf(buf, sizeof(buf), TR(T_IN_SEC), left);

  tft.fillRect(SCR_W - 130, 12, 105, 18, hdrBg);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(stale ? C_TXT : C_DIM, hdrBg);
  tCz(buf, SCR_W - 26, 12);
  tft.setTextDatum(TL_DATUM);

  // Dva tenke pruhy po spodnim okraji hlavicky:
  // EN: Two thin bars along the bottom edge of the header:
  // horni zeleny = do dalsiho nacteni dat, dolni modry = do zhasnuti displeje.
  // EN: upper green = to the next fetch, lower blue = to the display going dark.
  int pw = (int)((long)elapsed * SCR_W / CFG_FETCH);
  tft.fillRect(0, HDR_H - 6, SCR_W, 3, hdrBg);
  if (pw > 0) tft.fillRect(0, HDR_H - 6, pw, 3, dot);

  tft.fillRect(0, HDR_H - 3, SCR_W, 3, hdrBg);
  if (CFG_SLEEP > 0) {
    uint32_t idle = millis() - lastTouch;
    if (idle > CFG_SLEEP) idle = CFG_SLEEP;
    int sw = (int)((long)(CFG_SLEEP - idle) * SCR_W / CFG_SLEEP);
    if (sw > 0) tft.fillRect(0, HDR_H - 3, sw, 3, C_LOAD);
  }
}

void drawNav() {
  tft.fillRect(0, NAV_Y - 2, SCR_W, SCR_H - NAV_Y + 2, C_BG);
  tft.drawFastHLine(0, NAV_Y - 2, SCR_W, C_LINE);

  const int y = NAV_Y + 2, h = NAV_H - 6;

  // vlevo / left
  tft.fillRoundRect(8, y, 88, h, 6, C_CARD);
  tft.drawRoundRect(8, y, 88, h, 6, C_LINE);
  tft.fillTriangle(60, y + 8, 60, y + h - 8, 40, y + h / 2, C_TXT);

  // domu - na uvodni strance zvyraznene
  // EN: home - highlighted on the overview screen
  tft.fillRoundRect(116, y, 88, h, 6, screen == 0 ? C_LINE : C_CARD);
  tft.drawRoundRect(116, y, 88, h, 6, screen == 0 ? C_PV : C_LINE);
  icoHouse(145, y + 4, C_TXT);

  // vpravo / right
  tft.fillRoundRect(224, y, 88, h, 6, C_CARD);
  tft.drawRoundRect(224, y, 88, h, 6, C_LINE);
  tft.fillTriangle(260, y + 8, 260, y + h - 8, 280, y + h / 2, C_TXT);

  // indikator stranky / page indicator
  int dots = SCREENS;
  int dx = SCR_W / 2 - (dots * 10) / 2;
  for (int i = 0; i < dots; i++)
    tft.fillCircle(dx + i * 10 + 4, NAV_Y - 9, 2, i == screen ? C_TXT : C_LINE);
}

// Obsah aktualni obrazovky. / Content of the current screen.
void drawContent() {
  switch (screen) {
    case 0:  scrOverview();  break;
    case 1:  scrBattery();   break;
    case 2:  scrRuntime();   break;
    case 3:  scrSolar();     break;
    case 4:  scrGridLoad();  break;
    case 5:  scrWeather();   break;
    case 6:  scrInverter();  break;
    case 7:  scrGraphs();    break;
    case 8:  scrTemperatures(); break;
    case 9:  scrHistory();   break;
    case 10: scrSavings();   break;
    case 11: scrForecast();  break;
    case 12: scrSettings();  break;
    case 13: scrSettings2(); break;
    case 14: scrErrors();    break;
    case 15: scrSettings3(); break;
    default: scrAbout();     break;
  }
}

// Ramecek kolem obsahu, cervene pri starych datech nebo upozorneni.
// EN: Frame around the content, red on stale data or an alert.
void drawFrame() {
  if (!dataStale() && !alertActive()) return;
  tft.drawRect(0, CONT_Y, SCR_W, CONT_H, C_GRID);
  tft.drawRect(1, CONT_Y + 1, SCR_W - 2, CONT_H - 2, C_GRID);
}

// Prekresleni obrazovky. Plocha se vzdy nejdriv smaze, jinak by po kratsi
// EN: Screen redraw. The area is always cleared first, otherwise a shorter
// hodnote zbyvaly zbytky te predchozi.
// EN: value would leave remnants of the previous one.
void drawScreen() {
  if (!screenOn) return;
  tft.fillRect(0, CONT_Y, SCR_W, CONT_H, C_BG);
  drawContent();
  drawHeader();
  drawNav();
  drawFrame();
}

// ===========================================================================
// HISTORIE / HISTORY
// ===========================================================================
// Prumeruje hodnoty do desetiminutovych useku podle skutecneho casu.
// EN: Averages values into ten minute slots according to real time.
// Rozdelany usek se zapisuje prubezne, aby graf rostl zive.
// EN: The slot in progress is written continuously so the chart grows live.
void pushHistory() {
  struct tm t;
  // bez casu nevime, kam vzorek patri
  // EN: without the time we do not know where the sample belongs
  if (!getLocalTime(&t, 5)) return;

  // po startu zkusit navazat na flash
  // EN: after boot, try to pick up from flash
  if (!histLoaded) {
    histLoaded = true;
    histLoad();
  }

  // novy den - zacit s cistym grafem
  // EN: new day - start with a clean chart
  if (t.tm_yday != curDay) {
    // predchozi den do historie
    // EN: previous day into the history
    if (curDay >= 0) closeDay(t.tm_mday, lastMon);
    baseReset();   // novy den zacina od nuly / the new day starts from zero
    memset(dHas, 0, sizeof(dHas));
    curDay = t.tm_yday;
    curSlot = -1;
    peakPv = maxLoad = maxBattPwr = 0;
    minSoc = 100;
    minInvTemp = minOutTemp = 32767;
    maxInvTemp = maxOutTemp = -32768;
    minInvSlot = maxInvSlot = minOutSlot = maxOutSlot = -1;
    computeSun(t.tm_yday, tzOffsetMinutes());
  }

  lastMon = t.tm_mon;

  // Mesicni soucty patri vzdy jen do jednoho kalendarniho roku. Bez resetu
  // by se po dalsim roce scitaly znovu a uint16_t by nakonec pretekl.
  // EN: Monthly totals belong to one calendar year. Without this reset they
  //     would accumulate into the next year and eventually overflow uint16_t.
  if (sumYear != t.tm_year) {
    memset(hmPv, 0, sizeof(hmPv));
    memset(hmLoad, 0, sizeof(hmLoad));
    memset(hmSave, 0, sizeof(hmSave));
    memset(hmBIn, 0, sizeof(hmBIn));
    memset(hmBOut, 0, sizeof(hmBOut));
    memset(hmGrid, 0, sizeof(hmGrid));
    sumYear = t.tm_year;
    sumSave();
  }

  // pri prvnim behu se zaklad teprve ustavi, ten den bude neuplny
  // EN: on the first run the baseline is only being set, that day stays partial
  if (!baseValid && v_load_energy > 0) baseReset();

  // vychod a zapad slunce se pocitaji jednou za den
  // EN: sunrise and sunset are calculated once a day
  if (sunRise < 0) computeSun(t.tm_yday, tzOffsetMinutes());

  // denni extremy / daily extremes
  if (v_pv_power   > peakPv)  peakPv  = (int16_t)v_pv_power;
  if (v_load_power > maxLoad) maxLoad = (int16_t)v_load_power;
  if (v_soc > 0 && v_soc < minSoc) minSoc = (uint8_t)v_soc;

  int slot = (t.tm_hour * 60 + t.tm_min) / 10;
  if (curSlot < 0) curSlot = slot;

  // usek se uzavrel, zacit novy
  // EN: the slot closed, start a new one
  if (slot != curSlot) {
    curSlot = slot;
    dHas[slot] = false;
    // zapis jen jednou za 10 minut
    // EN: write only once every 10 minutes
    histSave();
  }

  // Do useku se uklada SPICKA, ne prumer. Prumer by kratke odbery schoval -
  // odber 1500 W trvajici dve minuty by z desetiminutoveho useku udelal 300 W
  // EN: a 1500 W draw lasting two minutes would average out to 300 W over ten minutes
  // a v grafu by po nem nezbyla stopa.
  // EN: and the chart would keep no trace of it.
  int16_t pv   = (int16_t)constrain(v_pv_power,   -32000.0f, 32000.0f);
  int16_t ld   = (int16_t)constrain(v_load_power, -32000.0f, 32000.0f);
  int16_t bt   = (int16_t)constrain(v_batt_power, -32000.0f, 32000.0f);

  if (!dHas[slot]) {                       // prvni vzorek v useku / first sample in the slot
    dPv[slot]   = pv;
    dLoad[slot] = ld;
    dBatt[slot] = bt;
  } else {
    if (pv > dPv[slot])   dPv[slot]   = pv;
    if (ld > dLoad[slot]) dLoad[slot] = ld;
    // u baterie rozhoduje velikost, aby se zachovalo i znamenko smeru
    // EN: for the battery magnitude decides, so the direction sign is kept
    if (abs(bt) > abs(dBatt[slot])) dBatt[slot] = bt;
  }

  // stav nabiti je okamzity stav, prumerovat ho nema smysl
  // EN: the state of charge is instantaneous, averaging it makes no sense
  dSoc[slot] = (uint8_t)constrain(v_soc, 0.0f, 100.0f);
  dInvTemp[slot] = (int16_t)constrain(roundf(i_temp * 10.0f), -32000.0f, 32000.0f);
  dOutTemp[slot] = (int16_t)constrain(roundf(w_temp * 10.0f), -32000.0f, 32000.0f);
  if (dInvTemp[slot] < minInvTemp) { minInvTemp = dInvTemp[slot]; minInvSlot = slot; }
  if (dInvTemp[slot] > maxInvTemp) { maxInvTemp = dInvTemp[slot]; maxInvSlot = slot; }
  if (dOutTemp[slot] < minOutTemp) { minOutTemp = dOutTemp[slot]; minOutSlot = slot; }
  if (dOutTemp[slot] > maxOutTemp) { maxOutTemp = dOutTemp[slot]; maxOutSlot = slot; }
  dHas[slot] = true;

  // Pokles kumulativniho citace znamena reset menice/API. Zaklad srovname,
  // ale tento vadny prubeh nezapocitame do denni historie.
  if (baseValid) {
    bool reset = false;
    if (v_load_energy < baseLoad)       { baseLoad = v_load_energy; reset = true; }
    if (v_grid_energy_in < baseGridIn)  { baseGridIn = v_grid_energy_in; reset = true; }
    if (v_batt_energy_in < baseBattIn)  { baseBattIn = v_batt_energy_in; reset = true; }
    if (v_batt_energy_out < baseBattOut){ baseBattOut = v_batt_energy_out; reset = true; }
    if (reset) { baseSave(); errorStart(ERR_COUNTER); }
    else errorStop(ERR_COUNTER);
  }

  if (abs(bt) > maxBattPwr) maxBattPwr = abs(bt);
}

// ---------------------------------------------------------------------------
// DENNI A MESICNI SOUHRNY / DAILY AND MONTHLY TOTALS
// Uklada se do NVS, takze prezijou i odpojeni napajeni.
// EN: Stored in NVS, so they survive a power cut.
// ---------------------------------------------------------------------------
void sumSave() {
  prefs.putBytes("hdPv",   hdPv,     sizeof(hdPv));
  prefs.putBytes("hdLd",   hdLoad,   sizeof(hdLoad));
  prefs.putBytes("hdSv",   hdSave,   sizeof(hdSave));
  prefs.putBytes("hdBi",   hdBIn,    sizeof(hdBIn));
  prefs.putBytes("hdBo",   hdBOut,   sizeof(hdBOut));
  prefs.putBytes("hdGr",   hdGrid,   sizeof(hdGrid));
  prefs.putBytes("hdDn",   hdDayNum, sizeof(hdDayNum));
  prefs.putBytes("hmPv",   hmPv,     sizeof(hmPv));
  prefs.putBytes("hmLd",   hmLoad,   sizeof(hmLoad));
  prefs.putBytes("hmSv",   hmSave,   sizeof(hmSave));
  prefs.putBytes("hmBi",   hmBIn,    sizeof(hmBIn));
  prefs.putBytes("hmBo",   hmBOut,   sizeof(hmBOut));
  prefs.putBytes("hmGr",   hmGrid,   sizeof(hmGrid));
  prefs.putInt("hdCount",  hdCount);
  prefs.putInt("hdPos",    hdPos);
  prefs.putInt("sumYear",  sumYear);
}

void sumLoad() {
  prefs.getBytes("hdPv",   hdPv,     sizeof(hdPv));
  prefs.getBytes("hdLd",   hdLoad,   sizeof(hdLoad));
  prefs.getBytes("hdSv",   hdSave,   sizeof(hdSave));
  prefs.getBytes("hdBi",   hdBIn,    sizeof(hdBIn));
  prefs.getBytes("hdBo",   hdBOut,   sizeof(hdBOut));
  prefs.getBytes("hdGr",   hdGrid,   sizeof(hdGrid));
  prefs.getBytes("hdDn",   hdDayNum, sizeof(hdDayNum));
  prefs.getBytes("hmPv",   hmPv,     sizeof(hmPv));
  prefs.getBytes("hmLd",   hmLoad,   sizeof(hmLoad));
  prefs.getBytes("hmSv",   hmSave,   sizeof(hmSave));
  prefs.getBytes("hmBi",   hmBIn,    sizeof(hmBIn));
  prefs.getBytes("hmBo",   hmBOut,   sizeof(hmBOut));
  prefs.getBytes("hmGr",   hmGrid,   sizeof(hmGrid));
  hdCount = prefs.getInt("hdCount", 0);
  hdPos   = prefs.getInt("hdPos",   0);
  sumYear = prefs.getInt("sumYear", -1);
  if (hdCount < 0 || hdCount > HIST_DAYS) hdCount = 0;
  if (hdPos   < 0 || hdPos   >= HIST_DAYS) hdPos  = 0;
}

// Denni hodnoty jako rozdil proti pulnoci. Kdyby se pocitadlo v menici
// vynulovalo, zaklad se srovna, aby vysledek nebyl zaporny.
// EN: Daily values as the difference since midnight. Should a counter in the
//     inverter reset, the baseline is realigned so the result stays positive.
float dayLoad()     { return v_load_energy     > baseLoad    ? v_load_energy     - baseLoad    : 0; }
float dayGridIn()   { return v_grid_energy_in  > baseGridIn  ? v_grid_energy_in  - baseGridIn  : 0; }
float dayBattIn()   { return v_batt_energy_in  > baseBattIn  ? v_batt_energy_in  - baseBattIn  : 0; }
float dayBattOut()  { return v_batt_energy_out > baseBattOut ? v_batt_energy_out - baseBattOut : 0; }

// vyroba je jedina hodnota, kterou API uz davá jako denni
// EN: production is the only value the API already reports per day
float dayPv()       { return w_pv_generated; }

void baseSave() {
  prefs.putFloat("bLoad",    baseLoad);
  prefs.putFloat("bGridIn",  baseGridIn);
  prefs.putFloat("bBattIn",  baseBattIn);
  prefs.putFloat("bBattOut", baseBattOut);
  prefs.putBool("bValid",    baseValid);
}

void baseLoadFromNvs() {
  baseLoad    = prefs.getFloat("bLoad",    0);
  baseGridIn  = prefs.getFloat("bGridIn",  0);
  baseBattIn  = prefs.getFloat("bBattIn",  0);
  baseBattOut = prefs.getFloat("bBattOut", 0);
  baseValid   = prefs.getBool("bValid", false);
}

// nastavi zaklad na aktualni stav pocitadel
// EN: sets the baseline to the current counter state
void baseReset() {
  baseLoad    = v_load_energy;
  baseGridIn  = v_grid_energy_in;
  baseBattIn  = v_batt_energy_in;
  baseBattOut = v_batt_energy_out;
  baseValid   = true;
  baseSave();
  Serial.printf("zaklad pocitadel: zatez %.2f, sit %.2f, baterie %.2f/%.2f kWh\n",
                baseLoad, baseGridIn, baseBattIn, baseBattOut);
}

// Uspora = energie, kterou dum spotreboval, ale nemusel koupit ze site.
// EN: Saving = energy the house used but did not have to buy from the grid.
// Sobestacnost: jakou cast dnesni spotreby pokryl vlastni zdroj, tedy
// vse krome toho, co dum odebral ze site.
// EN: Self-sufficiency: what share of today's consumption came from our own
//     source, that is everything except what the house took from the grid.
float selfSufficiency() {
  float ld = dayLoad();
  if (ld <= 0.01f) return 100;
  float own = ld - dayGridIn();
  if (own < 0) own = 0;
  return own / ld * 100.0f;
}

// Zmena proti predchozimu obdobi v procentech. Kdyz se neni s cim porovnavat,
// vraci 1000 - volajici to pozna a porovnani vubec nevykresli.
// EN: Change against the previous period in percent. With nothing to compare
//     to it returns 1000; the caller spots that and skips drawing it.
float changePct(float now, float before) {
  if (before <= 0.01f) return 1000;
  return (now - before) / before * 100.0f;
}

// vcerejsi uspora z ulozene historie / yesterday's savings from stored history
float yesterdaySave() {
  if (hdCount < 1) return 0;
  return hdSave[(hdPos - 1 + HIST_DAYS) % HIST_DAYS] / 10.0f;
}

float savedToday() {
  float own = dayLoad() - dayGridIn();
  if (own < 0) own = 0;
  return own * CFG_PRICE;
}

void satAdd(uint16_t& total, uint16_t value) {
  total = value > (uint16_t)(65535U - total) ? 65535U : total + value;
}

// Uzavre prave skonceny den a zapise ho do historie.
// EN: Closes the day that just ended and writes it into the history.
void closeDay(int mday, int mon) {
  uint16_t pv = (uint16_t)constrain(dayPv()      * 10.0f, 0.0f, 65000.0f);
  uint16_t ld = (uint16_t)constrain(dayLoad()    * 10.0f, 0.0f, 65000.0f);
  uint16_t sv = (uint16_t)constrain(savedToday() * 10.0f, 0.0f, 65000.0f);

  uint16_t bi = (uint16_t)constrain(dayBattIn()  * 10.0f, 0.0f, 65000.0f);
  uint16_t bo = (uint16_t)constrain(dayBattOut() * 10.0f, 0.0f, 65000.0f);
  uint16_t gr = (uint16_t)constrain(dayGridIn()  * 10.0f, 0.0f, 65000.0f);

  hdPv[hdPos]     = pv;
  hdLoad[hdPos]   = ld;
  hdSave[hdPos]   = sv;
  hdBIn[hdPos]    = bi;
  hdBOut[hdPos]   = bo;
  hdGrid[hdPos]   = gr;
  hdDayNum[hdPos] = (uint8_t)mday;
  hdPos = (hdPos + 1) % HIST_DAYS;
  if (hdCount < HIST_DAYS) hdCount++;

  if (mon >= 0 && mon < 12) {
    satAdd(hmPv[mon], pv);
    satAdd(hmLoad[mon], ld);
    satAdd(hmSave[mon], sv);
    satAdd(hmBIn[mon], bi);
    satAdd(hmBOut[mon], bo);
    satAdd(hmGrid[mon], gr);
  }

  sumSave();
  Serial.printf("den uzavren: FVE %.1f, zatez %.1f, baterie +%.1f/-%.1f, sit %.1f kWh, usetreno %.1f\n",
                pv / 10.0f, ld / 10.0f, bi / 10.0f, bo / 10.0f, gr / 10.0f, sv / 10.0f);
}

// Historie se uklada do NVS, aby restart nesmazal uz nasbirany den.
// EN: History goes to NVS so a reboot does not wipe the day collected so far.
// Zapisuje se jen pri prechodu na novy desetiminutovy usek, tedy 144x denne.
// EN: Written only when a new ten minute slot starts, so 144 times a day.
void histSave() {
  prefs.putInt("hday", curDay);
  prefs.putBytes("hpv",   dPv,   sizeof(dPv));
  prefs.putBytes("hload", dLoad, sizeof(dLoad));
  prefs.putBytes("hbatt", dBatt, sizeof(dBatt));
  prefs.putBytes("hitmp", dInvTemp, sizeof(dInvTemp));
  prefs.putBytes("hotmp", dOutTemp, sizeof(dOutTemp));
  prefs.putBytes("hsoc",  dSoc,  sizeof(dSoc));
  prefs.putBytes("hhas",  dHas,  sizeof(dHas));
  prefs.putShort("xpv",   peakPv);
  prefs.putShort("xload", maxLoad);
  prefs.putUChar("xsoc",  minSoc);
  prefs.putShort("xbatt", maxBattPwr);
  prefs.putShort("imin", minInvTemp); prefs.putShort("imax", maxInvTemp);
  prefs.putShort("omin", minOutTemp); prefs.putShort("omax", maxOutTemp);
  prefs.putShort("imins", minInvSlot); prefs.putShort("imaxs", maxInvSlot);
  prefs.putShort("omins", minOutSlot); prefs.putShort("omaxs", maxOutSlot);
}

// Nacte historii, ale jen kdyz je ulozena z dnesniho dne.
// EN: Loads the history, but only when it was stored today.
void histLoad() {
  struct tm t;
  // bez casu nevime, jestli sedi den
  // EN: without the time we cannot tell if the day matches
  if (!getLocalTime(&t, 50)) return;
  if (prefs.getInt("hday", -1) != t.tm_yday) {
    Serial.println("ulozena historie je z jineho dne, zahazuji");
    return;
  }
  prefs.getBytes("hpv",   dPv,   sizeof(dPv));
  prefs.getBytes("hload", dLoad, sizeof(dLoad));
  prefs.getBytes("hbatt", dBatt, sizeof(dBatt));
  prefs.getBytes("hitmp", dInvTemp, sizeof(dInvTemp));
  prefs.getBytes("hotmp", dOutTemp, sizeof(dOutTemp));
  prefs.getBytes("hsoc",  dSoc,  sizeof(dSoc));
  prefs.getBytes("hhas",  dHas,  sizeof(dHas));
  peakPv  = prefs.getShort("xpv",  0);
  maxLoad = prefs.getShort("xload", 0);
  minSoc  = prefs.getUChar("xsoc", 100);
  maxBattPwr = prefs.getShort("xbatt", 0);
  minInvTemp = prefs.getShort("imin", 32767); maxInvTemp = prefs.getShort("imax", -32768);
  minOutTemp = prefs.getShort("omin", 32767); maxOutTemp = prefs.getShort("omax", -32768);
  minInvSlot = prefs.getShort("imins", -1); maxInvSlot = prefs.getShort("imaxs", -1);
  minOutSlot = prefs.getShort("omins", -1); maxOutSlot = prefs.getShort("omaxs", -1);
  curDay  = t.tm_yday;
  Serial.println("historie dnesniho dne nactena z flash");
}

// Vola se v kazdem pruchodu smyckou, aby preteceni millis() neuniklo.
// EN: Called on every pass of the loop so a millis() wrap cannot be missed.
void tickUptime() {
  uint32_t now = millis();
  if (now < msLast) msWraps++;
  msLast = now;
}

// doba behu v sekundach vcetne jiz probehlych preteceni
// EN: uptime in seconds including the wraps that already happened
uint32_t uptimeSec() {
  return msWraps * 4294967UL + msLast / 1000;
}

// Doba behu pro zobrazeni. Po prvnim dni uz minuty nikoho nezajimaji,
// proto se prepne na dny a hodiny.
// EN: Uptime for display. After the first day the minutes stop being useful,
//     so it switches to days and hours.
const char* uptimeText() {
  uint32_t sec = uptimeSec();
  if (sec < 86400UL) return fmt("%.0f min", sec / 60.0f);
  return fmt2("%lu d %lu h", sec / 86400UL, (sec % 86400UL) / 3600UL);
}

// stari dat - kdyz se dlouho nic nenacetlo, nesmi displej tvarit, ze je vse v poradku
// EN: data age - after a long silence the display must not pretend all is well
bool dataStale() {
  return !haveFetch || (millis() - lastOkFetch) > CFG_STALE;
}

void updateClock() {
  struct tm t;
  if (getLocalTime(&t, 5)) {
    // strftime na teto platforme nedosazovalo %H ani %M a na displeji
    // EN: strftime on this platform filled in neither %H nor %M and the display
    // zbyla jen dvojtecka, proto se cas sklada primo z polozek struktury
    // EN: left only the colon, so the time is built from the struct fields directly
    snprintf(clockStr, sizeof(clockStr), "%02d:%02d", t.tm_hour, t.tm_min);
    timeOk = true;
  }
}

// Rychlost zmeny stavu nabiti. / Rate of change of the state of charge.
//
// Drive se odvozovala z merenych zmen SOC v patnactiminutovem okne, jenze
// EN: It used to come from measured SOC changes over a 15 minute window, but
// SOC prichazi z API po celych procentech. Pri nabijeni 1 kW do baterie
// EN: the SOC arrives from the API in whole percent. Charging 1 kW into a
// 9,6 kWh je skutecna rychlost 10 %/hod, ale za 15 minut se SOC casto
// EN: 9.6 kWh the real rate is 10 %/h, yet over 15 minutes the SOC often
// nezmenil vubec a vysledek byl nula.
// EN: did not change at all and the result was zero.
//
// Vypocet z vykonu a kapacity je okamzity, plynuly a presny.
// EN: Deriving it from power and capacity is instant, smooth and accurate.
void updateSocRate() {
  if (v_batt_capacity <= 0) { socRate = 0; return; }
  socRate = (v_batt_power / 1000.0f) / v_batt_capacity * 100.0f;
}

// ===========================================================================
// JSON / JSON
// ===========================================================================
bool metricValid(const char* topic, float value) {
  if (!isfinite(value)) return false;
  if (strstr(topic, "state_of_charge")) return value >= 0 && value <= 100;
  if (strstr(topic, "temperature")) return value >= -80 && value <= 150;
  if (strstr(topic, "percentage") || strstr(topic, "progress") || strstr(topic, "cloud_cover")) return value >= 0 && value <= 100;
  if (strstr(topic, "energy")) return value >= 0 && value <= 10000000.0f;
  if (strstr(topic, "voltage")) return value >= 0 && value <= 1000;
  if (strstr(topic, "frequency")) return value >= 0 && value <= 100;
  if (strstr(topic, "power") || strstr(topic, "apparent_power")) return fabsf(value) <= 100000;
  return fabsf(value) <= 1000000;
}

bool handleItems(JsonArray arr) {
  // vypsat vsechny hodnoty jen kdyz si o to rekne tlacitko v nastaveni
  // EN: dump every value only when the settings button asks for it
  if (dumpRequest) {
  dumpRequest = false;
  Serial.println();
  Serial.println("==========================================================");
  Serial.printf("  HODNOTY Z MENICE   [cas behu %lu s]\n", (unsigned long)uptimeSec());
  Serial.println("==========================================================");
  int i = 0;
  for (JsonObject item : arr) {
    const char* t = item["topic"] | "?";
    const char* u = item["unit"]  | "";
    float val     = item["value"] | 0.0f;
    Serial.printf("%3d  %-44s = %12.2f %s\n", ++i, t, val, u);
  }
  Serial.println("==========================================================");
  Serial.printf("  celkem %d promennych, skec jich pouziva %d\n", i, METRIC_N);
  Serial.println("==========================================================");
  snprintf(setMsg, sizeof(setMsg), TR(T_DUMPED), (unsigned long)uptimeSec());
  }

  for (JsonObject item : arr) {
    const char* topic = item["topic"];
    if (!topic) continue;
    float value = item["value"] | 0.0f;
    if (!metricValid(topic, value)) {
      Serial.printf("neplatna hodnota API: %s = %.2f\n", topic, value);
      apiInvalid = true;
      return false;
    }

    for (int i = 0; i < METRIC_N; i++) {
      if (strcmp(topic, METRICS[i].topic) == 0) {
        *(METRICS[i].var) = value;
        break;
      }
    }
  }
  apiInvalid = false;
  return true;
}

// ===========================================================================
// TEST SPOJENI / LINK TEST
// HTTP chyba -1 znamena, ze se nepodarilo navazat TCP spojeni. Tahle funkce
// EN: HTTP error -1 means the TCP connection failed. This function
// rozlisi, jestli je problem v siti, v adrese nebo v portu.
// EN: tells apart a network, address or port problem.
// ===========================================================================
void netDiag() {
  Serial.println();
  Serial.println("==========================================================");
  Serial.println("  TEST SPOJENI");
  Serial.println("==========================================================");
  Serial.printf("  stav WiFi   : %d (3 = pripojeno)\n", WiFi.status());
  Serial.printf("  IP desky    : %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("  maska       : %s\n", WiFi.subnetMask().toString().c_str());
  Serial.printf("  brana       : %s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("  DNS         : %s\n", WiFi.dnsIP().toString().c_str());
  Serial.printf("  signal      : %d dBm\n", WiFi.RSSI());
  Serial.printf("  cilova URL  : %s\n", url);

  IPAddress target;
  if (!target.fromString(API_HOST)) {
    Serial.println("  CHYBA: API_HOST neni platna IP adresa");
    Serial.println("==========================================================");
    return;
  }

  // je cil ve stejne podsiti? / is the target on the same subnet?
  IPAddress me = WiFi.localIP();
  IPAddress mask = WiFi.subnetMask();
  bool sameNet = true;
  for (int i = 0; i < 4; i++)
    if ((me[i] & mask[i]) != (target[i] & mask[i])) sameNet = false;
  Serial.printf("  podsit      : %s\n",
                sameNet ? "stejna jako deska" : "JINA - provoz jde pres branu");

  // Test brany. Kdyz neodpovi ani router, je deska nejspis na siti
  // EN: Gateway test. If even the router stays silent, the board is probably on a network
  // s izolaci klientu (hostovska WiFi) a na LAN se nedostane vubec.
  // EN: with client isolation (guest Wi-Fi) and cannot reach the LAN at all.
  IPAddress gw = WiFi.gatewayIP();
  bool gwOk = false;
  {
    WiFiClient c;
    Serial.printf("  brana port 80 : ");
    Serial.flush();
    gwOk = c.connect(gw, 80, 2500);
    Serial.println(gwOk ? "odpovida" : "neodpovida");
    if (gwOk) c.stop();
  }

  // zkusit se pripojit na obvykle porty / try to connect on the usual ports
  const uint16_t ports[] = { 80, 3000, 8080, 8123, 443 };
  bool any = false;
  for (uint8_t i = 0; i < sizeof(ports) / sizeof(ports[0]); i++) {
    WiFiClient c;
    Serial.printf("  port %-5u  : ", ports[i]);
    Serial.flush();
    if (c.connect(target, ports[i], 2500)) {
      Serial.println("OTEVRENY");
      c.stop();
      any = true;
    } else {
      Serial.println("neodpovida");
    }
  }

  Serial.println("----------------------------------------------------------");
  if (!any) {
    Serial.println("  Na teto adrese neodpovida zadny port.");
    if (!gwOk) {
      Serial.println("  Neodpovida ani brana -> deska je nejspis na siti");
      Serial.println("  s izolaci klientu (hostovska WiFi / jina VLAN).");
      Serial.println("  Reseni: pripojit desku na stejnou sit jako menic.");
    } else {
      Serial.println("  Brana odpovida, takze sit funguje - problem je v cili.");
      Serial.println("  - overte aktualni IP menice (mohl dostat jinou z DHCP)");
      Serial.println("  - bezi Solar Assistant?");
      Serial.println("  Tlacitkem SKEN SITE na strance nastaveni se da");
      Serial.println("  prohledat cela podsit a najit, kde menic je.");
    }
  } else {
    Serial.println("  Nejaky port odpovida - pokud to neni 80, upravte URL:");
    Serial.println("  const char* url = \"http://\" API_HOST \":3000/api/v1/metrics\";");
  }
  Serial.println("==========================================================");
  Serial.println();
  snprintf(setMsg, sizeof(setMsg), "test proveden, vysledek v Serialu");
}

// ===========================================================================
// SKEN PODSITE / SUBNET SCAN
// Projde cely rozsah .1 az .254 a vypise adresy, kde neco posloucha na
// EN: Walks the whole .1 to .254 range and lists addresses listening on
// portu 80. Trva do minuty, spousti se jen tlacitkem.
// EN: port 80. Takes up to a minute, runs only from the button.
// ===========================================================================
void netScan() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("sken preskocen - deska neni na WiFi");
    return;
  }

  IPAddress me = WiFi.localIP();
  Serial.println();
  Serial.println("==========================================================");
  Serial.printf("  SKEN SITE %d.%d.%d.1-254 na portu 80\n", me[0], me[1], me[2]);
  Serial.println("  muze trvat az minutu...");
  Serial.println("==========================================================");

  int found = 0;
  for (int host = 1; host <= 254; host++) {
    if (host == me[3]) continue;                 // sebe preskocit / skip ourselves

    IPAddress ip(me[0], me[1], me[2], host);
    WiFiClient c;
    if (c.connect(ip, 80, 250)) {
      Serial.printf("  %s  <<< odpovida na portu 80\n", ip.toString().c_str());
      c.stop();
      found++;
    }
    if (host % 32 == 0) { Serial.printf("  ... %d/254\n", host); Serial.flush(); }
    delay(1);
  }

  Serial.println("----------------------------------------------------------");
  Serial.printf("  nalezeno %d zarizeni\n", found);
  if (found)
    Serial.println("  Pokud je mezi nimi menic, prepiste API_HOST v kodu.");
  else
    Serial.println("  Nic nenalezeno - deska se na LAN nedostane.");
  Serial.println("==========================================================");
  Serial.println();

  snprintf(setMsg, sizeof(setMsg), TR(T_SCAN_DONE), found);
}

// ===========================================================================
// HTTP / HTTP
// ===========================================================================
bool fetchData() {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.begin(url);
  http.setAuthorization(user, pass);
  http.setTimeout(6000);

  int code = http.GET();
  if (code != 200) {
    Serial.printf("HTTP chyba: %d  (%s)\n", code,
                  code == -1 ? "nelze navazat TCP spojeni" :
                  code == -11 ? "vyprsel cas" :
                  code == 401 ? "spatne jmeno nebo heslo" : "viz dokumentace HTTPClient");
    http.end();
    return false;
  }

#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument doc;
#else
  DynamicJsonDocument doc(24576);
#endif

  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();

  if (err) {
    Serial.printf("JSON chyba: %s\n", err.c_str());
    return false;
  }

  JsonArray arr = doc.as<JsonArray>();
  if (arr.isNull()) {
    Serial.println("odpoved neni JSON pole");
    return false;
  }

  return handleItems(arr);
}

// ===========================================================================
// UVODNI OBRAZOVKA A KALIBRACE / SPLASH SCREEN AND CALIBRATION
// ===========================================================================
void splash(const char* l1, const char* l2, uint16_t col) {
  tft.fillScreen(C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(col, C_BG);
  tCz(l1, SCR_W / 2, SCR_H / 2 - 20);
  if (l2) {
    tft.setTextColor(C_DIM, C_BG);
    tCz(l2, SCR_W / 2, SCR_H / 2 + 16);
  }
  tft.setTextDatum(TL_DATUM);
}

void loadOrCalibrate() {
  uint16_t cal[5];
  // pro portret je potreba vlastni kalibrace, klic je jiny nez u rotace 1
  // EN: portrait needs its own calibration, the key differs from rotation 1
  // kalibrace plati vzdy jen pro jedno otoceni displeje
  // EN: calibration is always valid for a single display rotation
  const char* key = (CFG_ROT == 0) ? "calib0" : "calib2";
  size_t len = prefs.getBytes(key, cal, sizeof(cal));
  bool held = (tft.getTouchRawZ() > 600);

  if (len == sizeof(cal) && !held) {
    tft.setTouch(cal);
    Serial.println("kalibrace dotyku (portret) nactena z flash");
    return;
  }

  splash(TR(T_CALIB), TR(T_CALIB_HINT), C_TXT);
  delay(2200);
  tft.fillScreen(C_BG);
  tft.calibrateTouch(cal, C_PV, C_BG, 15);
  prefs.putBytes(key, cal, sizeof(cal));
  tft.setTouch(cal);
  Serial.println("kalibrace ulozena");
}

// ===========================================================================
// OVLADANI DOTYKEM / TOUCH INPUT
// ===========================================================================
void handleTouch() {
  static bool was = false;
  uint16_t x, y;

  if (!tft.getTouch(&x, &y)) { was = false; return; }
  // reagovat jen na stisk, ne na drzeni
  // EN: react to a press only, not to holding
  if (was) return;
  was = true;

  // prvni dotek po zhasnuti displej jen probudi
  // EN: the first touch after sleep only wakes the display
  if (!screenOn) { screenWake(); return; }
  lastTouch = millis();

  // prokliky z uvodni obrazovky na podrobnou stranku
  // EN: taps on the overview screen jump to the detail page
  if (screen == 0 && y < NAV_Y) {
    int target = -1;
    // Menic / Solarni PV / Inverter
    // EN: Solar PV
    if      (y >=  44 && y < 100) target = (x < 160) ? 6 : 3;
    else if (y >= 104 && y < 160) target = (x < 160) ? 4 : 1;   // Sit / Baterie / Grid / Battery
    // Predikce -> predikce uspor
    // EN: Forecast -> savings forecast
    else if (y >= 164 && y < 228) target = 11;
    // ukazatel Zatez / PV / Load
    // EN: PV gauge
    else if (y >= 238 && y < 322) target = (x < 160) ? 4 : 3;
    // ukazatel Sit / Baterie / Grid
    // EN: Battery gauge
    else if (y >= 332 && y < 416) target = (x < 160) ? 4 : 2;

    if (target >= 0) {
      screen = target;
      drawScreen();
    }
    return;
  }

  // radky na strance NASTAVENI 2 / rows on the SETTINGS 2 screen
  // klepnuti na graf historie prepne zobrazene obdobi
  // EN: tapping the history chart switches the period shown
  if (screen == 9 && y >= 44 && y < 240) {
    histRange = (histRange + 1) % 3;
    drawScreen();
    return;
  }

  if (screen == 13 && y < NAV_Y) {
    int row = (y - S2_Y0) / S2_STEP;
    if (row >= 0 && row < S2_ROWS && (y - S2_Y0) % S2_STEP <= S2_H) cfgNext(row);
    return;
  }

  if (screen == 15 && y < NAV_Y) {
    int row = (y - S3_Y0) / S3_STEP;
    if (row >= 0 && row < S3_ROWS && (y - S3_Y0) % S3_STEP <= S3_H) cfg3Next(row);
    return;
  }

  if (screen == 14 && y >= 354 && y <= 404) {
    errorClear();
    drawScreen();
    return;
  }

  // tlacitko na strance nastaveni / button on the settings screen
  if (y < NAV_Y) {
    if (screen == 12 && y >= SET_BTN_Y && y <= SET_BTN_Y + SET_BTN_H) {
      if (x >= SET_BTN_X && x <= SET_BTN_X + SET_BTN_W) {
        dumpRequest = true;
        // stahnout hned, necekat na interval
        // EN: fetch right away, do not wait for the interval
        lastFetch = millis() - CFG_FETCH;
        snprintf(setMsg, sizeof(setMsg), "%s", TR(T_FETCHING));
        drawScreen();
      } else if (x >= SET_BT2_X && x <= SET_BT2_X + SET_BTN_W) {
        snprintf(setMsg, sizeof(setMsg), "%s", TR(T_TESTING));
        drawScreen();
        netDiag();
        drawScreen();
      }
    }
    else if (screen == 12 && y >= SET_BT3_Y && y <= SET_BT3_Y + SET_BT3_H) {
      snprintf(setMsg, sizeof(setMsg), "%s", TR(T_SCANNING));
      drawScreen();
      netScan();
      drawScreen();
    }
    return;
  }

  int prevScreen = screen;
  if      (x < 108)  screen = (screen - 1 + SCREENS) % SCREENS;
  else if (x < 216)  screen = 0;
  else               screen = (screen + 1) % SCREENS;

  if (screen != prevScreen) drawScreen();
}

// ===========================================================================
void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println();
  Serial.println("=== Solar Assistant - graficky monitor (CYD portret) ===");

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  PWM_ATTACH(LED_R, 0);
  PWM_ATTACH(LED_G, 1);
  PWM_ATTACH(LED_B, 2);
  setLed(0, 0, 255);                 // modra = jeste nemame data / blue = no data yet

  tft.init();
  tft.setRotation(CFG_ROT);
  tft.fillScreen(C_BG);
  Serial.printf("displej %d x %d\n", tft.width(), tft.height());

  prefs.begin("cyd", false);
  cfgLoad();
  errorLoad();
  sumLoad();
  baseLoadFromNvs();
  // podsviceni pres PWM kvuli jasu
  // EN: backlight on PWM so brightness can be set
  PWM_ATTACH(TFT_BL, 3);
  PWM_WRITE(TFT_BL, 3, CFG_BRIGHT);
  tft.setRotation(CFG_ROT);
  loadOrCalibrate();

  // uvitaci obrazovka / splash screen
  tft.fillScreen(C_BG);
  drawLogo(150);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TXT, C_BG);
  tCz(TR(T_AUTHOR), SCR_W / 2, 268);
  tft.setTextColor(C_DIM, C_BG);
  tCz("firmware v" FW_VERSION, SCR_W / 2, 290);

  tft.setTextColor(C_WEATH, C_BG);
  tCz("github.com/tomas-cabaj", SCR_W / 2, 326);
  tCz("t.cabaj@email.cz", SCR_W / 2, 348);
  tft.setTextDatum(TL_DATUM);
  delay(2500);

  splash(TR(T_WIFI_CONN), ssid, C_TXT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 40) { delay(250); retry++; }

  wifiOk = (WiFi.status() == WL_CONNECTED);
  if (wifiOk) {
    Serial.print("WiFi OK, IP ");
    Serial.println(WiFi.localIP());
    setupOTA();
    configTzTime(NTP_TZ, NTP_SERVER1, NTP_SERVER2);
    Serial.println("cas se synchronizuje z internetu");
    splash(TR(T_WIFI_OK), TR(T_FETCHING), C_BATT);
    dataOk = fetchData();
    if (dataOk) {
      errorStop(ERR_FETCH); errorStop(ERR_API);
      lastOkFetch = millis(); haveFetch = true; pushHistory(); updateSocRate();
      if (v_soc < CFG_ALERT_SOC) errorStart(ERR_SOC); else errorStop(ERR_SOC);
      if (i_temp > CFG_ALERT_TEMP) errorStart(ERR_TEMP); else errorStop(ERR_TEMP);
    } else { errorStart(apiInvalid ? ERR_API : ERR_FETCH); }
    updateLoadLed();
  } else {
    Serial.println("WiFi se nepodarilo pripojit");
    splash(TR(T_WIFI_ERR), ssid, C_GRID);
    delay(2000);
  }
  lastFetch = millis();

  lastTouch = millis();
  tft.fillScreen(C_BG);
  drawScreen();
}

// ===========================================================================
void loop() {
  tickUptime();
  if (wifiOk) ArduinoOTA.handle();
  handleTouch();

  // zhasnuti displeje po necinnosti / display sleep after inactivity
  if (CFG_SLEEP > 0 && screenOn && millis() - lastTouch > CFG_SLEEP) screenSleep();

  // Dioda se musi obnovovat casto, jinak neni blikani videt - jeji stav se
  // pocita z millis() a pri obnove jednou za 20 s by se prepnula jen jednou
  // za 20 sekund.
  // EN: The LED has to refresh often or the blinking stays invisible - its
  //     state comes from millis(), and refreshing once per fetch would toggle
  //     it only once every 20 seconds.
  static uint32_t tLed = 0;
  if (millis() - tLed > 100) {
    tLed = millis();
    updateLoadLed();
  }

  // stahovani dat / data fetching
  if (millis() - lastFetch > CFG_FETCH) {
    lastFetch = millis();

    if (WiFi.status() != WL_CONNECTED) {
      wifiOk = false;
      Serial.println("WiFi spadla, zkousim znovu");
      WiFi.reconnect();
    } else {
      wifiOk = true;
    }

    dataOk = fetchData();
    updateLoadLed();

    if (dataOk) {
      failCount = 0;
      errorStop(ERR_FETCH);
      errorStop(ERR_API);
      lastOkFetch = millis();
      haveFetch   = true;
      pushHistory();
      updateSocRate();
      if (v_soc < CFG_ALERT_SOC) errorStart(ERR_SOC); else errorStop(ERR_SOC);
      if (i_temp > CFG_ALERT_TEMP) errorStart(ERR_TEMP); else errorStop(ERR_TEMP);

      Serial.printf("SOC %5.1f %%  FVE %6.0f W  BAT %+7.0f W  ZATEZ %6.0f W  SIT %+7.0f W  %.1f C  dioda=%s\n",
                    v_soc, v_pv_power, v_batt_power, v_load_power, v_grid_power, i_temp,
                    loadLevelName());

      drawScreen();
    } else {
      failCount++;
      uint8_t errorType = apiInvalid ? ERR_API : ERR_FETCH;
      if (apiInvalid) errorStop(ERR_FETCH); else errorStop(ERR_API);
      errorStop(ERR_SOC);
      errorStop(ERR_TEMP);
      errorStart(errorType);
      errorRepeat(errorType);
      Serial.printf("nacteni selhalo (%dx po sobe)\n", failCount);
      // pri prvnim selhani rovnou zjistit proc
      // EN: on the first failure find out why right away
      if (failCount == 1) netDiag();

      // po dlouhem vypadku restart - nema smysl viset donekonecna
      // EN: reboot after a long outage - no point hanging forever
      if (failCount >= FAIL_REBOOT) {
        Serial.println("prilis mnoho neuspechu, restartuji desku");
        histSave();
        delay(500);
        ESP.restart();
      }
      drawHeader();
    }
  }

  // prubezna aktualizace stari dat v hlavicce
  // EN: keep the data age in the header current
  static uint32_t tHdr = 0;
  if (millis() - tHdr > 1000) {
    tHdr = millis();
    drawHeader();
  }

  delay(10);
}
