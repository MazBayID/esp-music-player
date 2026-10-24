#include "WiFiSetup.h"
#include "Config.h"
#include "DisplayUI.h"
#include <WiFi.h>
#include <WiFiManager.h>

static bool playHeldDuringBoot() {
  // TTP223 modules can take a short moment to stabilize after power-up.
  // Sample over a window instead of reading the pin only once.
  showStatus("Wi-Fi setup", "Hold Play to reset");
  const unsigned long started = millis();
  while (millis() - started < 1800) {
    if (digitalRead(PIN_PLAY) == HIGH) {
      Serial.println("Wi-Fi setup: Play touch detected at boot");
      return true;
    }
    delay(20);
  }
  Serial.println("Wi-Fi setup: no Play touch detected at boot");
  return false;
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(true);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setConnectTimeout(25);

  const bool forceSetupPortal = playHeldDuringBoot();
  if (forceSetupPortal) {
    showStatus("Reset Wi-Fi", "Opening setup portal");
    wifiManager.resetSettings();
    delay(300);

    // Explicitly enable AP + station mode before starting the captive portal.
    WiFi.mode(WIFI_AP_STA);
    showStatus("Wi-Fi setup", "Starting hotspot...");
    Serial.printf("Wi-Fi setup: starting AP '%s'\n", SETUP_AP_NAME);
    if (!wifiManager.startConfigPortal(SETUP_AP_NAME, SETUP_AP_PASSWORD)) {
      showStatus("Wi-Fi setup timeout", "Restart to retry");
      delay(1500);
      ESP.restart();
      return;
    }
  } else {
    showStatus("Wi-Fi setup", "Please wait...");
    Serial.println("Wi-Fi setup: trying saved credentials");
    if (!wifiManager.autoConnect(SETUP_AP_NAME, SETUP_AP_PASSWORD)) {
      showStatus("Wi-Fi setup timeout", "Restart to retry");
      delay(1500);
      ESP.restart();
      return;
    }
  }
}

void maintainWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  showStatus("Wi-Fi lost", "Reconnecting...");
  WiFi.reconnect();
  delay(500);
}
