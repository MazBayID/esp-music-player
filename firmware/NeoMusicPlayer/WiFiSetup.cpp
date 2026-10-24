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

  // Holding Play at boot explicitly opens the setup AP, rather than relying
  // on autoConnect() to decide whether saved credentials should be reused.
  const bool forceSetupPortal = (digitalRead(PIN_PLAY) == HIGH);
  if (forceSetupPortal) {
    showStatus("Reset Wi-Fi", "Opening setup portal");
    wifiManager.resetSettings();
    delay(300);
    showStatus("Wi-Fi setup", "Starting hotspot...");
    if (!wifiManager.startConfigPortal(SETUP_AP_NAME, SETUP_AP_PASSWORD)) {
      showStatus("Wi-Fi setup timeout", "Restart to retry");
      delay(1500);
      ESP.restart();
      return;
    }
  } else {
    showStatus("Wi-Fi setup", "Please wait...");
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
