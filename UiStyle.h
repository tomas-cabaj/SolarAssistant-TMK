#pragma once

// Spolecna geometrie karet vychazi z prehledu. Barvy patri hodnotam a akcim;
// bezne obrysy zustavaji neutralni. Bez dynamicke alokace.
// EN: Shared card geometry follows the overview. Accent colours belong to
// values and actions; ordinary borders stay neutral. No dynamic allocation.
constexpr int UI_CARD_RADIUS = 8;
inline void uiPanel(int x, int y, int w, int h, uint16_t border = C_LINE) {
  tft.fillRoundRect(x, y, w, h, UI_CARD_RADIUS, C_CARD);
  tft.drawRoundRect(x, y, w, h, UI_CARD_RADIUS, border);
}

inline void uiStat(int x, int y, int w, int h, const char* label,
                   const char* value, uint16_t colour, bool small) {
  uiPanel(x, y, w, h);
  // Nadpis je sedy a vlevo, hodnota zustava vycentrovana v bloku.
  // EN: The label is grey and left-aligned; the value stays centred in the block.
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_DIM, C_CARD);
  tCz(label, x + 8, y + 6);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(colour, C_CARD);
  if (small) tCz(value, x + w / 2, y + h - 14);
  else tAs(value, x + w / 2, y + h - 15, 4);
  tft.setTextDatum(TL_DATUM);
}
