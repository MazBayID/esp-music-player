/*
  ESP32-C3 SuperMini Network Music Player
  Wi-Fi setup: WiFiManager captive portal.
  Hold the Play touch button while powering/resetting to forget saved Wi-Fi
  and open the setup hotspot: Neo-Music-Setup (password: neo-music).

  Libraries (Arduino Library Manager):
  - ESP8266Audio by Earle F. Philhower
  - WiFiManager by tzapu
  - Adafruit GFX Library
  - Adafruit SSD1306

  OLED: SSD1306 128x64, I2C address 0x3C
*/

#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "AudioFileSourceHTTPStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

// Replace with the Android phone's LAN IP address.
// Example: http://192.168.1.23:8080/playlist.txt
const char* PLAYLIST_URL = "http://192.168.1.23:8080/playlist.txt";
const char* SETUP_AP_NAME = "Neo-Music-Setup";
const char* SETUP_AP_PASSWORD = "neo-music";

// Pin mapping
constexpr int PIN_I2S_BCLK = 1;
constexpr int PIN_I2S_LRC  = 2;
constexpr int PIN_I2S_DIN  = 5;
constexpr int PIN_PREV = 4;
constexpr int PIN_NEXT = 6;
constexpr int PIN_PLAY = 7;
constexpr int PIN_OLED_SCL = 20;
constexpr int PIN_OLED_SDA = 21;

// OLED SSD1306 128x64
constexpr int OLED_WIDTH = 128;
constexpr int OLED_HEIGHT = 64;
constexpr uint8_t OLED_ADDRESS = 0x3C;
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

AudioGeneratorMP3* decoder = nullptr;
AudioFileSourceHTTPStream* httpSource = nullptr;
AudioFileSourceBuffer* bufferedSource = nullptr;
AudioOutputI2S* audioOutput = nullptr;

String playlist[100];
int playlistCount = 0;
int currentTrack = 0;
bool isPlaying = false;
bool oledReady = false;
unsigned long lastButtonTime = 0;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 220;

void showStatus(const String& title, const String& line2 = "") {
  Serial.println(title + (line2.length() ? " - " + line2 : ""));
  if (!oledReady) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("NEO MUSIC PLAYER");
  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);
  display.setCursor(0, 17);
  display.println(title);
  display.setCursor(0, 34);
  display.println(line2);
  display.display();
}

String trackNameFromUrl(const String& url) {
  int slash = url.lastIndexOf('/');
  String name = (slash >= 0) ? url.substring(slash + 1) : url;
  name.replace("%20", " ");
  if (name.length() > 20) name = name.substring(0, 20);
  return name;
}

void stopPlayback() {
  if (decoder) {
    if (decoder->isRunning()) decoder->stop();
    delete decoder;
    decoder = nullptr;
  }
  if (bufferedSource) {
    delete bufferedSource;
    bufferedSource = nullptr;
  }
  if (httpSource) {
    delete httpSource;
    httpSource = nullptr;
  }
  if (audioOutput) {
    delete audioOutput;
    audioOutput = nullptr;
  }
  isPlaying = false;
}

bool loadPlaylist() {
  if (WiFi.status() != WL_CONNECTED) {
    showStatus("Wi-Fi offline", "Check network");
    return false;
  }

  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(10000);
  if (!http.begin(PLAYLIST_URL)) {
    showStatus("Playlist error", "Check server URL");
    return false;
  }

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    String message = "HTTP " + String(code);
    http.end();
    showStatus("Playlist failed", message);
    return false;
  }

  playlistCount = 0;
  String body = http.getString();
  http.end();

  int start = 0;
  while (start < body.length() && playlistCount < 100) {
    int end = body.indexOf('\n', start);
    if (end < 0) end = body.length();
    String line = body.substring(start, end);
    line.trim();
    if (line.startsWith("http://") || line.startsWith("https://")) {
      playlist[playlistCount++] = line;
    }
    start = end + 1;
  }

  if (playlistCount == 0) {
    showStatus("Playlist empty", "Check /out folder");
    return false;
  }
  if (currentTrack >= playlistCount) currentTrack = 0;
  showStatus("Playlist loaded", String(playlistCount) + " tracks");
  return true;
}

