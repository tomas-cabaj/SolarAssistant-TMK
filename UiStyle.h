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
  else {
    // Vestavene pismo neumi znak č; u castek proto vykresli cislo velce
    // a jednotku Kč smooth fontem, aby se jednotka neztratila.
    // EN: The built-in font lacks č; draw the number large and Kč in the
    // smooth font so the unit remains visible.
    const char* unit = strstr(value, " Kč");
    if (unit && strcmp(unit, " Kč") == 0) {
      char number[24];
      size_t len = (size_t)(unit - value);
      if (len >= sizeof(number)) len = sizeof(number) - 1;
      memcpy(number, value, len);
      number[len] = '\0';
      czOff();
      int nw = tft.textWidth(number, 4);
      czOn();
      int uw = tft.textWidth("Kč");
      int total = nw + 4 + uw;
      tAs(number, x + w / 2 - total / 2 + nw / 2, y + h - 15, 4);
      tCz("Kč", x + w / 2 + total / 2 - uw / 2, y + h - 15);
    } else {
      tAs(value, x + w / 2, y + h - 15, 4);
    }
  }
  tft.setTextDatum(TL_DATUM);
}
