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
 *      devatenacti obrazovkach. Prepina se tlacitky dole: [<] [domu] [>],
 *      na uvodni strance se da kliknout primo na kterykoli blok.
 *
 *  EN: Reads data from Solar Assistant over its REST API and shows it on
 *      nineteen screens. Switched by the buttons at the bottom:
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
 *  16  DNES A VCERA / TODAY & YESTERDAY  srovnani dennich hodnot
 *  17  O APLIKACI / ABOUT     verze, deska, kontakt
 *  18  NAVRATNOST / PAYBACK   rucni investice a vyuzita energie
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
#define FW_VERSION   "2.01"

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
// Na pouzite desce jsou fyzicke kanaly G/B oproti popisu prohozene.
// EN: This board has the physical G/B channels swapped from the usual pin map.
#define LED_G           17
#define LED_B           16
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

#define SCREENS     19
enum ScreenId {
  SCR_OVERVIEW, SCR_BATTERY, SCR_RUNTIME, SCR_SOLAR, SCR_GRID, SCR_INVERTER,
  SCR_WEATHER, SCR_GRAPHS, SCR_TEMPERATURES, SCR_HISTORY, SCR_SAVINGS,
  SCR_FORECAST, SCR_COMPARE, SCR_ROI, SCR_ERRORS, SCR_ALERTS, SCR_SETTINGS,
  SCR_SETTINGS2, SCR_ABOUT
};
static_assert(SCR_ABOUT + 1 == SCREENS, "Screen identifiers mismatch");

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
const uint16_t OPT_GRID_LIMIT[] = { 0, 1000, 1500, 2000, 2500, 3000 };
const uint16_t OPT_GRID_TIME[]  = { 5, 10, 15, 30, 60 };

#define OPT_N(a) (int)(sizeof(a) / sizeof(a[0]))

uint8_t iFetch = 1, iSleep = 2, iBright = 3, iRange = 1;
uint8_t iGreen = 2, iOrange = 1, iRot = 0;
uint8_t iBlink = 0, iPrice = 10, iCurr = 0;
uint8_t iAlertSoc = 2, iAlertTemp = 3, iStale = 1, iErrLed = 1;
uint8_t iGridLimit = 3, iGridTime = 3;

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
#define CFG_GRID_LIMIT OPT_GRID_LIMIT[iGridLimit]
#define CFG_GRID_TIME  OPT_GRID_TIME[iGridTime]
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
enum { ERR_FETCH = 1, ERR_API, ERR_COUNTER, ERR_SOC, ERR_TEMP, ERR_GRID };
struct ErrorEntry { uint8_t type, count, hour, minute, endHour, endMinute; uint16_t yday, durationMin; };
ErrorEntry errorLog[ERR_LOG_N];
uint8_t errCount = 0, errPos = 0;
uint8_t activeErrors = 0;
bool apiInvalid = false;
uint32_t gridHighSince = 0;

// millis() pretece po 49 dnech. Rozdily dvou casu to prezijou samy, ale doba
// behu ne - proto se preteceni pocitaji zvlast.
// EN: millis() wraps after 49 days. Differences of two stamps survive that on
//     their own, the uptime does not - hence a separate wrap counter.
uint32_t msLast = 0, msWraps = 0;
int      screen = 0;
bool     needFullRedraw = true;

// Neblokujici interpolace slouzi pouze pro kresleni. Zive hodnoty API se
// nemeni, takze upozorneni, historie a vypocty vzdy pouzivaji cerstva data.
// EN: Non-blocking interpolation is drawing-only. Live API values stay
// untouched, so alerts, history and calculations always see fresh data.
enum GaugeMetric { GM_PV, GM_LOAD, GM_GRID, GM_BATT, GM_SOC, GM_CLOUD, GM_TEMP, GM_COUNT };
// Signatury pouzivaji zakladni typ, protoze Arduino vklada automaticke
// prototypy pred mistni enumy. Konstanty GaugeMetric zustavaji typove citelne.
// EN: Signatures use a base type because Arduino inserts automatic prototypes
// before local enums. GaugeMetric constants keep call sites readable.
float gaugeTarget(uint8_t metric);
float animatedGauge(uint8_t metric);
void startGaugeAnimation(const float previous[GM_COUNT], bool hadData);
void drawAnimatedGauges();
struct GaugeAnimation {
  float from[GM_COUNT];
  uint32_t started;
  bool active;
};
GaugeAnimation gaugeAnim = {{0}, 0, false};
constexpr uint32_t GAUGE_ANIM_MS = 600;
constexpr uint32_t GAUGE_FRAME_MS = 40;
uint32_t gaugeLastFrame = 0;

// Posledni bod vybrany dotykem v carovem grafu.
// EN: Last point selected by touch in a line chart.
struct GraphCursor {
  int8_t screenId;
  int8_t graphId;
  int16_t slot;
  bool active;
};
GraphCursor graphCursor = {-1, -1, -1, false};

// Vybrany den ve sloupcovem grafu uspor; -1 = bez vyberu.
// EN: Selected day in the savings bar chart; -1 means no selection.
int8_t savingsSelected = -1;

// Vybrany mesic v grafu predikce; -1 = bez vyberu.
// EN: Selected month in the forecast chart; -1 means no selection.
int8_t forecastSelected = -1;

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
int sunDayMinutes = -1;  // -1: unknown, 0/1440: polar night/day

bool    timeOk = false;
char    clockStr[8] = "--:--";
bool    histLoaded = false;

// denni extremy, nuluji se o pulnoci spolu s historii
// EN: daily extremes, cleared at midnight together with the history
int16_t peakPv = 0, maxLoad = 0;
uint8_t minSoc = 100;
int16_t minInvTemp = 32767, maxInvTemp = -32768, minOutTemp = 32767, maxOutTemp = -32768;
int16_t minInvSlot = -1, maxInvSlot = -1, minOutSlot = -1, maxOutSlot = -1;

// Uzavreny vcerejsek pro prime porovnani s dneskem. Uklada se samostatne,
// aby se neztratil pri posunu kruhove historie 31 dni.
struct DaySummary {
  uint16_t pv, load, save, maxLoad;
  int16_t minInv, maxInv, minOut, maxOut;
  uint8_t day, valid;
};
DaySummary yesterday = {};



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
    case ERR_TEMP:    return TR(T_ALERT_TEMP);
    default:          return TR(T_ERR_GRID);
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
  entry.type = type; entry.count = 1; entry.yday = 0; entry.durationMin = 0;
  entry.hour = entry.minute = entry.endHour = entry.endMinute = 255;
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
      struct tm t;
      if (getLocalTime(&t, 5)) { entry.endHour = t.tm_hour; entry.endMinute = t.tm_min; }
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