bool startTrack(int index) {
  if (playlistCount == 0 && !loadPlaylist()) return false;
  if (index < 0) index = playlistCount - 1;
  if (index >= playlistCount) index = 0;
  currentTrack = index;
  stopPlayback();

  showStatus("Connecting...", trackNameFromUrl(playlist[currentTrack]));
  httpSource = new AudioFileSourceHTTPStream(playlist[currentTrack].c_str());
  if (!httpSource) {
    showStatus("Memory error", "HTTP source");
    return false;
  }

  bufferedSource = new AudioFileSourceBuffer(httpSource, 4096);
  audioOutput = new AudioOutputI2S();
  audioOutput->SetPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DIN);
  audioOutput->SetGain(0.65);

  decoder = new AudioGeneratorMP3();
  if (!decoder->begin(bufferedSource, audioOutput)) {
    showStatus("Cannot play", trackNameFromUrl(playlist[currentTrack]));
    stopPlayback();
    return false;
  }
  isPlaying = true;
  showStatus("NOW PLAYING", trackNameFromUrl(playlist[currentTrack]));
  return true;
}

void togglePlayPause() {
  // This first version stops playback; pressing again restarts the track.
  if (isPlaying && decoder) {
    String name = trackNameFromUrl(playlist[currentTrack]);
    stopPlayback();
    showStatus("STOPPED", name);
  } else {
    startTrack(currentTrack);
  }
}

bool buttonPressed(int pin) {
  // TTP223 normally outputs HIGH while touched.
  if (digitalRead(pin) != HIGH) return false;
  unsigned long now = millis();
  if (now - lastButtonTime < BUTTON_DEBOUNCE_MS) return false;
  lastButtonTime = now;
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(250);
  pinMode(PIN_PREV, INPUT);
  pinMode(PIN_NEXT, INPUT);
  pinMode(PIN_PLAY, INPUT);

  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  if (oledReady) {
    display.clearDisplay();
    display.display();
  }
  showStatus("Starting...", "ESP32-C3");

  // Hold Play while powering on/resetting to erase stored Wi-Fi and
  // force the setup portal. Release it after the OLED shows this message.
  bool forceWifiSetup = (digitalRead(PIN_PLAY) == HIGH);
  WiFi.mode(WIFI_STA);
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setConnectTimeout(25);

  if (forceWifiSetup) {
    showStatus("Reset Wi-Fi", "Opening setup portal");
    wifiManager.resetSettings();
    delay(700);
  }

  showStatus("Wi-Fi setup", "Please wait...");
  // If no credentials are saved or connection fails, create a temporary AP.
  // Connect phone to Neo-Music-Setup (password neo-music), then visit
  // http://192.168.4.1 if the captive portal does not open automatically.
  if (!wifiManager.autoConnect(SETUP_AP_NAME, SETUP_AP_PASSWORD)) {
    showStatus("Wi-Fi setup timeout", "Restart to retry");
    delay(1500);
    ESP.restart();
  }

  Serial.print("Connected Wi-Fi: ");
  Serial.println(WiFi.SSID());
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
  showStatus("Wi-Fi connected", WiFi.localIP().toString());
  delay(500);

  if (loadPlaylist()) startTrack(0);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    showStatus("Wi-Fi lost", "Reconnecting...");
    WiFi.reconnect();
    delay(500);
    return;
  }

  if (decoder && decoder->isRunning()) {
    if (!decoder->loop()) {
      decoder->stop();
      isPlaying = false;
      if (playlistCount > 0) startTrack((currentTrack + 1) % playlistCount);
    }
  }

  if (buttonPressed(PIN_PREV)) {
    startTrack(currentTrack - 1);
  } else if (buttonPressed(PIN_NEXT)) {
    startTrack(currentTrack + 1);
  } else if (buttonPressed(PIN_PLAY)) {
    togglePlayPause();
  }
  delay(1);
}
