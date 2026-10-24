#include <Arduino.h>
#include <WiFi.h>
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

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected Wi-Fi: ");
    Serial.println(WiFi.SSID());
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());
    showStatus("Wi-Fi connected", WiFi.localIP().toString());
  } else {
    Serial.print("Wi-Fi not connected; status=");
    Serial.println((int)WiFi.status());
    showStatus("Wi-Fi not connected", "Check setup portal");
  }

  delay(700);
  showStatus("Loading playlist", "Checking server...");
  if (loadPlaylist()) {
    showStatus("Playlist ready", "Starting playback");
    startTrack(0);
  } else {
    showStatus("Playlist failed", "Check phone/server");
    Serial.println("Playlist: load failed; check playlist URL and phone server");
  }
}

void loop() {
  maintainWiFi();
  playerLoop();
  handleControls();
  delay(1);
}