void updateGridAlert() {
  if (CFG_GRID_LIMIT == 0 || v_grid_power < CFG_GRID_LIMIT) {
    gridHighSince = 0;
    errorStop(ERR_GRID);
    return;
  }
  if (gridHighSince == 0) gridHighSince = millis();
  if (millis() - gridHighSince >= (uint32_t)CFG_GRID_TIME * 60000UL)
    errorStart(ERR_GRID);
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
void icoLine(int x1, int y1, int x2, int y2, uint16_t c) {
  tft.drawLine(x1, y1, x2, y2, c);
  if (abs(x2 - x1) >= abs(y2 - y1)) tft.drawLine(x1, y1 + 1, x2, y2 + 1, c);
  else tft.drawLine(x1 + 1, y1, x2 + 1, y2, c);
}

void icoSun(int x, int y, uint16_t c) {
  tft.fillCircle(x + 15, y + 15, 6, c);
  for (int a = 0; a < 360; a += 45) {
    float r = a * DEG_TO_RAD;
    icoLine(x + 15 + cosf(r) * 10, y + 15 + sinf(r) * 10,
            x + 15 + cosf(r) * 14, y + 15 + sinf(r) * 14, c);
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
  icoCloud(x, y + 3, C_TXT);
}

void icoBattery(int x, int y, uint16_t c) {
  tft.drawRoundRect(x + 5, y + 4, 20, 24, 4, C_TXT);
  tft.drawRoundRect(x + 6, y + 5, 18, 22, 3, C_TXT);
  tft.fillRect(x + 11, y + 1, 8, 3, C_TXT);
  tft.fillRoundRect(x + 9, y + 8, 12, 16, 2, c);
  tft.fillTriangle(x + 17, y + 9, x + 11, y + 17, x + 15, y + 17, C_TXT);
  tft.fillTriangle(x + 13, y + 23, x + 19, y + 15, x + 15, y + 15, C_TXT);
}

void icoPlug(int x, int y, uint16_t c) {
  tft.drawRoundRect(x + 6, y + 10, 18, 12, 3, c);
  tft.drawLine(x + 10, y + 4, x + 10, y + 10, c);
  tft.drawLine(x + 20, y + 4, x + 20, y + 10, c);
  tft.drawLine(x + 24, y + 16, x + 28, y + 16, c);
}

void icoInverter(int x, int y, uint16_t c) {
  tft.drawRoundRect(x + 4, y + 3, 22, 26, 4, C_TXT);
  tft.drawRoundRect(x + 5, y + 4, 20, 24, 3, C_TXT);
  tft.fillRoundRect(x + 8, y + 7, 14, 9, 2, C_BG);
  icoLine(x + 10, y + 12, x + 12, y + 10, c);
  icoLine(x + 12, y + 10, x + 15, y + 14, c);
  icoLine(x + 15, y + 14, x + 19, y + 10, c);
  tft.fillCircle(x + 11, y + 22, 2, C_TXT);
  tft.fillCircle(x + 19, y + 22, 2, C_TXT);
}

// fotovoltaicky panel se stojanem / photovoltaic panel with a stand
void icoPanel(int x, int y, uint16_t c) {
  tft.fillCircle(x + 23, y + 6, 4, C_PV);
  tft.drawRect(x + 2, y + 8, 25, 15, c);
  tft.drawRect(x + 3, y + 9, 23, 13, c);
  tft.drawFastVLine(x + 10, y + 9, 13, c);
  tft.drawFastVLine(x + 18, y + 9, 13, c);
  tft.drawFastHLine(x + 3, y + 15, 23, c);
  icoLine(x + 15, y + 23, x + 15, y + 28, C_TXT);
  tft.drawFastHLine(x + 8, y + 28, 15, C_TXT);
}

// prihradovy stozar vysokeho napeti / lattice transmission tower
void icoPylon(int x, int y, uint16_t c) {
  icoLine(x + 6, y + 28, x + 13, y + 5, C_TXT);
  icoLine(x + 24, y + 28, x + 17, y + 5, C_TXT);
  tft.drawFastHLine(x + 12, y + 5, 7, C_TXT);
  tft.drawFastHLine(x + 4, y + 11, 23, C_TXT);
  tft.drawFastHLine(x + 1, y + 17, 29, C_TXT);
  icoLine(x + 10, y + 11, x + 20, y + 17, C_TXT);
  icoLine(x + 20, y + 11, x + 10, y + 17, C_TXT);
  icoLine(x + 9, y + 18, x + 21, y + 27, C_TXT);
  icoLine(x + 21, y + 18, x + 9, y + 27, C_TXT);
}

void icoHouse(int x, int y, uint16_t c) {
  tft.fillTriangle(x + 2, y + 15, x + 15, y + 3, x + 28, y + 15, C_TXT);
  tft.fillRect(x + 6, y + 14, 19, 14, C_TXT);
  tft.fillRect(x + 9, y + 18, 5, 5, c);
  tft.fillRect(x + 17, y + 18, 5, 10, c);
}

void icoTemp(int x, int y, uint16_t c) {
  tft.drawRoundRect(x + 10, y + 2, 10, 21, 5, C_TXT);
  tft.drawRoundRect(x + 11, y + 3, 8, 19, 4, C_TXT);
  tft.fillCircle(x + 15, y + 24, 6, C_TXT);
  tft.fillCircle(x + 15, y + 24, 4, c);
  tft.fillRect(x + 14, y + 10, 3, 14, c);
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
  constexpr float STEP = 1.5f;
  for (float a = aFrom; a < aTo; a += STEP) {
    float b = min(a + STEP, aTo);
    float ar = a * DEG_TO_RAD, br = b * DEG_TO_RAD;
    int aiX = lroundf(cx + cosf(ar) * rIn), aiY = lroundf(cy + sinf(ar) * rIn);
    int aoX = lroundf(cx + cosf(ar) * rOut), aoY = lroundf(cy + sinf(ar) * rOut);
    int biX = lroundf(cx + cosf(br) * rIn), biY = lroundf(cy + sinf(br) * rIn);
    int boX = lroundf(cx + cosf(br) * rOut), boY = lroundf(cy + sinf(br) * rOut);
    tft.fillTriangle(aiX, aiY, aoX, aoY, boX, boY, col);
    tft.fillTriangle(aiX, aiY, boX, boY, biX, biY, col);
  }
}

float gaugeTarget(uint8_t metric) {
  switch (metric) {
    case GM_PV:    return v_pv_power;
    case GM_LOAD:  return v_load_power;
    case GM_GRID:  return v_grid_power;
    case GM_BATT:  return v_batt_power;
    case GM_SOC:   return v_soc;
    case GM_CLOUD: return w_cloud;
    default:       return i_temp;
  }
}

float animatedGauge(uint8_t metric) {
  if (!gaugeAnim.active) return gaugeTarget(metric);
  float p = constrain((millis() - gaugeAnim.started) / (float)GAUGE_ANIM_MS, 0.0f, 1.0f);
  p = p * p * (3.0f - 2.0f * p);
  return gaugeAnim.from[metric] + (gaugeTarget(metric) - gaugeAnim.from[metric]) * p;
}

// Animaci pouzivaji jen stranky s pulkruhovym ukazatelem.
// EN: Only pages containing a semicircular gauge use the animation.
bool screenUsesGaugeAnimation(int page) {
  return page == SCR_OVERVIEW || page == SCR_BATTERY || page == SCR_SOLAR ||
         page == SCR_GRID || page == SCR_WEATHER || page == SCR_INVERTER;
}

void startGaugeAnimation(const float previous[GM_COUNT], bool hadData) {
  // Animace je zamerne vypnuta: nova data se vykresli jednim prekreslenim.
  // EN: Animation is intentionally disabled: new data is drawn in one refresh.
  (void)previous;
  (void)hadData;
  gaugeLastFrame = 0;
  gaugeAnim.active = false;
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
#include "UiStyle.h"
#include "WeatherIcons.h"
#include "UiIcons.h"

// Bitmapa se kresli primo z flash; nevytvari se docasny buffer v RAM.
// EN: The bitmap is drawn directly from flash; no temporary RAM buffer is made.
void drawUiIcon(uint8_t icon, int x, int y) {
  if (icon >= UI_ICON_COUNT) return;
  // RGB565 je ulozen v poradi bajtu pro TFT_eSPI.
  // EN: RGB565 is stored in the byte order expected by TFT_eSPI.
  tft.setSwapBytes(true);
  tft.pushImage(x, y, UI_ICON_SIZE, UI_ICON_SIZE, UI_ICON_DATA[icon]);
  tft.setSwapBytes(false);
}

void card(int x, int y, int w, int h, int icon, uint16_t iconCol,
          const char* title, const char* l1, const char* l2) {
  uiPanel(x, y, w, h);

  drawUiIcon((uint8_t)icon, x + 6, y + 6);

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
  uiStat(x, y, w, h, label, value, col, true);
}

// mala dlazdice: popisek nahore, hodnota dole
// EN: small tile: label on top, value below
void statBox(int x, int y, int w, int h, const char* label,
          const char* value, uint16_t col) {
  uiStat(x, y, w, h, label, value, col, false);
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

// Cele hodnoty oddeluje mezerou po tisicich a umi pridat jednotku.
// EN: Whole values are grouped by thousands and can receive a unit suffix.
const char* fmtGrouped(uint32_t value, const char* suffix = nullptr) {
  fidx = (fidx + 1) % 8;
  char digits[12];
  snprintf(digits, sizeof(digits), "%lu", (unsigned long)value);
  int len = strlen(digits), out = 0;
  for (int i = 0; i < len; ++i) {
    if (i > 0 && ((len - i) % 3) == 0) fbuf[fidx][out++] = ' ';
    fbuf[fidx][out++] = digits[i];
  }
  if (suffix && suffix[0]) {
    fbuf[fidx][out++] = ' ';
    for (int i = 0; suffix[i] && out < (int)sizeof(fbuf[fidx]) - 1; ++i)
      fbuf[fidx][out++] = suffix[i];
  }
  fbuf[fidx][out] = '\0';
  return fbuf[fidx];
}

// Castka vzdy pouziva aktualne zvolenou menu a oddelovac tisicu.
// EN: Money always uses the selected currency and a thousands separator.
const char* fmtMoney(float value) {
  double rounded = floor((double)value + 0.5);
  if (rounded < 0) rounded = 0;
  if (rounded > 4294967295.0) rounded = 4294967295.0;
  return fmtGrouped((uint32_t)rounded, CFG_CURR);
}

const char* weatherText(int code) {
  static const int labels[] = {T_W_CLEAR, T_W_PARTLY, T_W_OVERCAST, T_W_FOG,
    T_W_DRIZZLE, T_W_RAIN, T_W_SNOW, T_W_SHOWERS, T_W_SNOWSH, T_W_STORM};
  return TR(labels[weatherKind(code)]);
}

// ===========================================================================
// OBRAZOVKA 0 - PREHLED / SCREEN 0 - OVERVIEW
// ===========================================================================
void scrOverview() {
  // ---- karty / cards ----
  static char invUse[24];
  snprintf(invUse, sizeof(invUse), "%s %.0f %%", TR(T_USAGE), v_load_pct);
  card(6,    44, 150, 56, UI_ICON_INVERTER, C_LOAD, TR(T_INVERTER),
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
  card(164,  44, 150, 56, UI_ICON_SOLAR, C_PV, TR(T_SOLARPV),
       fmtPower(v_pv_power), pvUI);

  // napeti a frekvence site; bez mezer kolem lomitka, jinak se to do karty nevejde
  // EN: grid voltage and frequency; no spaces around the slash or it will not fit
  static char gridVF[24];
  snprintf(gridVF, sizeof(gridVF), "%.0fV/%.1fHz", v_grid_voltage, v_grid_freq);
  card(6,   104, 150, 56, UI_ICON_GRID, C_GRID, TR(T_GRID),
       gridVF, fmtPower(v_grid_power));

  card(164, 104, 150, 56, UI_ICON_BATTERY, C_BATT, TR(T_BATTERY),
       fmt("%.1f V", v_batt_voltage), fmt("%.0f %%", v_soc));

  // proud baterie hned za procenty, zeleny pri nabijeni, cerveny pri vybijeni
  // EN: battery current right after the percentage, green charging, red discharging
  static char curBuf[16];
  snprintf(curBuf, sizeof(curBuf), "%c%.0fA",
           v_batt_power >= 0 ? '+' : '-', fabsf(v_batt_current));
  czOn();
  int wSoc = tft.textWidth(fmt("%.0f %%", v_soc));
  tft.setTextColor(v_batt_power >= 0 ? C_BATT : C_GRID, C_CARD);
  tCz(curBuf, 164 + 50 + wSoc + 8, 104 + 39);

  // ---- predikce vyroby na cely den / whole-day production forecast ----
  uiPanel(6, 164, 308, 64, C_PV);

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
  snprintf(savBuf, sizeof(savBuf), "%s %s / %s",
           TR(T_TODAY), fmtMoney(savedToday()), fmtMoney(savedAll));
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
  float shownLoad = animatedGauge(GM_LOAD), shownPv = animatedGauge(GM_PV);
  float shownGrid = animatedGauge(GM_GRID), shownBatt = animatedGauge(GM_BATT);
  halfGauge(84,  300, 62, 13, shownLoad, CFG_RANGE, C_LOAD,
            fmtPower(v_load_power), TR(T_LOAD));
  halfGauge(236, 300, 62, 13, shownPv, CFG_RANGE, C_PV,
            fmtPower(v_pv_power), TR(T_SOLARPV));
  halfGauge(84,  394, 62, 13, fabsf(shownGrid), CFG_RANGE,
            shownGrid > 1 ? C_GRID : C_DIM,
            fmtPower(fabsf(v_grid_power)), TR(T_GRID));
  // vykon se znamenkem: kladny se nabiji, zaporny vybiji
  // EN: signed power: positive is charging, negative is discharging
  halfGauge(236, 394, 62, 13, fabsf(shownBatt), (CFG_RANGE / 2),
            shownBatt >= 0 ? C_BATT : C_GRID,
            fmtSigned(v_batt_power),
            shownBatt >= 0 ? TR(T_BATT_CHG) : TR(T_BATT_DIS));
}

// ===========================================================================
// OBRAZOVKA 1 - BATERIE / SCREEN 1 - BATTERY
// ===========================================================================
void scrBattery() {
  const int cx = 160, cy = 208, r = 118, th = 22;
  float shownSoc = animatedGauge(GM_SOC);
  // zeleny prstenec pri nabijeni, cerveny pri vybijeni
  // EN: green ring while charging, red while discharging
  uint16_t col = v_batt_power >= 0 ? C_BATT : C_GRID;
  // pod 20 % zustane cislo cervene i pri nabijeni, aby varovani neslo prehlednout
  // EN: below 20 % the number stays red even while charging, so the warning cannot be missed
  uint16_t numCol = v_soc < 20 ? C_GRID : col;

  arcRing(cx, cy, r - th, r, 180, 360, C_TRACK);
  arcRing(cx, cy, r - th, r, 180, 180 + 180 * constrain(shownSoc / 100.0f, 0.0f, 1.0f), col);
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

  const char* minText = fmt("%.0f %%", (float)minSoc);
  // Optimisticky odhad: cela zbyvajici predikovana vyroba muze do baterie.
  float forecastSoc = v_batt_capacity > 0 ? min(100.0f, v_soc + w_pv_remaining / v_batt_capacity * 100.0f) : v_soc;
  const char* eveningText = fmt("%.0f %%", forecastSoc);

  // Prave sloupce jsou zarovnane podle skutecne sirky hodnot. Popisek
  // vecerniho SOC proto nikdy nezasahne do procenta ani do leveho sloupce.
  czOn();
  int eveningValueLeft = 314 - tft.textWidth(eveningText);
  int eveningLabelRight = eveningValueLeft - 8;

  tft.setTextColor(C_DIM, C_BG);
  tCz(TR(T_MIN_SHORT), 6, 396);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_TXT, C_BG);
  tCz(minText, 150, 396);
  tft.setTextColor(C_DIM, C_BG); tCz(TR(T_SOC_EVENING), eveningLabelRight, 396);
  tft.setTextColor(C_BATT, C_BG); tCz(eveningText, 314, 396);
  tft.setTextDatum(TL_DATUM);
}

// ===========================================================================
// OBRAZOVKA 2 - SOLAR / SCREEN 2 - SOLAR
// ===========================================================================
void scrSolar() {
  float shownPv = animatedGauge(GM_PV);
  halfGauge(160, 160, 100, 20, shownPv, CFG_RANGE, C_PV,
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
  float shownGrid = animatedGauge(GM_GRID), shownLoad = animatedGauge(GM_LOAD);
  halfGauge(84,  150, 68, 15, fabsf(shownGrid), CFG_RANGE,
            shownGrid > 1 ? C_GRID : C_DIM,
            fmtPower(fabsf(v_grid_power)),
            shownGrid > 0 ? TR(T_GRID_IMP) : TR(T_GRID_NONE));
  halfGauge(236, 150, 68, 15, shownLoad, CFG_RANGE, C_LOAD,
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
  uiPanel(6, 48, 308, 92);
  weatherIcon(18, 64, code, w_is_day <= 0.5f);

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

  float shownCloud = animatedGauge(GM_CLOUD);
  halfGauge(160, 213, 62, 13, shownCloud, 100, C_DIM,
            fmt("%.0f %%", w_cloud), TR(T_CLOUDS));

  // Exact linear day/night ratio; unlike bar(), a zero share stays zero.
  int dayLen = sunDayMinutes;
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_PV, C_BG); tCz(TR(T_DAY), 48, 177);
  tCz(dayLen >= 0 ? fmt("%.0f %%", dayPercent(dayLen)) : "--", 48, 198);
  tft.setTextColor(C_LOAD, C_BG); tCz(TR(T_NIGHT), 272, 177);
  tCz(dayLen >= 0 ? fmt("%.0f %%", 100 - dayPercent(dayLen)) : "--", 272, 198);
  tft.setTextDatum(TL_DATUM);
  tft.fillRoundRect(12, 242, 296, 12, 6, C_LINE);
  if (dayLen >= 0) {
    int dayWidth = (int)((int32_t)dayLen * 292 / 1440);
    tft.fillRect(14, 245, dayWidth, 6, C_PV);
    tft.fillRect(14 + dayWidth, 245, 292 - dayWidth, 6, C_LOAD);
  }

  const int sy = 262, sh = 48, sg = 5;
  statBox(6,   sy,           150, sh, TR(T_WIND),    fmt("%.1f km/h", w_wind), C_WEATH);
  statBox(164, sy,           150, sh, TR(T_IRRAD),   fmt("%.0f W/m2", w_irradiance), C_PV);
  statBox(6,   sy+sh+sg,     150, sh, TR(T_SUNRISE), hhmm(sunRise), C_PV);
  statBox(164, sy+sh+sg,     150, sh, TR(T_SUNSET),  hhmm(sunSet), C_LOAD);

  // delka dne a noci se odvodi z vychodu a zapadu
  // EN: day and night length follow from sunrise and sunset
  int nightLen = dayLen >= 0 ? 1440 - dayLen : -1;
  statBox(6,   sy+2*(sh+sg), 150, sh, TR(T_DAY_LEN),   hhmm(dayLen), C_PV);
  statBox(164, sy+2*(sh+sg), 150, sh, TR(T_NIGHT_LEN), hhmm(nightLen), C_LOAD);
}

// ===========================================================================
// OBRAZOVKA 5 - MENIC / SCREEN 5 - INVERTER
// ===========================================================================
void scrInverter() {
  float shownTemp = animatedGauge(GM_TEMP);
  uint16_t tc = shownTemp > 70 ? C_GRID : (shownTemp > 55 ? C_PV : C_BATT);
  halfGaugeU(160, 160, 100, 20, shownTemp, 100, tc,
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

// Prerusovana cara ukazuje optimisticky stav do zapadu slunce: cela zbyvajici
// predikovana energie FVE by mohla nabit baterii.
void plotSocForecast(int gy, int gh) {
  if (curSlot < 0 || v_batt_capacity <= 0 || sunSet <= 0) return;
  int endSlot = constrain(sunSet / 10, 0, DAY_N - 1);
  if (endSlot <= curSlot) return;
  float endSoc = min(100.0f, v_soc + w_pv_remaining / v_batt_capacity * 100.0f);
  int x0 = PLOT_X0 + curSlot * PLOT_STEP;
  int y0 = gy + gh - 1 - (int)(v_soc * (gh - 2) / 100.0f);
  int x1 = PLOT_X0 + endSlot * PLOT_STEP;
  int y1 = gy + gh - 1 - (int)(endSoc * (gh - 2) / 100.0f);
  for (int x = x0; x < x1; x += 5) {
    int xe = min(x + 2, x1);
    int ya = y0 + (int)((long)(y1 - y0) * (x - x0) / (x1 - x0));
    int yb = y0 + (int)((long)(y1 - y0) * (xe - x0) / (x1 - x0));
    tft.drawLine(x, ya, xe, yb, C_PV);
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

int nearestGraphSlot(int wanted) {
  wanted = constrain(wanted, 0, DAY_N - 1);
  if (dHas[wanted]) return wanted;
  for (int distance = 1; distance < DAY_N; ++distance) {
    int left = wanted - distance, right = wanted + distance;
    if (left >= 0 && dHas[left]) return left;
    if (right < DAY_N && dHas[right]) return right;
  }
  return -1;
}

// Kurzor se popise na opacne strane grafu, aby ramecek nezakryl vybrany bod.
// EN: The label is placed opposite the cursor so it does not cover the point.
void graphCursorOverlay(int graphId, int gy, int gh, const char* label,
                        int y1, uint16_t col1, int y2, uint16_t col2) {
  if (!graphCursor.active || graphCursor.screenId != screen ||
      graphCursor.graphId != graphId || graphCursor.slot < 0) return;
  int x = PLOT_X0 + graphCursor.slot * PLOT_STEP;
  tft.drawFastVLine(x, gy + 1, gh - 2, C_TXT);
  tft.fillCircle(x, y1, 3, col1);
  if (y2 >= 0) tft.fillCircle(x, y2, 3, col2);

  czOn();
  int boxW = min(GR_WIDTH - 8, tft.textWidth(label) + 12);
  int boxX = x > GR_L + GR_WIDTH / 2 ? GR_L + 4 : GR_L + GR_WIDTH - boxW - 4;
  tft.fillRoundRect(boxX, gy + 4, boxW, 22, 4, C_CARD);
  tft.drawRoundRect(boxX, gy + 4, boxW, 22, 4, C_TXT);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TXT, C_CARD);
  tCz(label, boxX + boxW / 2, gy + 15);
  tft.setTextDatum(TL_DATUM);
}

bool selectGraphPoint(int x, int y) {
  if (x < GR_L || x >= GR_L + GR_WIDTH) return false;
  int graphId = -1;
  if (screen == SCR_GRAPHS) {
    if (y >= 68 && y < 158) graphId = 0;
    else if (y >= 202 && y < 292) graphId = 1;
    else if (y >= 336 && y < 396) graphId = 2;
  } else if (screen == SCR_TEMPERATURES) {
    if (y >= 82 && y < 202) graphId = 0;
    else if (y >= 272 && y < 392) graphId = 1;
  }
  if (graphId < 0) return false;

  int wanted = (x - PLOT_X0 + PLOT_STEP / 2) / PLOT_STEP;
  int slot = nearestGraphSlot(wanted);
  if (slot < 0) return true;
  graphCursor = {(int8_t)screen, (int8_t)graphId, (int16_t)slot, true};
  return true;
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
  if (graphCursor.active && graphCursor.screenId == screen && graphCursor.graphId == 0) {
    int slot = graphCursor.slot;
    char selected[48];
    snprintf(selected, sizeof(selected), "%02d:%02d  %d / %d W",
             slot / 6, (slot % 6) * 10, dPv[slot], dLoad[slot]);
    int pvY = 68 + 89 - (int)((long)constrain((int)dPv[slot], 0, maxPower) * 88 / maxPower);
    int loadY = 68 + 89 - (int)((long)constrain((int)dLoad[slot], 0, maxPower) * 88 / maxPower);
    graphCursorOverlay(0, 68, 90, selected, pvY, C_PV, loadY, C_LOAD);
  }

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
  if (graphCursor.active && graphCursor.screenId == screen && graphCursor.graphId == 1) {
    int slot = graphCursor.slot;
    char selected[40];
    snprintf(selected, sizeof(selected), "%02d:%02d  %+d W",
             slot / 6, (slot % 6) * 10, dBatt[slot]);
    int battY = 202 + 45 - (int)((long)constrain((int)dBatt[slot], -maxB, maxB) * 43 / maxB);
    graphCursorOverlay(1, 202, 90, selected, battY, C_BATT, -1, C_BATT);
  }

  // stav nabiti / state of charge
  graphFrame(336, 60, TR(T_SOC), fmt("%.0f %%", v_soc), C_BATT);
  plotDaySoc(336, 60, C_BATT);
  plotSocForecast(336, 60);
  yAxis(336, 60, "100", "50", "0");
  if (graphCursor.active && graphCursor.screenId == screen && graphCursor.graphId == 2) {
    int slot = graphCursor.slot;
    char selected[32];
    snprintf(selected, sizeof(selected), "%02d:%02d  %u %%",
             slot / 6, (slot % 6) * 10, (unsigned)dSoc[slot]);
    int socY = 336 + 59 - (int)((long)dSoc[slot] * 58 / 100);
    graphCursorOverlay(2, 336, 60, selected, socY, C_BATT, -1, C_BATT);
  }

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
  if (graphCursor.active && graphCursor.screenId == screen && graphCursor.graphId == 0) {
    int slot = graphCursor.slot;
    char selected[36];
    snprintf(selected, sizeof(selected), "%02d:%02d  %.1f °C",
             slot / 6, (slot % 6) * 10, dInvTemp[slot] / 10.0f);
    int value = constrain((int)dInvTemp[slot], 0, 1000);
    int pointY = 82 + 119 - (int)((long)value * 118 / 1000);
    graphCursorOverlay(0, 82, 120, selected, pointY, C_PV, -1, C_PV);
  }

  tft.setTextColor(C_DIM, C_BG); tCz(TR(T_MIN_MAX), 6, 237);
  tft.setTextDatum(TR_DATUM); tft.setTextColor(C_WEATH, C_BG); tCz(outExt, 314, 237); tft.setTextDatum(TL_DATUM);
  graphFrame(272, 120, TR(T_OUT_TEMP), fmt("%.1f °C", w_temp), C_WEATH);
  plotDayRange(272, 120, dOutTemp, -200, 400, C_WEATH);
  yAxis(272, 120, "40", "10", "-20");
  if (graphCursor.active && graphCursor.screenId == screen && graphCursor.graphId == 1) {
    int slot = graphCursor.slot;
    char selected[36];
    snprintf(selected, sizeof(selected), "%02d:%02d  %.1f °C",
             slot / 6, (slot % 6) * 10, dOutTemp[slot] / 10.0f);
    int value = constrain((int)dOutTemp[slot], -200, 400);
    int pointY = 272 + 119 - (int)((long)(value + 200) * 118 / 600);
    graphCursorOverlay(1, 272, 120, selected, pointY, C_WEATH, -1, C_WEATH);
  }
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
uint16_t monthWithToday(uint16_t stored, float today) {
  if (today <= 0) return stored;
  uint32_t add = (uint32_t)constrain(today * 10.0f, 0.0f, 65535.0f);
  uint32_t total = (uint32_t)stored + add;
  return total > 65535UL ? 65535U : (uint16_t)total;
}

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
    if (histRange == 2 && idx == lastMon) {
      pv = monthWithToday(pv, dayPv());
      ld = monthWithToday(ld, dayLoad());
    }
    if (pv > maxV) maxV = pv;
    if (ld > maxV) maxV = ld;
    sPv += pv;
    sLd += ld;
    sSv += histRange == 2 && idx == lastMon ? monthWithToday(hmSave[idx], savedToday()) : (histRange == 2 ? hmSave[idx] : hdSave[idx]);
    sBi += histRange == 2 && idx == lastMon ? monthWithToday(hmBIn[idx], dayBattIn()) : (histRange == 2 ? hmBIn[idx] : hdBIn[idx]);
    sBo += histRange == 2 && idx == lastMon ? monthWithToday(hmBOut[idx], dayBattOut()) : (histRange == 2 ? hmBOut[idx] : hdBOut[idx]);
  }

  const int gx = 32, gy = 74, gw = 282, gh = 140;
  tft.drawRect(gx, gy, gw, gh, C_LINE);
  for (int k = 1; k < 4; k++) {
    int y = gy + gh * k / 4;
    for (int x = gx + 3; x < gx + gw - 2; x += 6) tft.drawPixel(x, y, C_CARD);
  }
  // Svisla stupnice v kWh pomaha porovnat vysku sloupcu.
  // EN: A vertical kWh scale makes the bar heights easier to compare.
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_DIM, C_BG);
  tCz(fmt("%.1f", maxV / 10.0f), gx - 4, gy + 2);
  tCz(fmt("%.1f", maxV / 20.0f), gx - 4, gy + gh / 2);
  tCz("0", gx - 4, gy + gh - 2);
  tft.setTextDatum(TL_DATUM);


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
    if (histRange == 2 && idx == lastMon) {
      pv = monthWithToday(pv, dayPv());
      ld = monthWithToday(ld, dayLoad());
    }
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
  snprintf(sv, sizeof(sv), "%s", fmtMoney(sSv / 10.0f));
  statBox(6, sy + 2 * (sh + sg), 308, sh, TR(T_SAVED), sv, C_BATT);
}

// ===========================================================================
// OBRAZOVKA - USPORY / SCREEN - SAVINGS
// ===========================================================================
void scrSavings() {
  // dnesek / today
  float dPvKwh = dayPv(), dSave = savedToday();

  // aktualni mesic / current month
  float mPv = 0, mLd = 0, mSv = 0;
  if (lastMon >= 0 && lastMon < 12) {
    mPv = hmPv[lastMon] / 10.0f;
    mLd = hmLoad[lastMon] / 10.0f;
    mSv = hmSave[lastMon] / 10.0f;
  }
  float dLdKwh = dayLoad();
  // Mesicni souhrn v NVS obsahuje jen uzavrene dny. Prubezny dnesek se
  // prida pouze pro zobrazeni, aby se pri pulnoci nezapocital podruhe.
  if (lastMon >= 0 && lastMon < 12) {
    mPv += dPvKwh; mLd += dLdKwh; mSv += dSave;
  }
  // rok / year
  float yPv = 0, yLd = 0, ySv = 0;
  for (int i = 0; i < 12; i++) {
    yPv += hmPv[i] / 10.0f; yLd += hmLoad[i] / 10.0f; ySv += hmSave[i] / 10.0f;
  }
  if (lastMon >= 0 && lastMon < 12) {
    yPv += dPvKwh; yLd += dLdKwh; ySv += dSave;
  }

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
  float pv[3] = { dPvKwh, mPv, yPv };
  float ld[3] = { dLdKwh, mLd, yLd };
  float sv[3] = { dSave,  mSv, ySv };

  for (int i = 0; i < 3; i++) {
    int ry = y + i * (rowH + 6);
    uiPanel(6, ry, 308, rowH);

    tft.setTextColor(C_TXT, C_CARD);
    tCz(labels[i], 14, ry + 15);

    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(C_PV, C_CARD);
    tCz(fmt("%.1f kWh", pv[i]), 150, ry + 15);
    tft.setTextColor(C_LOAD, C_CARD);
    tCz(fmt("%.1f kWh", ld[i]), 236, ry + 15);

    char b[24];
    snprintf(b, sizeof(b), "%s", fmtMoney(sv[i]));
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

  const int gx = 32, gw = 282;
  tft.drawRect(gx, gy, gw, gh, C_LINE);
  if (hdCount > 0) {
    int n = hdCount < 14 ? hdCount : 14;
    int first = (hdPos - n + HIST_DAYS) % HIST_DAYS;
    int maxV = 10;
    for (int i = 0; i < n; i++) {
      int idx = (first + i) % HIST_DAYS;
      if (hdSave[idx] > maxV) maxV = hdSave[idx];
    }
    int step = gw / n;
    tft.setTextColor(C_LINE, C_BG);
    for (int j = 1; j < 4; ++j)
      for (int xx = gx + 3; xx < gx + gw - 2; xx += 6)
        tft.drawPixel(xx, gy + (gh * j) / 4, C_LINE);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(C_DIM, C_BG);
    tCz(fmt("%.0f", maxV / 10.0f), gx - 4, gy + 2);
    tCz(fmt("%.0f", maxV / 20.0f), gx - 4, gy + gh / 2);
    tCz("0", gx - 4, gy + gh - 2);
    tft.setTextDatum(MC_DATUM);
    for (int i = 0; i < n; i++) {
      int idx = (first + i) % HIST_DAYS;
      int hh = (int)((long)hdSave[idx] * (gh - 4) / maxV);
      tft.fillRect(gx + i * step + 1, gy + gh - hh - 1, step - 3, hh, C_BATT);

      // popisek dne pod sloupcem, u hustsich grafu jen kazdy druhy
      // EN: day label under the bar, every other one when the chart is dense
      if (n <= 10 || i % 2 == 0) {
        tft.setTextColor(C_DIM, C_BG);
        tCz(fmt("%.0f", (float)hdDayNum[idx]), gx + i * step + step / 2, gy + gh + 12);
      }
      if (savingsSelected == idx)
        tft.drawFastVLine(gx + i * step + step / 2, gy + 1, gh - 2, C_TXT);
    }
    tft.setTextDatum(TL_DATUM);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(C_DIM, C_BG);
    char b[32];
    if (savingsSelected >= 0) {
      snprintf(b, sizeof(b), "%u: %s", hdDayNum[savingsSelected],
               fmtMoney(hdSave[savingsSelected] / 10.0f));
      tft.setTextColor(C_BATT, C_BG);
    } else {
      snprintf(b, sizeof(b), "%s", fmtMoney(maxV / 10.0f));
      tft.setTextColor(C_DIM, C_BG);
    }
    tCz(b, 314, gy - 21);
    tft.setTextDatum(TL_DATUM);
  }
}

// Vybere sloupec úspor podle místa dotyku.
// EN: Select a savings bar from the touch position.
bool selectSavingsPoint(int x, int y) {
  const int gx = 32, gw = 282, gy = 328, gh = 70;
  if (hdCount <= 0 || x < gx || x >= gx + gw || y < gy || y >= gy + gh) return false;
  int n = hdCount < 14 ? hdCount : 14;
  int first = (hdPos - n + HIST_DAYS) % HIST_DAYS;
  int step = gw / n;
  int i = constrain((x - gx) / step, 0, n - 1);
  savingsSelected = (int8_t)((first + i) % HIST_DAYS);
  return true;
}

// Vybere mesic predikce podle mista dotyku.
// EN: Select a forecast month from the touch position.
bool selectForecastPoint(int x, int y) {
  const int gx = 32, gw = 282, gy = 196, gh = 184;
  if (x < gx || x >= gx + gw || y < gy || y >= gy + gh) return false;
  int step = gw / 12;
  forecastSelected = (int8_t)constrain((x - gx) / step, 0, 11);
  return true;
}

// ===========================================================================
// OBRAZOVKA - DNES A VCERA / SCREEN - TODAY AND YESTERDAY
// ===========================================================================
void scrCompare() {
  float todayPv = dayPv(), todayLoad = dayLoad(), todaySave = savedToday();
  float yPv = yesterday.valid ? yesterday.pv / 10.0f : 0;
  float yLoad = yesterday.valid ? yesterday.load / 10.0f : 0;
  float ySave = yesterday.valid ? yesterday.save / 10.0f : 0;

  const int todayX = 205, yesterdayX = 278;
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_BATT, C_BG); tCz(TR(T_TODAY), todayX, 50);
  tft.setTextColor(C_DIM, C_BG);  tCz(TR(T_YESTERDAY), yesterdayX, 50);
  tft.setTextDatum(TL_DATUM);

  const char* labels[3] = { TR(T_PRODUCTION), TR(T_CONSUMPT), TR(T_SAVED) };
  float now[3] = { todayPv, todayLoad, todaySave };
  float old[3] = { yPv, yLoad, ySave };
  for (int i = 0; i < 3; i++) {
    int y = 64 + i * 38;
    uiPanel(6, y, 308, 34);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_TXT, C_CARD); tCz(labels[i], 62, y + 17);
    tft.setTextColor(i == 2 ? C_BATT : (i == 0 ? C_PV : C_LOAD), C_CARD);
    if (i == 2) tCz(fmtMoney(now[i]), todayX, y + 17); else tCz(fmt("%.1f kWh", now[i]), todayX, y + 17);
    tft.setTextColor(C_DIM, C_CARD);
    if (!yesterday.valid) tCz("--", yesterdayX, y + 17);
    else if (i == 2) tCz(fmtMoney(old[i]), yesterdayX, y + 17); else tCz(fmt("%.1f kWh", old[i]), yesterdayX, y + 17);
    tft.setTextDatum(TL_DATUM);
  }

  uiPanel(6, 182, 308, 36);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TXT, C_CARD); tCz(TR(T_MAX_LOAD), 62, 200);
  tft.setTextColor(C_LOAD, C_CARD); tCz(fmtPower(maxLoad), todayX, 200);
  tft.setTextColor(C_DIM, C_CARD); tCz(yesterday.valid ? fmtPower(yesterday.maxLoad) : "--", yesterdayX, 200);
  tft.setTextDatum(TL_DATUM);

  const int gx = 6, gy = 250, gw = 308, gh = 154;
  float top = max(max(todayPv, todayLoad), max(yPv, yLoad)); if (top < 1) top = 1;
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_DIM, C_BG); tCz(TR(T_COMPARE_GRAPH), SCR_W / 2, 230);
  tft.setTextDatum(TL_DATUM);
  tft.drawRect(gx, gy, gw, gh, C_LINE);
  // Jemne vodici linky zlepsi odečet vysky sloupcu.
  for (int k = 1; k < 4; k++) {
    int y = gy + gh * k / 4;
    for (int x = gx + 3; x < gx + gw - 2; x += 6) tft.drawPixel(x, y, C_CARD);
  }
  // Dva sousedni sloupce pro kazdy den: vyroba FVE a spotreba.
  // EN: Two adjacent columns per day: photovoltaic production and consumption.
  float values[4] = { yPv, yLoad, todayPv, todayLoad };
  uint16_t cols[4] = { C_PV, C_LOAD, C_PV, C_LOAD };
  const int barX[4] = { 30, 82, 188, 240 };
  for (int i = 0; i < 4; i++) {
    int h = (int)(values[i] * (gh - 60) / top);
    tft.fillRect(gx + barX[i], gy + gh - h - 22, 30, h, cols[i]);
  }
  char oldText[20], nowText[20];
  snprintf(oldText, sizeof(oldText), "%.1f / %.1f kWh", yPv, yLoad);
  snprintf(nowText, sizeof(nowText), "%.1f / %.1f kWh", todayPv, todayLoad);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_DIM, C_BG); tCz(TR(T_YESTERDAY), gx + 79, gy + 11);
  tft.setTextColor(C_BATT, C_BG); tCz(TR(T_TODAY), gx + 229, gy + 11);
  tft.setTextColor(C_TXT, C_BG);
  tAs(oldText, gx + 79, gy + 29, 2); tAs(nowText, gx + 229, gy + 29, 2);
  tft.setTextColor(C_PV, C_BG);
  tCz("FV", gx + 45, gy + gh - 9); tCz("FV", gx + 203, gy + gh - 9);
  tft.setTextColor(C_LOAD, C_BG);
  tCz(TR(T_CONSUMPT), gx + 97, gy + gh - 9); tCz(TR(T_CONSUMPT), gx + 255, gy + gh - 9);
  tft.setTextDatum(TL_DATUM);
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

  const int gx = 32, gy = 196, gw = 282, gh = 184;
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
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_DIM, C_BG);
  tCz(fmt("%.0f", maxV), gx - 4, gy + 2);
  tCz(fmt("%.0f", maxV / 2.0f), gx - 4, gy + gh / 2);
  tCz("0", gx - 4, gy + gh - 2);
  for (int j = 1; j < 4; ++j)
    for (int xx = gx + 3; xx < gx + gw - 2; xx += 6)
      tft.drawPixel(xx, gy + (gh * j) / 4, C_LINE);

  const int step = gw / 12;
  for (int i = 0; i < 12; i++) {
    int x = gx + i * step;
    int planH = (int)(plannedSave(i) * (gh - 4) / maxV);
    int actualH = (int)(actualMonthSave(i) * (gh - 4) / maxV);
    tft.fillRect(x + 2, gy + gh - planH - 1, 9, planH, C_PV);
    if (actualH > 0) tft.fillRect(x + 12, gy + gh - actualH - 1, 9, actualH, C_BATT);
    if (forecastSelected == i)
      tft.drawFastVLine(x + step / 2, gy + 1, gh - 2, C_TXT);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_DIM, C_BG);
    tCz(fmt("%.0f", (float)(i + 1)), x + step / 2, gy + gh + 12);
  }
  if (forecastSelected >= 0 && forecastSelected < 12) {
    char planBuf[18], actualBuf[18], selectedBuf[44];
    snprintf(planBuf, sizeof(planBuf), "%s", fmtMoney(plannedSave(forecastSelected)));
    snprintf(actualBuf, sizeof(actualBuf), "%s", fmtMoney(actualMonthSave(forecastSelected)));
    snprintf(selectedBuf, sizeof(selectedBuf), "%d: %s / %s", forecastSelected + 1, planBuf, actualBuf);
    // Hodnota patri dovnitr grafu, aby neprepisovala jeho nadpis a meritko.
    // EN: Keep the value inside the chart so it does not overwrite its title or scale.
    czOn();
    int boxW = min(gw - 8, tft.textWidth(selectedBuf) + 12);
    int boxX = gx + gw - boxW - 4;
    tft.fillRoundRect(boxX, gy + 4, boxW, 22, 4, C_CARD);
    tft.drawRoundRect(boxX, gy + 4, boxW, 22, 4, C_TXT);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_BATT, C_CARD);
    tCz(selectedBuf, boxX + boxW / 2, gy + 15);
  }
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_DIM, C_BG);
  tCz(TR(T_PLAN_VS_ACTUAL), SCR_W / 2, 404);
  tft.setTextDatum(TL_DATUM);
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

