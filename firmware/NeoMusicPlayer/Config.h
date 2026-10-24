#pragma once
#include <Arduino.h>

constexpr int PIN_I2S_BCLK = 1;
constexpr int PIN_I2S_LRC = 2;
constexpr int PIN_I2S_DIN = 5;
constexpr int PIN_PREV = 4;
constexpr int PIN_NEXT = 6;
constexpr int PIN_PLAY = 7;
constexpr int PIN_OLED_SCL = 20;
constexpr int PIN_OLED_SDA = 21;

constexpr int OLED_WIDTH = 128;
constexpr int OLED_HEIGHT = 64;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 220;
constexpr size_t MAX_TRACKS = 100;

static const char PLAYLIST_URL[] = "http://192.168.1.23:8080/playlist.txt";
static const char SETUP_AP_NAME[] = "Neo-Music-Setup";
static const char SETUP_AP_PASSWORD[] = "neo-music";