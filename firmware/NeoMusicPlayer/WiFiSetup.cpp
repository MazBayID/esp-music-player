#include "WiFiSetup.h"
#include "Config.h"
#include "DisplayUI.h"
#include <WiFi.h>
#include <WiFiManager.h>

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setConnectTimeout(25);

  // Hold the Play touch sensor during reset to clear saved credentials.
  if (digitalRead(PIN_PLAY) == HIGH) {
    showStatus("Reset Wi-Fi", "Opening setup portal");
    wifiManager.resetSettings();
    delay(700);
  }

  showStatus("Wi-Fi setup", "Please wait...");
  if (!wifiManager.autoConnect(SETUP_AP_NAME, SETUP_AP_PASSWORD)) {
    showStatus("Wi-Fi setup timeout", "Restart to retry");
    delay(1500);
    ESP.restart();
  }
}

void maintainWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  showStatus("Wi-Fi lost", "Reconnecting...");
  WiFi.reconnect();
  delay(500);
}