// Manual payback ledger. One versioned NVS blob makes each edit atomic.
// Amounts/energy are bounded integers; calculations use double, never API data.
struct RoiState {
  uint32_t version, investment, kwh;
  uint16_t year;
  uint8_t month, day;
};
RoiState roi = {1, 0, 0, 0, 1, 1};
const uint32_t ROI_MAX = 9999999;
const uint32_t ROI_STEPS[] = {1, 10, 100, 1000, 10000};
uint8_t roiMoneyStep = 3, roiEnergyStep = 1, roiDatePart = 0;
bool roiSaveFailed = false;

constexpr int roiMonthDays(int year, int month) {
  const uint8_t days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (month < 1 || month > 12) return 0;
  return days[month-1] + (month == 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

constexpr bool roiValidDate(int year, int month, int day) {
  return year >= 2000 && year <= 2199 && day >= 1 && day <= roiMonthDays(year, month);
}

constexpr int roiDayNumber(int year, int month, int day) {
  int result = day - 1;
  for (int y = 2000; y < year; ++y) result += 337 + roiMonthDays(y, 2);
  for (int m = 1; m < month; ++m) result += roiMonthDays(year, m);
  return result;
}

constexpr uint32_t roiAdjust(uint32_t value, uint32_t step, bool increase) {
  return increase ? (step > ROI_MAX - value ? ROI_MAX : value + step)
                  : (value < step ? 0 : value - step);
}

// Compile-time regression checks run on the same functions as the firmware.
static_assert(roiValidDate(2024, 2, 29) && !roiValidDate(2100, 2, 29), "ROI leap years");
static_assert(!roiValidDate(2026, 0, 1) && !roiValidDate(2026, 4, 31), "ROI invalid dates");
static_assert(roiDayNumber(2025, 1, 1) - roiDayNumber(2024, 1, 1) == 366, "ROI leap interval");
static_assert(roiDayNumber(2026, 9, 19) - roiDayNumber(2024, 9, 19) == 730, "ROI elapsed days");
static_assert(roiAdjust(0, 10000, false) == 0, "ROI lower bound");
static_assert(roiAdjust(ROI_MAX - 1, 10000, true) == ROI_MAX, "ROI upper bound");
static_assert(roiAdjust(6000, 10, true) == 6010, "ROI manual increment");

void roiLoad() {
  RoiState stored = {};
  if (prefs.getBytesLength("roi1") != sizeof(stored)) return;
  if (prefs.getBytes("roi1", &stored, sizeof(stored)) != sizeof(stored)) return;
  if (stored.version != 1 || stored.investment > ROI_MAX || stored.kwh > ROI_MAX) return;
  if (stored.year != 0 && !roiValidDate(stored.year, stored.month, stored.day)) return;
  roi = stored;
}

void roiButton(int x, int y, const char* label) {
  tft.fillRoundRect(x, y, 42, 34, 5, C_LINE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TXT, C_LINE);
  tCz(label, x + 21, y + 17);
  tft.setTextDatum(TL_DATUM);
}

void roiRow(int y, const char* label, const char* value, uint32_t step) {
  uiPanel(6, y, 308, 70);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_DIM, C_CARD);
  tCz(label, 92, y + 14);
  char text[32];
  snprintf(text, sizeof(text), "%s %s", TR(T_ROI_STEP), fmtGrouped(step));
  tCz(text, 242, y + 14);
  roiButton(12, y + 31, "-"); roiButton(266, y + 31, "+");
  tft.setTextDatum(MC_DATUM); tft.setTextColor(C_TXT, C_CARD);
  tCz(value, 160, y + 51);
  tft.setTextDatum(TL_DATUM);
}

void scrRoi() {
  char text[48];
  snprintf(text, sizeof(text), "%s", fmtMoney(roi.investment));
  roiRow(46, TR(T_ROI_INV), text, ROI_STEPS[roiMoneyStep]);
  if (roi.kwh >= 10000) snprintf(text, sizeof(text), "%.3f MWh", roi.kwh / 1000.0);
  else snprintf(text, sizeof(text), "%s kWh", fmtGrouped(roi.kwh));
  roiRow(120, TR(T_ROI_ENERGY), text, ROI_STEPS[roiEnergyStep]);
  snprintf(text, sizeof(text), "%.2f %s/kWh", (double)CFG_PRICE, CFG_CURR);
  tft.setTextDatum(MC_DATUM); tft.setTextColor(C_DIM, C_BG); tCz(text, 160, 202);

  double paid = (double)roi.kwh * CFG_PRICE;
  double remaining = fmax(0.0, (double)roi.investment - paid);
  double fraction = roi.investment ? fmin(1.0, paid / roi.investment) : 0;
  uiPanel(6, 214, 308, 96);
  const int pieX = 78, pieY = 262;
  tft.fillCircle(pieX, pieY, 40, C_LINE);
  arcRing(pieX, pieY, 0, 40, -90, -90 + fraction * 360, C_BATT);
  tft.fillCircle(pieX, pieY, 26, C_CARD);
  snprintf(text, sizeof(text), "%.0f %%", fraction * 100);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TXT, C_CARD); tCz(text, pieX, pieY);
  tft.setTextColor(C_DIM, C_CARD);
  tCz(TR(T_ROI_PAID), 210, 230); tCz(TR(T_REMAINING), 210, 275);
  snprintf(text, sizeof(text), "%s", fmtMoney(paid));
  tft.setTextColor(C_BATT, C_CARD); tCz(text, 210, 251);
  snprintf(text, sizeof(text), "%s", fmtMoney(remaining));
  tft.setTextColor(C_TXT, C_CARD); tCz(text, 210, 296);

  tft.setTextColor(C_DIM, C_BG); tCz(TR(T_ROI_START), 160, 320);
  roiButton(12, 332, "-"); roiButton(266, 332, "+");
  // Datum se po vykresleni tlacitek vraci na TL; datumove hodnoty musi byt MC.
  // EN: Buttons restore TL datum; date values must use MC datum to stay centred.
  tft.setTextDatum(MC_DATUM);
  if (roi.year == 0) { tft.setTextColor(C_TXT, C_BG); tCz(TR(T_ROI_DATE), 160, 349); }
  else for (int p = 0; p < 3; ++p) {
    snprintf(text, sizeof(text), "%02u", p == 0 ? roi.day : p == 1 ? roi.month : roi.year);
    tft.setTextColor(p == roiDatePart ? C_PV : C_TXT, C_BG);
    tCz(text, 86 + p * 74, 349);
  }
  const char* estimate = TR(T_ROI_WAIT);
  struct tm now;
  if (roi.investment && remaining == 0) estimate = TR(T_ROI_DONE);
  else if (roi.year && roi.investment && paid > 0 && getLocalTime(&now, 5) &&
           roiValidDate(now.tm_year + 1900, now.tm_mon + 1, now.tm_mday)) {
    int days = roiDayNumber(now.tm_year + 1900, now.tm_mon + 1, now.tm_mday) - roiDayNumber(roi.year, roi.month, roi.day);
    if (days > 0) {
      double years = remaining / paid * days / 365.2425;
      if (years > 999) snprintf(text, sizeof(text), ">999 %s", TR(T_ROI_YEARS));
      else snprintf(text, sizeof(text), "%.1f %s", years, TR(T_ROI_YEARS));
      estimate = text;
    }
  }
  uiPanel(6, 374, 308, 42, roiSaveFailed ? C_GRID : C_PV);
  const char* estimateValue = roiSaveFailed ? TR(T_ROI_SAVE_ERR) : estimate;
  // Nadpis zustava vlevo, samotny odhad je velkym fontem uprostred.
  // EN: The label stays left; the estimate itself uses a large centred font.
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_DIM, C_CARD); tCz(TR(T_ROI_EST), 14, 381);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(roiSaveFailed ? C_GRID : C_PV, C_CARD);
  if (roiSaveFailed) tCz(estimateValue, 235, 400);
  else tAs(estimateValue, 235, 400, 4);
  tft.setTextDatum(TL_DATUM);
}

