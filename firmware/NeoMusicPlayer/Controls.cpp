#include "Controls.h"
#include "Config.h"
#include "MusicPlayer.h"
#include "DisplayUI.h"
#include <Arduino.h>
#include <WiFi.h>

struct ButtonState {
  bool down = false;
  bool longHandled = false;
  unsigned long pressedAt = 0;
};

static ButtonState prevState;
static ButtonState nextState;
static ButtonState playState;
static unsigned long lastActionAt = 0;
static bool menuOpen = false;
static int menuSelection = 0;
static constexpr int MENU_ITEMS = 5;

void initControls() {
  pinMode(PIN_PREV, INPUT);
  pinMode(PIN_NEXT, INPUT);
  pinMode(PIN_PLAY, INPUT);
}

static void renderMenu() {
  showMenu(menuSelection, repeatAllEnabled(), shuffleEnabled());
}

static void selectMenuItem() {
  switch (menuSelection) {
    case 0:
      menuOpen = false;
      showStatus("Menu closed", "Playback continues");
      break;
    case 1:
      toggleRepeatAll();
      renderMenu();
      break;
    case 2:
      toggleShuffle();
      renderMenu();
      break;
    case 3:
      if (loadPlaylist()) showStatus("Playlist updated", "New list is ready");
      else showStatus("Reload failed", "Check Wi-Fi/server");
      break;
    case 4:
      if (WiFi.status() == WL_CONNECTED)
        showStatus("Wi-Fi connected", WiFi.localIP().toString());
      else
        showStatus("Wi-Fi offline", "Reconnect pending");
      break;
  }
}

static void dispatchShortPress(int action) {
  const unsigned long now = millis();
  if (now - lastActionAt < BUTTON_DEBOUNCE_MS) return;
  lastActionAt = now;

  if (menuOpen) {
    if (action == 0) {
      menuSelection = (menuSelection + MENU_ITEMS - 1) % MENU_ITEMS;
      renderMenu();
    } else if (action == 1) {
      menuSelection = (menuSelection + 1) % MENU_ITEMS;
      renderMenu();
    } else {
      selectMenuItem();
    }
    return;
  }

  if (action == 0) skipTrack(-1);
  else if (action == 1) skipTrack(1);
  else togglePlayPause();
}

static void processButton(int pin, ButtonState& state, int action, bool supportsLongPress) {
  const bool pressed = digitalRead(pin) == HIGH;
  const unsigned long now = millis();

  if (pressed && !state.down) {
    state.down = true;
    state.longHandled = false;
    state.pressedAt = now;
    return;
  }

  if (pressed && state.down && supportsLongPress && !state.longHandled &&
      now - state.pressedAt >= BUTTON_LONG_PRESS_MS) {
    state.longHandled = true;
    lastActionAt = now;
    if (menuOpen) {
      menuOpen = false;
      showStatus("Menu closed", "Playback continues");
    } else {
      menuOpen = true;
      menuSelection = 0;
      renderMenu();
    }
    return;
  }

  if (!pressed && state.down) {
    const unsigned long duration = now - state.pressedAt;
    const bool wasLongPress = state.longHandled;
    state.down = false;
    state.longHandled = false;
    if (!wasLongPress && duration >= BUTTON_MIN_PRESS_MS)
      dispatchShortPress(action);
  }
}

void handleControls() {
  processButton(PIN_PREV, prevState, 0, false);
  processButton(PIN_NEXT, nextState, 1, false);
  processButton(PIN_PLAY, playState, 2, true);
}
