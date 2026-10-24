#include "Controls.h"
#include "Config.h"
#include "MusicPlayer.h"
#include <Arduino.h>

static unsigned long lastButtonTime = 0;

void initControls() {
  pinMode(PIN_PREV, INPUT);
  pinMode(PIN_NEXT, INPUT);
  pinMode(PIN_PLAY, INPUT);
}

static bool buttonPressed(int pin) {
  if (digitalRead(pin) != HIGH) return false;
  unsigned long now = millis();
  if (now - lastButtonTime < BUTTON_DEBOUNCE_MS) return false;
  lastButtonTime = now;
  return true;
}

void handleControls() {
  if (buttonPressed(PIN_PREV)) skipTrack(-1);
  else if (buttonPressed(PIN_NEXT)) skipTrack(1);
  else if (buttonPressed(PIN_PLAY)) togglePlayPause();
}