void roiTouch(int x, int y) {
  RoiState before = roi;
  if ((y >= 46 && y < 116) || (y >= 120 && y < 190)) {
    bool money = y < 116;
    int rowY = money ? 46 : 120;
    uint8_t& stepIndex = money ? roiMoneyStep : roiEnergyStep;
    uint32_t& value = money ? roi.investment : roi.kwh;
    if (y < rowY + 31) stepIndex = (stepIndex + 1) % 5;
    else if (x >= 12 && x < 54) value = roiAdjust(value, ROI_STEPS[stepIndex], false);
    else if (x >= 266 && x < 308) value = roiAdjust(value, ROI_STEPS[stepIndex], true);
  } else if (y >= 332 && y < 366) {
    if (!roi.year) {
      struct tm now;
      if (getLocalTime(&now, 5) && roiValidDate(now.tm_year + 1900, now.tm_mon + 1, now.tm_mday)) {
        roi.year = now.tm_year + 1900; roi.month = now.tm_mon + 1; roi.day = now.tm_mday;
      } else { roi.year = 2026; roi.month = roi.day = 1; }
    } else if (x >= 54 && x < 266) roiDatePart = min(2, (x - 54) / 74);
    else if ((x >= 12 && x < 54) || (x >= 266 && x < 308)) {
      int delta = x < 54 ? -1 : 1;
      if (roiDatePart == 0) roi.day = constrain((int)roi.day + delta, 1, roiMonthDays(roi.year, roi.month));
      if (roiDatePart == 1) roi.month = constrain((int)roi.month + delta, 1, 12);
      if (roiDatePart == 2) roi.year = constrain((int)roi.year + delta, 2000, 2199);
      roi.day = min((int)roi.day, roiMonthDays(roi.year, roi.month));
    }
  }
  if (memcmp(&before, &roi, sizeof(roi)) != 0) {
    roiSaveFailed = prefs.putBytes("roi1", &roi, sizeof(roi)) != sizeof(roi);
    if (roiSaveFailed) roi = before;
  }
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
  char memoryText[28];
  snprintf(memoryText, sizeof(memoryText), "%lu kB / %lu",
           (unsigned long)(ESP.getFreeHeap() / 1024),
           (unsigned long)prefs.freeEntries());
  statBoxSmall(164, sy+sh+sg, 150, sh, "RAM / NVS", memoryText, C_TXT);

  int y = sy + 2 * (sh + sg) + 6;
  uiPanel(6, y, 308, 96);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TXT, C_CARD); tCz(TR(T_CONTACT), 160, y + 14);
  tft.setTextColor(C_WEATH, C_CARD);
  tCz("Web  www.pcprovas.cz", 160, y + 37);
  tCz("GitHub  github.com/tomas-cabaj", 160, y + 59);
  tCz("E-mail  t.cabaj@email.cz", 160, y + 81);
  tft.setTextDatum(TL_DATUM);
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
  if (iGridLimit >= OPT_N(OPT_GRID_LIMIT)) iGridLimit = 3;
  if (iGridTime  >= OPT_N(OPT_GRID_TIME))  iGridTime = 3;
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
  iGridLimit = prefs.getUChar("iGridL", 3);
  iGridTime  = prefs.getUChar("iGridT", 3);
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
  prefs.putUChar("iGridL", iGridLimit);
  prefs.putUChar("iGridT", iGridTime);
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
    uiPanel(6, y, 308, S2_H);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_TXT, C_CARD);
    tCz(TR(cfgLabel(i)), 100, y + S2_H / 2);
    tft.setTextColor(C_PV, C_CARD);
    tCz(cfgValue(i), 255, y + S2_H / 2);
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

