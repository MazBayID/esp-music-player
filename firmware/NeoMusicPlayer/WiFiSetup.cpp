#include "WiFiSetup.h"
#include "Config.h"
#include "DisplayUI.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_wifi.h>

static void printWiFiState(const char* label) {
  Serial.print(label);
  Serial.print(" status=");
  Serial.print((int)WiFi.status());
  Serial.print(" mode=");
  Serial.print((int)WiFi.getMode());
  Serial.print(" ssid=");
  Serial.println(WiFi.SSID());
}

void setupWiFi() {
  showStatus("Wi-Fi init", "Starting radio...");
  Serial.println("Wi-Fi: initialize in station mode");
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  delay(150);

  // ESP-IDF power units are quarter-dBm: 32 means a requested 8 dBm.
  // Apply this before any connection attempt to accommodate the weak clone RF.
  esp_err_t powerResult = esp_wifi_set_max_tx_power(32);
  Serial.print("Wi-Fi: set TX power 8 dBm, result=");
  Serial.println((int)powerResult);
  showStatus("RF power: 8 dBm", powerResult == ESP_OK ? "Power limit set" : "Power API failed");

  showStatus("Wi-Fi connecting", "Trying saved network");
  Serial.println("Wi-Fi: trying saved credentials from NVS");
  WiFi.begin();

  const unsigned long connectStarted = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - connectStarted < 15000UL) {
    delay(250);
    if ((millis() - connectStarted) % 3000UL < 250UL) {
      printWiFiState("Wi-Fi connecting:");
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi: connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("Wi-Fi IP: ");
    Serial.println(WiFi.localIP());
    showStatus("Wi-Fi connected", WiFi.localIP().toString());
    return;
  }

  const int failedStatus = (int)WiFi.status();
  Serial.print("Wi-Fi: saved network failed, status=");
  Serial.println(failedStatus);
  showStatus("Wi-Fi failed", "Status " + String(failedStatus));
  delay(1200);

  showStatus("Starting hotspot", "RF power 8 dBm");
  Serial.println("Wi-Fi: switching to AP+STA setup portal");

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(true);
  wifiManager.setConnectTimeout(20);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setAPCallback([](WiFiManager *manager) {
    (void)manager;
    Serial.println("Wi-Fi: setup access point callback fired");
    Serial.print("Wi-Fi AP IP: ");
    Serial.println(WiFi.softAPIP());
    showStatus("Hotspot active", SETUP_AP_NAME);
    delay(300);
    showStatus("Connect phone to", "Neo-Music-Setup");
  });

  WiFi.mode(WIFI_AP_STA);
  delay(300);
  printWiFiState("Wi-Fi before portal:");

  const bool connected = wifiManager.startConfigPortal(SETUP_AP_NAME, SETUP_AP_PASSWORD);
  if (!connected) {
    const int portalStatus = (int)WiFi.status();
    Serial.print("Wi-Fi: portal returned false, status=");
    Serial.println(portalStatus);
    Serial.print("Wi-Fi AP IP at exit: ");
    Serial.println(WiFi.softAPIP());
    showStatus("Portal ended", "Status " + String(portalStatus));
    delay(1500);
    showStatus("Restarting...", "Retry Wi-Fi setup");
    delay(1000);
    ESP.restart();
    return;
  }

  Serial.print("Wi-Fi: portal connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("Wi-Fi IP: ");
  Serial.println(WiFi.localIP());
  showStatus("Wi-Fi connected", WiFi.localIP().toString());
}

void maintainWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  showStatus("Wi-Fi lost", "Reconnecting...");
  Serial.print("Wi-Fi: lost connection, status=");
  Serial.println((int)WiFi.status());
  WiFi.reconnect();
  delay(500);
}
