#pragma once

// Copy this file to secrets.h and fill in the real values.
// secrets.h is intentionally ignored by Git.

// Wi-Fi
const char* ssid     = "your_wifi_ssid";
const char* password = "your_wifi_password";

// Solar Assistant API
#define API_HOST "192.168.10.240"
const char* url  = "http://" API_HOST "/api/v1/metrics";
const char* user = "admin";
const char* pass = "your_solar_assistant_password";

// OTA
#define OTA_HOST     "esp32-solar-lcd"
#define OTA_PASSWORD "your_ota_password"