#define S3_Y0  48
#define S3_H   43
#define S3_STEP 48
#define S3_ROWS 6

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
    case 3: return errLedName();
    case 4: return CFG_GRID_LIMIT == 0 ? TR(T_OFF) : fmt("%.1f kW", CFG_GRID_LIMIT / 1000.0f);
    default: return fmt("%.0f min", (float)CFG_GRID_TIME);
  }
}

int cfg3Label(int row) {
  switch (row) {
    case 0: return T_ALERT_SOC_LIMIT;
    case 1: return T_ALERT_TEMP_LIMIT;
    case 2: return T_ALERT_OFFLINE;
    case 3: return T_ERR_LED;
    case 4: return T_GRID_LIMIT;
    default: return T_GRID_TIME;
  }
}

void scrSettings3() {
  for (int i = 0; i < S3_ROWS; i++) {
    int y = S3_Y0 + i * S3_STEP;
    uiPanel(6, y, 308, S3_H);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_TXT, C_CARD); tCz(TR(cfg3Label(i)), 100, y + S3_H / 2);
    tft.setTextColor(C_PV, C_CARD); tCz(cfg3Value(i), 255, y + S3_H / 2);
    tft.setTextDatum(TL_DATUM);
  }
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_DIM, C_BG); tCz(TR(T_TAP_HINT), SCR_W / 2, 356);
  tft.setTextDatum(TL_DATUM);
}

