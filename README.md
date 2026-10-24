# ESP32-C3 SuperMini Music Player

Firmware for streaming MP3 tracks from a Python HTTP server running in Termux on an Android phone. Audio output uses a MAX98357A I2S amplifier; track controls use three TTP223 touch modules; status is shown on an SSD1306 I2C OLED.

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

Connect all module grounds together. Power the MAX98357A and speaker according to the breakout board's specifications. Do not connect a speaker directly to ESP32 GPIO pins. Verify the OLED's voltage requirements and I2C address before powering it.

## Firmware

Open `firmware/esp32c3_music_player.ino` in Arduino IDE.

Install these libraries through Library Manager:

- **ESP8266Audio** by Earle F. Philhower
- **Adafruit GFX Library**
- **Adafruit SSD1306**
- ESP32 board support by Espressif Systems

Choose an ESP32-C3 board profile (for example, **ESP32C3 Dev Module**) and select the correct serial port.

Edit these values near the top of the sketch before flashing:

- `WIFI_SSID`: the Wi-Fi network the phone and ESP32 will both use.
- `WIFI_PASSWORD`: that network's password.
- `PLAYLIST_URL`: replace `192.168.1.23` with the Android phone's actual LAN IP address.

The OLED configuration assumes a 128×64 SSD1306 at I2C address `0x3C`. If your display differs, adjust the constants in the sketch.

## Termux music server

Keep the music files in the `out` folder beside `server.py`. Start the Python server in Termux:

```sh
python server.py
```

The server should listen on port 8080 and provide:

- `http://PHONE_IP:8080/playlist.txt`
- `http://PHONE_IP:8080/your-track.mp3`

Find the phone's IP address on the same Wi-Fi network and set `PLAYLIST_URL` accordingly. The phone and ESP32 must be on a network that permits devices to communicate with each other. If using a phone hotspot, check that the hotspot allows connected clients to reach the phone itself. Do not use `127.0.0.1` in the ESP32 firmware; that would refer to the ESP32, not the phone.

**Current firmware decodes MP3 streams.** Put MP3 files in `out` and ensure the playlist endpoint returns URLs for those files. Other formats such as WAV, FLAC, or AAC are not guaranteed to play with this sketch.

## Controls

- **Previous (GPIO 4):** play the previous track.
- **Next (GPIO 6):** play the next track.
- **Play (GPIO 7):** start the current track if stopped; while playing, this stops the stream. Starting it again begins the selected track from the beginning.

TTP223 modules are expected to output HIGH when touched.

## Troubleshooting

- **Playlist failed:** open `http://PHONE_IP:8080/playlist.txt` from another device on the same network and verify the URL and port.
- **Wi-Fi failed:** check SSID/password and ensure both devices use the same reachable network.
- **OLED blank:** check power, SDA/SCL wiring, display type, and I2C address (`0x3C` is common; some modules use `0x3D`).
- **No sound:** check speaker wiring and amplifier power, confirm BCLK/LRC/DIN pin mapping, and start with a modest gain.
- **Build errors:** confirm the listed libraries and ESP32 board package are installed.

This firmware has been committed to the repository but has not been compiled or tested on physical hardware in this environment.