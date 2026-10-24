#pragma once

void initPlayerSettings();
bool loadPlaylist();
bool startTrack(int index);
void skipTrack(int direction);
void togglePlayPause();
void toggleRepeatAll();
void toggleShuffle();
bool repeatAllEnabled();
bool shuffleEnabled();
void playerLoop();
