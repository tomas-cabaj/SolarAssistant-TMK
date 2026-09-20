#pragma once

// Stejne trideni jako u textu pocasi; ikona a popis se tak nemohou rozejit.
// Veskera geometrie zustava v prostoru 64 x 64 bodu.
// EN: Same classification as the weather captions, keeping icon and text
// aligned. All geometry stays inside a 64 x 64 area.
enum WeatherKind { WX_CLEAR, WX_PARTLY, WX_CLOUD, WX_FOG, WX_DRIZZLE,
                   WX_RAIN, WX_SNOW, WX_SHOWERS, WX_SNOWSH, WX_STORM };
constexpr int dayPercent(int minutes) {
  return minutes < 0 ? -1 : minutes >= 1440 ? 100 : (minutes * 100 + 720) / 1440;
}
static_assert(dayPercent(-1) == -1 && dayPercent(0) == 0, "Unknown and polar night");
static_assert(dayPercent(720) == 50 && dayPercent(1440) == 100, "Equinox and polar day");
static_assert(dayPercent(756) == 53, "Day/night rounding");
constexpr WeatherKind weatherKind(int code) {
  return code == 0 ? WX_CLEAR : code <= 2 ? WX_PARTLY : code == 3 ? WX_CLOUD :
         code <= 48 ? WX_FOG : code <= 57 ? WX_DRIZZLE : code <= 67 ? WX_RAIN :
         code <= 77 ? WX_SNOW : code <= 82 ? WX_SHOWERS : code <= 86 ? WX_SNOWSH : WX_STORM;
}
static_assert(weatherKind(0) == WX_CLEAR && weatherKind(3) == WX_CLOUD, "Weather clear/cloud");
static_assert(weatherKind(45) == WX_FOG && weatherKind(61) == WX_RAIN, "Weather fog/rain");
static_assert(weatherKind(71) == WX_SNOW && weatherKind(95) == WX_STORM, "Weather snow/storm");

inline void weatherSunMoon(int x, int y, bool night) {
  if (night) {
    tft.fillCircle(x + 20, y + 19, 13, C_WEATH);
    tft.fillCircle(x + 26, y + 14, 12, C_CARD);
    tft.drawFastHLine(x + 42, y + 10, 7, C_TXT);
    tft.drawFastVLine(x + 45, y + 7, 7, C_TXT);
  } else {
    tft.fillCircle(x + 23, y + 23, 11, C_PV);
    // Predpocitane paprsky: pri prekresleni neni potreba trigonometrie.
    // EN: Precomputed rays avoid trigonometry during redraw.
    const int8_t ray[8][4] = {{0,-15,0,-20},{11,-11,15,-15},{15,0,20,0},{11,11,15,15},
                             {0,15,0,20},{-11,11,-15,15},{-15,0,-20,0},{-11,-11,-15,-15}};
    for (const auto& r : ray) tft.drawLine(x+23+r[0], y+23+r[1], x+23+r[2], y+23+r[3], C_PV);
  }
}

inline void weatherIcon(int x, int y, int code, bool night) {
  WeatherKind kind = weatherKind(code);
  if (kind == WX_CLEAR || kind == WX_PARTLY || kind == WX_SHOWERS) weatherSunMoon(x, y, night);
  if (kind == WX_CLEAR) return;
  uint16_t cloud = kind == WX_STORM ? C_DIM : C_TXT;
  tft.fillCircle(x + 18, y + 33, 11, cloud);
  tft.fillCircle(x + 32, y + 26, 15, cloud);
  tft.fillCircle(x + 46, y + 34, 10, cloud);
  tft.fillRoundRect(x + 14, y + 33, 37, 12, 5, cloud);
  if (kind == WX_FOG) {
    for (int row = 0; row < 3; ++row) tft.drawFastHLine(x + 8 + row * 3, y + 49 + row * 5, 44 - row * 5, C_DIM);
  } else if (kind == WX_SNOW || kind == WX_SNOWSH) {
    for (int i = 0; i < 3; ++i) {
      int cx = x + 16 + i * 15, cy = y + 53 + (i % 2) * 4;
      tft.drawFastHLine(cx - 3, cy, 7, C_WEATH);
      tft.drawFastVLine(cx, cy - 3, 7, C_WEATH);
      tft.drawLine(cx - 2, cy - 2, cx + 2, cy + 2, C_WEATH);
    }
  } else if (kind == WX_STORM) {
    tft.fillTriangle(x + 32, y + 43, x + 21, y + 55, x + 33, y + 53, C_PV);
    tft.fillTriangle(x + 30, y + 50, x + 40, y + 48, x + 25, y + 63, C_PV);
  } else if (kind == WX_DRIZZLE || kind == WX_RAIN || kind == WX_SHOWERS) {
    int length = kind == WX_DRIZZLE ? 3 : 8;
    for (int i = 0; i < 3; ++i) {
      int dx = x + 18 + i * 14;
      tft.drawLine(dx, y + 49, dx - 3, y + 49 + length, C_WEATH);
      tft.drawLine(dx + 1, y + 49, dx - 2, y + 49 + length, C_WEATH);
    }
  }
}
