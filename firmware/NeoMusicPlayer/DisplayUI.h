#pragma once
#include <Arduino.h>

void initDisplay();
void showStatus(const String& title, const String& line2 = "");
void showMenu(int selectedItem, bool repeatAll, bool shuffleEnabled);
