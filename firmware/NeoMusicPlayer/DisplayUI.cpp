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