#include "MusicPlayer.h"
#include "Config.h"
#include "DisplayUI.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
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
static bool repeatAll = true;
static bool shuffleEnabledValue = false;
static int shuffleOrder[MAX_TRACKS];
static int shufflePosition = 0;

static String trackNameFromUrl(const String& url) {
  int slash = url.lastIndexOf('/');
  String name = (slash >= 0) ? url.substring(slash + 1) : url;
  name.replace("%20", " ");
  if (name.length() > 20) name = name.substring(0, 20);
  return name;
}

static void persistSettings() {
  Preferences preferences;
  if (!preferences.begin("neo-player", false)) return;
  preferences.putBool("repeat", repeatAll);
  preferences.putBool("shuffle", shuffleEnabledValue);
  preferences.end();
}

static void buildShuffleOrder(bool putCurrentFirst) {
  for (int i = 0; i < playlistCount; ++i) shuffleOrder[i] = i;
  for (int i = playlistCount - 1; i > 0; --i) {
    int j = random(i + 1);
    int temp = shuffleOrder[i];
    shuffleOrder[i] = shuffleOrder[j];
    shuffleOrder[j] = temp;
  }

  if (putCurrentFirst && playlistCount > 0) {
    for (int i = 0; i < playlistCount; ++i) {
      if (shuffleOrder[i] == currentTrack) {
        int temp = shuffleOrder[0];
        shuffleOrder[0] = shuffleOrder[i];
        shuffleOrder[i] = temp;
        break;
      }
    }
  }
  shufflePosition = 0;
}

static void syncShufflePosition() {
  for (int i = 0; i < playlistCount; ++i) {
    if (shuffleOrder[i] == currentTrack) {
      shufflePosition = i;
      return;
    }
  }
}

void initPlayerSettings() {
  Preferences preferences;
  if (preferences.begin("neo-player", true)) {
    repeatAll = preferences.getBool("repeat", true);
    shuffleEnabledValue = preferences.getBool("shuffle", false);
    preferences.end();
  }
  randomSeed(micros());
}

bool repeatAllEnabled() {
  return repeatAll;
}

bool shuffleEnabled() {
  return shuffleEnabledValue;
}

void toggleRepeatAll() {
  repeatAll = !repeatAll;
  persistSettings();
  Serial.print("Playback: repeat all ");
  Serial.println(repeatAll ? "enabled" : "disabled");
}

void toggleShuffle() {
  shuffleEnabledValue = !shuffleEnabledValue;
  if (shuffleEnabledValue) buildShuffleOrder(true);
  persistSettings();
  Serial.print("Playback: shuffle ");
  Serial.println(shuffleEnabledValue ? "enabled" : "disabled");
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
  buildShuffleOrder(true);
  showStatus("Playlist loaded", String(playlistCount) + " tracks");
  return true;
}

bool startTrack(int index) {
  if (playlistCount == 0 && !loadPlaylist()) return false;
  if (index < 0) index = playlistCount - 1;
  if (index >= playlistCount) index = 0;
  currentTrack = index;
  if (shuffleEnabledValue) syncShufflePosition();
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

void skipTrack(int direction) {
  if (playlistCount == 0 && !loadPlaylist()) return;

  if (shuffleEnabledValue) {
    int targetPosition = shufflePosition + direction;
    if (targetPosition < 0 || targetPosition >= playlistCount) {
      if (!repeatAll) return;
      if (direction < 0) {
        targetPosition = playlistCount - 1;
      } else {
        buildShuffleOrder(false);
        if (playlistCount > 1 && shuffleOrder[0] == currentTrack) {
          int temp = shuffleOrder[0];
          shuffleOrder[0] = shuffleOrder[1];
          shuffleOrder[1] = temp;
        }
        targetPosition = 0;
      }
    }
    startTrack(shuffleOrder[targetPosition]);
    return;
  }

  startTrack(currentTrack + direction);
}

void togglePlayPause() {
  // The current audio library does not expose safe seek/resume here:
  // this control stops the stream, then starts the current track from its beginning.
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

    if (playlistCount <= 0) {
      stopPlayback();
      showStatus("Playback ended", "Playlist unavailable");
      return;
    }

    if (shuffleEnabledValue) {
      if (shufflePosition + 1 < playlistCount) {
        startTrack(shuffleOrder[shufflePosition + 1]);
      } else if (repeatAll) {
        buildShuffleOrder(false);
        if (playlistCount > 1 && shuffleOrder[0] == currentTrack) {
          int temp = shuffleOrder[0];
          shuffleOrder[0] = shuffleOrder[1];
          shuffleOrder[1] = temp;
        }
        startTrack(shuffleOrder[0]);
      } else {
        stopPlayback();
        showStatus("PLAYLIST END", "Touch PLAY to restart");
      }
    } else if (currentTrack + 1 < playlistCount) {
      startTrack(currentTrack + 1);
    } else if (repeatAll) {
      startTrack(0);
    } else {
      stopPlayback();
      showStatus("PLAYLIST END", "Touch PLAY to restart");
    }
  }
}