void cfg3Next(int row) {
  switch (row) {
    case 0: iAlertSoc = (iAlertSoc + 1) % OPT_N(OPT_ALERT_SOC); break;
    case 1: iAlertTemp = (iAlertTemp + 1) % OPT_N(OPT_ALERT_TEMP); break;
    case 2: iStale = (iStale + 1) % OPT_N(OPT_STALE); break;
    case 3: iErrLed = (iErrLed + 1) % OPT_N(OPT_ERR_LED); updateLoadLed(); break;
    case 4: iGridLimit = (iGridLimit + 1) % OPT_N(OPT_GRID_LIMIT); break;
    default: iGridTime = (iGridTime + 1) % OPT_N(OPT_GRID_TIME); break;
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
      uiPanel(6, y, 308, 50);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(e.type == ERR_SOC || e.type == ERR_TEMP ? C_GRID : C_PV, C_CARD);
      tCz(errorName(e.type), 160, y + 13);
      char buf[38];
      if (e.hour < 24 && e.endHour < 24) snprintf(buf, sizeof(buf), "%02u:%02u-%02u:%02u  %u min / %u", e.hour, e.minute, e.endHour, e.endMinute, e.durationMin, e.count);
      else if (e.hour < 24 && e.durationMin > 0) snprintf(buf, sizeof(buf), "%02u:%02u  %u min / %u", e.hour, e.minute, e.durationMin, e.count);
      else if (e.hour < 24) snprintf(buf, sizeof(buf), "%02u:%02u  %s %u", e.hour, e.minute, TR(T_FAILURES), e.count);
      else snprintf(buf, sizeof(buf), "%s %u", TR(T_FAILURES), e.count);
      tft.setTextColor(C_DIM, C_CARD); tCz(buf, 160, y + 36);
      tft.setTextDatum(TL_DATUM);
    }
  }
  uiPanel(6, 354, 308, 50, C_GRID);
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
#define SET_BT3_Y  342         // restart zarizeni / device restart
#define SET_BT3_H   42
#define SET_BT4_X    6
uint32_t restartArmedAt = 0;

