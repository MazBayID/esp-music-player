# ESP32-C3 SuperMini Music Player

Firmware for streaming MP3 tracks from a Python HTTP server running in Termux on Android. Audio output uses a MAX98357A I2S amplifier, controls use three TTP223 touch modules, and status is shown on an SSD1306 I2C OLED (0.96-inch, 128×64).

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

## Install and flash firmware

Open `firmware/esp32c3_music_player.ino` in Arduino IDE. Install these libraries through Library Manager:

- **ESP8266Audio** by Earle F. Philhower
- **WiFiManager** by tzapu
- **Adafruit GFX Library**
- **Adafruit SSD1306**
- ESP32 board support by Espressif Systems

Choose an ESP32-C3 board profile (for example, **ESP32C3 Dev Module**) and select the correct serial port.

The OLED is configured for SSD1306 128×64 at I2C address `0x3C`. Some modules use `0x3D`; change `OLED_ADDRESS` in the sketch if needed.

## Configure Wi-Fi from your phone

You do **not** need to put the Wi-Fi SSID or password into the sketch. WiFiManager saves the chosen network credentials in ESP32 flash.

### First-time setup

1. Power on the ESP32. If it has no saved network or cannot connect, it creates a temporary Wi-Fi access point named **`Neo-Music-Setup`**.
2. Connect your Android phone to that access point. Password: **`neo-music`**.
3. If the setup page does not appear automatically, open **http://192.168.4.1** in the phone browser.
4. Select your intended Wi-Fi network, enter its password, and save.
5. The ESP32 reboots/connects using the saved credentials. Your phone and ESP32 need to be able to communicate on the same local network.

Your phone may warn that the setup hotspot has no internet; stay connected while configuring it.

### Change the saved Wi-Fi later

1. Turn off or reset the ESP32.
2. Touch and **hold the Play TTP223 (GPIO 7)** while powering on or resetting the board.
3. When the OLED shows the Wi-Fi setup message, release the touch button.
4. The firmware erases the saved Wi-Fi credentials and starts the `Neo-Music-Setup` hotspot again. Repeat the setup steps above.

If the portal times out after three minutes, restart the ESP32 and try again. Avoid touching the Play sensor during startup unless you intend to reset Wi-Fi settings.

## Connect to the Termux music server

Keep MP3 files in the `out` folder beside `server.py`, then start the server in Termux:

```sh
python server.py
```

The server listens on port 8080 and should provide:

- `http://PHONE_IP:8080/playlist.txt`
- `http://PHONE_IP:8080/your-track.mp3`

Find the Android phone's current LAN IP address and update `PLAYLIST_URL` near the top of the sketch, for example:

```cpp
const char* PLAYLIST_URL = "http://192.168.1.23:8080/playlist.txt";
```

Replace the example IP with your phone's real address. The URL is the **phone's** address, not `127.0.0.1`. If your phone's IP changes, update this URL and reflash the firmware. A future version could make the server address configurable through the setup page as well.

The phone and ESP32 must be on a network that permits local device-to-device communication. Some hotspot configurations isolate clients or prevent access to the phone itself.

**Audio format:** this firmware decodes MP3 streams. Put MP3 files in `out`; WAV, FLAC, AAC, and other formats are not guaranteed to work.

## Controls

- **Previous (GPIO 4):** play the previous track.
- **Next (GPIO 6):** play the next track.
- **Play (GPIO 7):** start the selected track if stopped; while playing, stop the stream. Press again to restart that track from the beginning.

TTP223 modules are expected to output HIGH when touched. Despite the button label, true pause/resume is not implemented yet.

## Troubleshooting

- **Setup hotspot not visible:** restart the board, wait for startup, and check the serial monitor at 115200 baud.
- **Wi-Fi setup page missing:** connect to `Neo-Music-Setup` and manually open `http://192.168.4.1`.
- **Playlist failed:** verify the phone IP and port; open `http://PHONE_IP:8080/playlist.txt` from another device on the same network.
- **OLED blank:** check power, SDA/SCL wiring, and I2C address (`0x3C` or `0x3D`).
- **No sound:** check speaker/amplifier power and wiring, I2S pins, and MP3 file format.
- **Build errors:** verify all libraries above and the ESP32 board package are installed.

The firmware and documentation have been committed to GitHub, but compilation and physical hardware testing have not yet been performed in this environment.