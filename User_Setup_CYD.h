// ============================================================================
//  TFT_eSPI setup pro CYD s podsvicenim na GPIO27
//  (ESP32-3248S035 3,5" / ESP32-2432S032 3,2")
// ----------------------------------------------------------------------------
//  Obsah zkopirujte pres:  Arduino\libraries\TFT_eSPI\User_Setup.h
// ============================================================================

// ---- Radic displeje --------------------------------------------------------
// Vyberte PRAVE JEDNU variantu podle uhlopricky displeje:
//
//   3,5"  480x320  -> ST7796_DRIVER    (deska ESP32-3248S035)
//   3,2"  320x240  -> ST7789_DRIVER    (deska ESP32-2432S032)
//
#define ST7796_DRIVER
#define TFT_WIDTH  320
#define TFT_HEIGHT 480

//#define ST7789_DRIVER
//#define TFT_WIDTH  240
//#define TFT_HEIGHT 320

// ---- SPI sbernice displeje -------------------------------------------------
// Bez tohoto radku pouzije TFT_eSPI sbernici VSPI, kterou potrebuje dotyk.
#define USE_HSPI_PORT

// ---- Piny displeje (HSPI) --------------------------------------------------
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1   // RST je na desce natvrdo na EN

// ---- Podsviceni ------------------------------------------------------------
// ZMERENO NA TETO DESCE: GPIO27 (ne 21 jako u 2,8" varianty)
#define TFT_BL   27
#define TFT_BACKLIGHT_ON HIGH

// ---- Barevne poradi --------------------------------------------------------
// Pokud budou cervena a modra prohozene, prepnete TFT_BGR <-> TFT_RGB.
#define TFT_RGB_ORDER TFT_BGR

// ---- Pisma -----------------------------------------------------------------
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// ---- SPI -------------------------------------------------------------------
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  16000000

// ---- Dotyk -----------------------------------------------------------------
// TEST: u 3,5" desky (3248S035R) sdili rezistivni XPT2046 SPI s displejem.
// TFT_eSPI umi sdileni sbernice spravne osetrit - staci mu rici CS pin.
// Pokud se dotyk neozve, zkuste misto 33 hodnotu 21 nebo 5.
#define TOUCH_CS 33
