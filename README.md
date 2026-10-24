# ESP32-C3 SuperMini Music Player

Firmware for streaming MP3 tracks from a Python HTTP server running in Termux on Android. Audio output uses a MAX98357A I2S amplifier, controls use three TTP223 touch modules, and status is shown on an SSD1306 I2C OLED (0.96-inch, 128×64).

## Phone-only firmware build

You do **not** need a PC, ArduinoDroid, or local Arduino library installation. GitHub Actions compiles the modular source and publishes firmware `.bin` files.

1. Open the repository's **Actions** tab: https://github.com/MazBayID/esp-music-player/actions
2. Select **Build ESP32-C3 firmware** and open the newest successful run.
3. In **Artifacts**, download `neo-music-player-esp32-c3-firmware`.
4. Extract the ZIP on Android. Use an ESP32 flashing app/tool that supports ESP32-C3 and the generated binary layout. If the artifact contains multiple BIN files, do not flash an arbitrary one as though it were the complete image; follow the firmware flashing guide added alongside the build output.

The modular sketch used by the workflow is in `firmware/NeoMusicPlayer/`. It is split into configuration, OLED, Wi-Fi setup, music streaming, and touch-control modules. The older `firmware/esp32c3_music_player.ino` is retained as a legacy reference; it is **not** the workflow's build input.

## Hardware pin mapping

| Module | Signal | ESP32-C3 SuperMini |
|---|---|---:|
| MAX98357A | BCLK | GPIO 1 |
| MAX98357A | LRC / WS | GPIO 2 |
| MAX98357A | DIN | GPIO 5 |
| TTP223 | Previous | GPIO 4 |
| TTP223 | Next | GPIO 6 |
| TTP223 | Play / stop | GPIO 7 |
| OLED SSD1306 | SCL | GPIO 20 |
| OLED SSD1306 | SDA | GPIO 21 |

Connect all module grounds together. Power the MAX98357A and speaker according to the breakout board specifications. Never connect a speaker directly to ESP32 GPIO pins. Check the OLED module's power requirements before wiring it.

## Wi-Fi setup

On first boot, the ESP32 creates a setup access point if no saved Wi-Fi network is available:
- SSID: `Neo-Music-Setup`
- Password: `neo-music`
- Setup page: http://192.168.4.1

Connect your Android phone to the setup access point, open the setup page, select your normal Wi-Fi network, and save. Your phone and ESP32 must be able to communicate on the same local network.

To change the saved Wi-Fi later, hold the Play touch sensor (GPIO 7) while resetting or powering on the ESP32, then release it when the OLED displays the setup status. This clears saved Wi-Fi credentials and reopens the setup portal.

## Termux music server

Keep MP3 files in the `out` folder beside `server.py`, then run:

```sh
python server.py
```

The server should provide `http://PHONE_IP:8080/playlist.txt` and direct URLs to MP3 files. Update `PLAYLIST_URL` in `firmware/NeoMusicPlayer/Config.h` to use your Android phone's actual LAN IP address, then commit the change so GitHub Actions builds the updated firmware. Do not use `127.0.0.1`; the ESP32 needs the phone's LAN address. If the phone IP changes, update and rebuild.

Some hotspots isolate clients or block access to the phone itself. If the ESP32 cannot fetch the playlist, check that both devices can reach each other on the local network.

## Controls

- Previous (GPIO 4): play the previous track.
- Next (GPIO 6): play the next track.
- Play (GPIO 7): stop the current stream; press again to restart that track from the beginning.

The current Play behavior is stop/restart, not true pause/resume. The firmware decodes MP3 streams; WAV, FLAC, AAC, and other formats are not guaranteed to work.

## Build status

GitHub Actions is configured to compile the modular sketch for the ESP32-C3 and upload the generated BIN files as a 30-day artifact. A successful CI build confirms compilation only; actual audio, OLED, Wi-Fi, and touch behavior still require testing on the physical hardware.
