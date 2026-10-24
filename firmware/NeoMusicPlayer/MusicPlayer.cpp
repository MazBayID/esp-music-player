#include "MusicPlayer.h"
#include "Config.h"
#include "DisplayUI.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "AudioFileSourceHTTPStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

static AudioGeneratorMP3* decoder = nullptr;
static AudioFileSourceHTTPStream* httpSource = nullptr;
static AudioFileSourceBuffer* bufferedSource = nullptr;
static AudioOutputI2S* audioOutput = nullptr;
static String playlist[MAX_TRACKS];
static int playlistCount = 0;
static int currentTrack = 0;
static bool isPlaying = false;

static String trackNameFromUrl(const String& url) {
  int slash = url.lastIndexOf('/');
  String name = (slash >= 0) ? url.substring(slash + 1) : url;
  name.replace("%20", " ");
  if (name.length() > 20) name = name.substring(0, 20);
  return name;
}

static void stopPlayback() {
  if (decoder) {
    if (decoder->isRunning()) decoder->stop();
    delete decoder;
    decoder = nullptr;
  }
  if (bufferedSource) { delete bufferedSource; bufferedSource = nullptr; }
  if (httpSource) { delete httpSource; httpSource = nullptr; }
  if (audioOutput) { delete audioOutput; audioOutput = nullptr; }
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
  while (start < body.length() && playlistCount < (int)MAX_TRACKS) {
    int end = body.indexOf('\n', start);
    if (end < 0) end = body.length();
    String line = body.substring(start, end);
    line.trim();
    if (line.startsWith("http://") || line.startsWith("https://"))
      playlist[playlistCount++] = line;
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
  if (!httpSource) { showStatus("Memory error", "HTTP source"); return false; }
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
  // Current behavior is stop/restart, not true pause/resume.
  if (isPlaying && decoder) {
    String name = trackNameFromUrl(playlist[currentTrack]);
    stopPlayback();
    showStatus("STOPPED", name);
  } else {
    startTrack(currentTrack);
  }
}

void playerLoop() {
  if (!decoder || !decoder->isRunning()) return;
  if (!decoder->loop()) {
    decoder->stop();
    isPlaying = false;
    if (playlistCount > 0) startTrack((currentTrack + 1) % playlistCount);
  }
}