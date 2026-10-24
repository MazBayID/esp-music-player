#include "DisplayUI.h"
#include "Config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
static bool oledReady = false;

void initDisplay() {
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  if (oledReady) {
    display.setRotation(2);  // Rotate OLED 180 degrees
    display.clearDisplay();
    display.display();
  }
}

void showStatus(const String& title, const String& line2) {
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

void showMenu(int selectedItem, bool repeatAll, bool shuffleEnabled) {
  if (!oledReady) return;

  String items[5] = {
    "Back to player",
    String("Repeat all: ") + (repeatAll ? "ON" : "OFF"),
    String("Shuffle: ") + (shuffleEnabled ? "ON" : "OFF"),
    "Reload playlist",
    "Wi-Fi status"
  };

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("NEO / CONTROL MENU");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  for (int i = 0; i < 5; ++i) {
    const int y = 12 + i * 10;
    if (i == selectedItem) {
      display.fillRect(0, y, OLED_WIDTH, 10, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(3, y + 1);
    display.print(items[i]);
  }

  display.setTextColor(SSD1306_WHITE);
  display.display();
}