void scrSettings() {
  const int sh = 50, sg = 6;
  int y = 52;

  // IP adresa je dlouha, proto na celou sirku
  // EN: the IP address is long, hence full width
  statBoxSmall(6, y, 308, sh, TR(T_BOARD_IP),
               wifiOk ? WiFi.localIP().toString().c_str() : TR(T_NOT_CONN), C_TXT);
  y += sh + sg;

  statBox(6,   y, 150, sh, TR(T_SIGNAL),   fmt("%.0f dBm", (float)WiFi.RSSI()), C_TXT);
  statBoxSmall(164, y, 150, sh, TR(T_UPTIME), uptimeText(), C_TXT);
  y += sh + sg;

  statBoxSmall(6,   y, 150, sh, TR(T_WIFI_NET), ssid, C_DIM);
  statBoxSmall(164, y, 150, sh, TR(T_OTA_NAME), OTA_HOST, C_DIM);

  // tlacitko pro vypis vsech hodnot do Serialu
  // EN: button that dumps every value to Serial
  uiPanel(SET_BTN_X, SET_BTN_Y, SET_BTN_W, SET_BTN_H, C_WEATH);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_WEATH, C_CARD);
  tCz(TR(T_DUMP), SET_BTN_X + SET_BTN_W / 2, SET_BTN_Y + 16);
  tft.setTextColor(C_DIM, C_CARD);
  tCz(TR(T_TO_SERIAL), SET_BTN_X + SET_BTN_W / 2, SET_BTN_Y + 34);

  // tlacitko pro test sitoveho spojeni / button for the network link test
  uiPanel(SET_BT2_X, SET_BTN_Y, SET_BTN_W, SET_BTN_H, C_PV);
  tft.setTextColor(C_PV, C_CARD);
  tCz(TR(T_TEST_CONN), SET_BT2_X + SET_BTN_W / 2, SET_BTN_Y + 16);
  tft.setTextColor(C_DIM, C_CARD);
  tCz(TR(T_TGT_PORTS), SET_BT2_X + SET_BTN_W / 2, SET_BTN_Y + 34);

  // Chraneny restart zarizeni / guarded device restart
  uiPanel(SET_BT4_X, SET_BT3_Y, 308, SET_BT3_H, C_GRID);
  tft.setTextColor(C_GRID, C_CARD);
  tCz(TR(T_RESTART), SCR_W / 2, SET_BT3_Y + 21);
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

  if (cosH > 1.0f)  { sunRise = sunSet = -1; sunDayMinutes = 0; return; }   // polarni noc / polar night
  if (cosH < -1.0f) { sunRise = 0; sunSet = 1439; sunDayMinutes = 1440; return; }  // polarni den / polar day

  // polovina delky dne ve stupnich
  // EN: half the day length in degrees
  float H = acosf(cosH) / RAD;
  float noon = 720.0f - 4.0f * GEO_LON - eot + tzMinutes;

  sunRise = (int)(noon - 4.0f * H);
  sunSet  = (int)(noon + 4.0f * H);
  sunDayMinutes = constrain(sunSet - sunRise, 0, 1440);
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
struct ScreenDefinition { int title; void (*draw)(); };
const ScreenDefinition screenDefinitions[] = {
  {T_APP, scrOverview}, {T_S_BATT, scrBattery}, {T_S_RUNTIME, scrRuntime},
  {T_S_SOLAR, scrSolar}, {T_S_GRID, scrGridLoad}, {T_S_INV, scrInverter},
  {T_S_WEATH, scrWeather}, {T_S_CHART, scrGraphs}, {T_S_TEMP, scrTemperatures},
  {T_S_HISTORY, scrHistory}, {T_S_SAVINGS, scrSavings}, {T_S_FORECAST, scrForecast},
  {T_S_COMPARE, scrCompare}, {T_ROI, scrRoi}, {T_S_ERRORS, scrErrors},
  {T_S_SET3, scrSettings3}, {T_S_SET, scrSettings}, {T_S_SET2, scrSettings2},
  {T_S_ABOUT, scrAbout}
};
static_assert(sizeof(screenDefinitions) / sizeof(screenDefinitions[0]) == SCREENS, "Screen count mismatch");

const char* screenName(int i) {
  return TR(screenDefinitions[i >= 0 && i < SCREENS ? i : SCR_ABOUT].title);
}

