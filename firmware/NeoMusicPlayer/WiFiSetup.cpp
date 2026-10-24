#include "WiFiSetup.h"
#include "Config.h"
#include "DisplayUI.h"
#include <WiFi.h>
#include <WiFiManager.h>

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  // First try credentials saved in ESP32 NVS. If they are missing or fail,
  // automatically open the setup hotspot without requiring a button press.
  showStatus("Connecting Wi-Fi", "Checking saved network...");
  Serial.println("Wi-Fi: trying saved credentials");
  WiFi.begin();

  const unsigned long connectStarted = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - connectStarted < 15000UL) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi: connected to ");
    Serial.println(WiFi.SSID());
    return;
  }

  Serial.println("Wi-Fi: saved network unavailable; opening setup portal");
  showStatus("Wi-Fi unavailable", "Starting hotspot...");

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(true);
  wifiManager.setConnectTimeout(20);
  wifiManager.setConfigPortalTimeout(180);

  // AP+STA allows the phone to configure Wi-Fi while the ESP32 scans/connects.
  WiFi.mode(WIFI_AP_STA);
  delay(250);

  if (!wifiManager.startConfigPortal(SETUP_AP_NAME, SETUP_AP_PASSWORD)) {
    showStatus("Wi-Fi setup timeout", "Restart to retry");
    Serial.println("Wi-Fi: setup portal timed out; restarting");
    delay(1500);
    ESP.restart();
    return;
  }

  Serial.print("Wi-Fi: portal finished, connected to ");
  Serial.println(WiFi.SSID());
}

void maintainWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  showStatus("Wi-Fi lost", "Reconnecting...");
  WiFi.reconnect();
  delay(500);
}
