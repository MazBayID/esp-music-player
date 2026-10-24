#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "Config.h"
#include "DisplayUI.h"
#include "WiFiSetup.h"
#include "MusicPlayer.h"
#include "Controls.h"

void setup() {
  Serial.begin(115200);
  delay(250);
  initControls();
  initDisplay();
  showStatus("Starting...", "ESP32-C3");
  setupWiFi();
  // ESP-IDF uses quarter-dBm units: 40 = 10 dBm requested.
  esp_wifi_set_max_tx_power(40);
  Serial.print("Connected Wi-Fi: ");
  Serial.println(WiFi.SSID());
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
  showStatus("Wi-Fi connected", WiFi.localIP().toString());
  delay(500);
  if (loadPlaylist()) startTrack(0);
}

void loop() {
  maintainWiFi();
  playerLoop();
  handleControls();
  delay(1);
}