// Tematicka ikonka stranky; -1 znamena, ze v zahlavi zustane vice mista.
// EN: Page theme icon; -1 leaves more room in the header.
int pageIcon(int page) {
  switch (page) {
    case SCR_BATTERY: return UI_ICON_BATTERY;
    case SCR_RUNTIME: return UI_ICON_HISTORY;
    case SCR_SOLAR: return UI_ICON_SOLAR;
    case SCR_GRID: return UI_ICON_GRID;
    case SCR_INVERTER: return UI_ICON_INVERTER;
    case SCR_WEATHER: return UI_ICON_WEATHER;
    case SCR_GRAPHS: return UI_ICON_CHART;
    case SCR_TEMPERATURES: return UI_ICON_TEMPERATURE;
    case SCR_HISTORY: return UI_ICON_HISTORY;
    case SCR_SAVINGS: return UI_ICON_SAVINGS;
    case SCR_FORECAST: return UI_ICON_FORECAST;
    case SCR_COMPARE: return UI_ICON_COMPARE;
    case SCR_ROI: return UI_ICON_PAYBACK;
    case SCR_ERRORS: return UI_ICON_OFFLINE;
    case SCR_ALERTS: return UI_ICON_ALERT;
    case SCR_SETTINGS:
    case SCR_SETTINGS2: return UI_ICON_SETTINGS;
    case SCR_ABOUT: return UI_ICON_HOUSE;
    default: return -1;
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

  if (screen == SCR_OVERVIEW) {
    // Na prehledu patri vlevo cely nazev a vpravo hodiny misto odpoctu.
    // EN: The overview shows the full name left and the clock instead of the countdown right.
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(C_TXT, hdrBg);
    tCz(screenName(screen), 8, 10);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(timeOk ? C_TXT : C_DIM, hdrBg);
    tCz(clockStr, SCR_W - 26, 12);
    tft.setTextDatum(TL_DATUM);
  } else {
    // Pred kreslenim se zmeri vsechna tri pole; dlouhe nazvy pouziji dva radky.
    // EN: All three fields are measured before drawing; long titles use two rows.
    czOn();
    int icon = pageIcon(screen);
    int titleX = icon >= 0 ? UI_ICON_SIZE + 8 : 8;
    if (icon >= 0) drawUiIcon((uint8_t)icon, 0, 1);
    int wTitle = tft.textWidth(screenName(screen));
    int wClock = tft.textWidth(clockStr);
    int wStatus = tft.textWidth(buf);
    int cxMin = titleX + wTitle + (wClock + 1) / 2 + 8;
    int cxMax = SCR_W - 34 - wStatus - (wClock + 1) / 2;
    bool twoRows = cxMin > cxMax;
    int clockX = twoRows ? (icon >= 0 ? 46 + (wClock + 1) / 2 : 8 + (wClock + 1) / 2)
                         : constrain(SCR_W / 2, cxMin, cxMax);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(C_TXT, hdrBg);
    tCz(screenName(screen), titleX, twoRows ? 1 : 10);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(timeOk ? C_TXT : C_DIM, hdrBg);
    tCz(clockStr, clockX, twoRows ? 25 : 19);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(stale ? C_TXT : C_DIM, hdrBg);
    tCz(buf, SCR_W - 26, twoRows ? 18 : 12);
    tft.setTextDatum(TL_DATUM);
  }

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

  // Tlacitka maji stin, jemny horni lesk a svetly ramecek jako sada ikon.
  // EN: Buttons use a shadow, subtle top highlight and bright frame like the icon set.
  auto button = [&](int x, bool active) {
    tft.fillRoundRect(x, y + 2, 88, h, 8, C_TRACK);
    tft.fillRoundRect(x, y, 88, h - 2, 8, C_CARD);
    tft.drawRoundRect(x, y, 88, h - 2, 8, active ? C_PV : C_LINE);
  };

  button(8, false);
  tft.setSwapBytes(true);
  tft.pushImage(37, y + 3, UI_NAV_ICON_SIZE, UI_NAV_ICON_SIZE, UI_NAV_LEFT_DATA);

  // domu - na uvodni strance zvyraznene
  // EN: home - highlighted on the overview screen
  button(116, screen == SCR_OVERVIEW);
  tft.pushImage(145, y + 3, UI_NAV_ICON_SIZE, UI_NAV_ICON_SIZE, UI_NAV_HOME_DATA);

  button(224, false);
  tft.pushImage(253, y + 3, UI_NAV_ICON_SIZE, UI_NAV_ICON_SIZE, UI_NAV_RIGHT_DATA);
  tft.setSwapBytes(false);

  // indikator stranky / page indicator
  int dots = SCREENS;
  int dx = SCR_W / 2 - (dots * 10) / 2;
  for (int i = 0; i < dots; i++)
    tft.fillCircle(dx + i * 10 + 4, NAV_Y - 9, 2, i == screen ? C_TXT : C_LINE);
}

// Obsah aktualni obrazovky. / Content of the current screen.
void drawContent() {
  screenDefinitions[screen >= 0 && screen < SCREENS ? screen : SCR_ABOUT].draw();
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
  // Jedna SPI transakce zkrati viditelne mazani a prekresleni cele stranky.
  // EN: One SPI transaction shortens the visible clear-and-redraw interval.
  tft.startWrite();
  tft.fillRect(0, CONT_Y, SCR_W, CONT_H, C_BG);
  drawContent();
  drawHeader();
  drawNav();
  drawFrame();
  tft.endWrite();
}

// Pri animaci se prekresli jen oblast aktualniho ukazatele, ne cela obrazovka.
// EN: During animation only the current gauge area is redrawn, never the full screen.
void drawAnimatedGauges() {
  if (!screenOn) return;
  tft.startWrite();
  switch (screen) {
    case SCR_OVERVIEW: {
      tft.fillRect(14, 230, 140, 96, C_BG);
      tft.fillRect(166, 230, 140, 96, C_BG);
      tft.fillRect(14, 326, 140, 92, C_BG);
      tft.fillRect(166, 326, 140, 92, C_BG);
      float load = animatedGauge(GM_LOAD), pv = animatedGauge(GM_PV);
      float grid = animatedGauge(GM_GRID), batt = animatedGauge(GM_BATT);
      halfGauge(84, 300, 62, 13, load, CFG_RANGE, C_LOAD, fmtPower(v_load_power), TR(T_LOAD));
      halfGauge(236, 300, 62, 13, pv, CFG_RANGE, C_PV, fmtPower(v_pv_power), TR(T_SOLARPV));
      halfGauge(84, 394, 62, 13, fabsf(grid), CFG_RANGE,
                grid > 1 ? C_GRID : C_DIM, fmtPower(fabsf(v_grid_power)), TR(T_GRID));
      halfGauge(236, 394, 62, 13, fabsf(batt), CFG_RANGE / 2,
                batt >= 0 ? C_BATT : C_GRID, fmtSigned(v_batt_power),
                batt >= 0 ? TR(T_BATT_CHG) : TR(T_BATT_DIS));
      break;
    }
    case SCR_BATTERY: {
      const int cx = 160, cy = 208, r = 118, th = 22;
      tft.fillRect(34, 82, 252, 146, C_BG);
      float soc = animatedGauge(GM_SOC);
      uint16_t col = v_batt_power >= 0 ? C_BATT : C_GRID;
      uint16_t numCol = v_soc < 20 ? C_GRID : col;
      arcRing(cx, cy, r - th, r, 180, 360, C_TRACK);
      arcRing(cx, cy, r - th, r, 180, 180 + 180 * constrain(soc / 100.0f, 0.0f, 1.0f), col);
      gaugeTicks(cx, cy, r);
      const char* socStr = fmt("%.0f", v_soc);
      czOff(); int wNum = tft.textWidth(socStr, 7); czOn();
      const int wPct = 12; int x0 = cx - (wNum + 5 + wPct) / 2;
      tft.setTextDatum(TL_DATUM); tft.setTextColor(numCol, C_BG); tAs(socStr, x0, 134, 7);
      tft.setTextColor(C_DIM, C_BG); tCz("%", x0 + wNum + 5, 158);
      char rbuf[24]; snprintf(rbuf, sizeof(rbuf), "%+.1f %%/hod", socRate);
      tft.setTextDatum(MC_DATUM); tft.setTextColor(socRate >= 0 ? C_BATT : C_GRID, C_BG);
      tAs(rbuf, cx, 196, 4);
      tft.setTextColor(v_soc < CFG_ALERT_SOC ? C_GRID : C_TXT, C_BG);
      tCz(v_soc < CFG_ALERT_SOC ? TR(T_ALERT_SOC) :
          (v_batt_power >= 0 ? TR(T_CHARGING) : TR(T_DISCHARGING)), cx, 220);
      tft.setTextDatum(TL_DATUM);
      break;
    }
    case SCR_SOLAR: {
      tft.fillRect(44, 44, 232, 136, C_BG);
      float pv = animatedGauge(GM_PV);
      halfGauge(160, 160, 100, 20, pv, CFG_RANGE, C_PV, fmtPower(v_pv_power), TR(T_PV_NOW));
      break;
    }
    case SCR_GRID: {
      tft.fillRect(10, 78, 300, 96, C_BG);
      float grid = animatedGauge(GM_GRID), load = animatedGauge(GM_LOAD);
      halfGauge(84, 150, 68, 15, fabsf(grid), CFG_RANGE,
                grid > 1 ? C_GRID : C_DIM, fmtPower(fabsf(v_grid_power)),
                grid > 0 ? TR(T_GRID_IMP) : TR(T_GRID_NONE));
      halfGauge(236, 150, 68, 15, load, CFG_RANGE, C_LOAD, fmtPower(v_load_power), TR(T_HOUSE_LOAD));
      break;
    }
    case SCR_WEATHER: {
      tft.fillRect(90, 145, 140, 96, C_BG);
      float cloud = animatedGauge(GM_CLOUD);
      halfGauge(160, 213, 62, 13, cloud, 100, C_DIM, fmt("%.0f %%", w_cloud), TR(T_CLOUDS));
      break;
    }
    case SCR_INVERTER: {
      tft.fillRect(44, 44, 232, 140, C_BG);
      float temp = animatedGauge(GM_TEMP);
      uint16_t tc = temp > 70 ? C_GRID : (temp > 55 ? C_PV : C_BATT);
      halfGaugeU(160, 160, 100, 20, temp, 100, tc, fmt("%.1f", i_temp), "°C", TR(T_INV_TEMP));
      if (i_temp > CFG_ALERT_TEMP) {
        tft.setTextDatum(MC_DATUM); tft.setTextColor(C_GRID, C_BG);
        tCz(TR(T_ALERT_TEMP), SCR_W / 2, 176); tft.setTextDatum(TL_DATUM);
      }
      break;
    }
    default:
      break;
  }
  tft.endWrite();
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

void yesterdaySummarySave() { prefs.putBytes("ySum", &yesterday, sizeof(yesterday)); }
void yesterdaySummaryLoad() {
  size_t n = prefs.getBytes("ySum", &yesterday, sizeof(yesterday));
  if (n != sizeof(yesterday)) memset(&yesterday, 0, sizeof(yesterday));
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

  yesterday.pv = pv; yesterday.load = ld; yesterday.save = sv;
  yesterday.maxLoad = maxLoad < 0 ? 0 : (uint16_t)maxLoad;
  yesterday.minInv = minInvTemp; yesterday.maxInv = maxInvTemp;
  yesterday.minOut = minOutTemp; yesterday.maxOut = maxOutTemp;
  yesterday.day = (uint8_t)mday; yesterday.valid = 1;
  yesterdaySummarySave();

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
  uint32_t days = sec / 86400UL;
  uint32_t years = days / 365UL;
  days %= 365UL;
  uint32_t months = days / 30UL;
  days %= 30UL;
  uint32_t hours = (sec % 86400UL) / 3600UL;
  uint32_t minutes = (sec % 3600UL) / 60UL;
  static char value[32];
  snprintf(value, sizeof(value), "%lur %lum %lud %luh %lum",
           (unsigned long)years, (unsigned long)months, (unsigned long)days,
           (unsigned long)hours, (unsigned long)minutes);
  return value;
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
      Serial.println("  Zkontrolujte adresu API a izolaci klientu na WiFi.");
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

void bootScreen(const char* status, const char* detail, uint16_t col) {
  tft.fillScreen(C_BG);
  drawLogo(54);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TXT, C_BG); tCz("Cabaj Tomáš  2026", 160, 158);
  tft.setTextColor(C_DIM, C_BG); tCz("firmware v" FW_VERSION, 160, 181);
  uiPanel(18, 205, 284, 100);
  tft.setTextColor(C_TXT, C_CARD); tCz(TR(T_CONTACT), 160, 220);
  tft.setTextColor(C_WEATH, C_CARD);
  tCz("www.pcprovas.cz", 160, 245);
  tCz("github.com/tomas-cabaj", 160, 267);
  tCz("t.cabaj@email.cz", 160, 289);
  uiPanel(18, 336, 284, 68, col);
  tft.setTextColor(col, C_CARD); tCz(status, 160, 357);
  if (detail) {
    tft.setTextColor(C_DIM, C_CARD); tCz(detail, 160, 383);
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
  if (screen == SCR_ROI && y < NAV_Y) { roiTouch(x, y); drawScreen(); return; }
  // EN: taps on the overview screen jump to the detail page
  if (screen == SCR_OVERVIEW && y < NAV_Y) {
    int target = -1;
    // Menic / Solarni PV / Inverter
    // EN: Solar PV
    if      (y >=  44 && y < 100) target = (x < 160) ? SCR_INVERTER : SCR_SOLAR;
    else if (y >= 104 && y < 160) target = (x < 160) ? SCR_GRID : SCR_BATTERY;
    // Predikce -> predikce uspor
    // EN: Forecast -> savings forecast
    else if (y >= 164 && y < 228) target = SCR_FORECAST;
    // ukazatel Zatez / PV / Load
    // EN: PV gauge
    else if (y >= 238 && y < 322) target = (x < 160) ? SCR_GRID : SCR_SOLAR;
    // ukazatel Sit / Baterie / Grid
    // EN: Battery gauge
    else if (y >= 332 && y < 416) target = (x < 160) ? SCR_GRID : SCR_RUNTIME;

    if (target >= 0) {
      screen = target;
      drawScreen();
    }
    return;
  }

  // Klepnuti do caroveho grafu vybere nejblizsi ulozeny desetiminutovy bod.
  // EN: A tap in a line chart selects the nearest stored ten-minute point.
  if ((screen == SCR_GRAPHS || screen == SCR_TEMPERATURES) &&
      y < NAV_Y && selectGraphPoint(x, y)) {
    drawScreen();
    return;
  }

  // Klepnuti do grafu uspor vybere denni sloupec.
  // EN: Tapping the savings chart selects its daily bar.
  if (screen == SCR_SAVINGS && selectSavingsPoint(x, y)) {
    drawScreen();
    return;
  }

  // Klepnuti do grafu predikce vybere mesic.
  // EN: Tapping the forecast chart selects a month.
  if (screen == SCR_FORECAST && selectForecastPoint(x, y)) {
    drawScreen();
    return;
  }

  // radky na strance NASTAVENI 2 / rows on the SETTINGS 2 screen
  // klepnuti na graf historie prepne zobrazene obdobi
  // EN: tapping the history chart switches the period shown
  if (screen == SCR_HISTORY && y >= 44 && y < 240) {
    histRange = (histRange + 1) % 3;
    drawScreen();
    return;
  }

  if (screen == SCR_SETTINGS2 && y < NAV_Y) {
    int row = (y - S2_Y0) / S2_STEP;
    if (row >= 0 && row < S2_ROWS && (y - S2_Y0) % S2_STEP <= S2_H) cfgNext(row);
    return;
  }

  if (screen == SCR_ALERTS && y < NAV_Y) {
    int row = (y - S3_Y0) / S3_STEP;
    if (row >= 0 && row < S3_ROWS && (y - S3_Y0) % S3_STEP <= S3_H) cfg3Next(row);
    return;
  }

  if (screen == SCR_ERRORS && y >= 354 && y <= 404) {
    errorClear();
    drawScreen();
    return;
  }

  // tlacitko na strance nastaveni / button on the settings screen
  if (y < NAV_Y) {
    if (screen == SCR_SETTINGS && y >= SET_BTN_Y && y <= SET_BTN_Y + SET_BTN_H) {
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
    else if (screen == SCR_SETTINGS && y >= SET_BT3_Y && y <= SET_BT3_Y + SET_BT3_H) {
      if (restartArmedAt && millis() - restartArmedAt < 5000) {
        histSave();
        delay(150);
        ESP.restart();
      }
      restartArmedAt = millis();
      snprintf(setMsg, sizeof(setMsg), "%s", TR(T_RESTART_AGAIN));
      drawScreen();
    }
    return;
  }

  int prevScreen = screen;
  if      (x < 108)  screen = (screen - 1 + SCREENS) % SCREENS;
  else if (x < 216)  screen = SCR_OVERVIEW;
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
  roiLoad();
  errorLoad();
  sumLoad();
  yesterdaySummaryLoad();
  baseLoadFromNvs();
  // podsviceni pres PWM kvuli jasu
  // EN: backlight on PWM so brightness can be set
  PWM_ATTACH(TFT_BL, 3);
  PWM_WRITE(TFT_BL, 3, CFG_BRIGHT);
  tft.setRotation(CFG_ROT);
  loadOrCalibrate();

  // Kontaktni stranka zustava viditelna, meni se jen stavovy blok dole.
  // EN: The contact page stays visible while only the lower status panel changes.
  bootScreen(TR(T_APP), TR(T_WIFI_CONN), C_WEATH);
  delay(2500);

  bootScreen(TR(T_WIFI_CONN), ssid, C_TXT);
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
    bootScreen(TR(T_WIFI_OK), TR(T_FETCHING), C_BATT);
    dataOk = fetchData();
    if (dataOk) {
      errorStop(ERR_FETCH); errorStop(ERR_API);
      lastOkFetch = millis(); haveFetch = true; pushHistory(); updateSocRate();
      if (v_soc < CFG_ALERT_SOC) errorStart(ERR_SOC); else errorStop(ERR_SOC);
      if (i_temp > CFG_ALERT_TEMP) errorStart(ERR_TEMP); else errorStop(ERR_TEMP);
      updateGridAlert();
    } else { errorStart(apiInvalid ? ERR_API : ERR_FETCH); }
    updateLoadLed();
  } else {
    Serial.println("WiFi se nepodarilo pripojit");
    bootScreen(TR(T_WIFI_ERR), ssid, C_GRID);
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
    float previousGauge[GM_COUNT];
    for (int i = 0; i < GM_COUNT; ++i) previousGauge[i] = gaugeTarget((uint8_t)i);
    bool hadDataBeforeFetch = haveFetch;

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
      updateGridAlert();
      startGaugeAnimation(previousGauge, hadDataBeforeFetch);

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
