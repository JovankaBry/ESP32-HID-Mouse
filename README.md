# ESP32-HID-Mouse

An ESP32-WROOM-32 acting as a Bluetooth (BLE) HID mouse. Once paired, it
shows up as a normal Bluetooth mouse and moves/clicks/scrolls on its own,
while your real mouse keeps working alongside it.

## Hardware

- ESP32-WROOM-32 Dev Board, 38-pin.
- Pushbutton on **GPIO 22** — pause/resume the automation.
- LED on **GPIO 21** — shows automation state, blinks during a firmware
  update.

## How it works

- BLE mouse via the `ESP32 BLE Mouse` library, advertised as "Logitech MX
  Master 3". Pair with it like any Bluetooth mouse — no PIN.
- Once connected, it repeats a small square-move + scroll pattern.
- WiFi firmware updates (OTA) via `ArduinoOTA`, so new code can be pushed
  without USB after the first flash.

## Project layout

PlatformIO project at [`Code/HID-Mouse`](Code/HID-Mouse):

- `src/main.cpp` — the whole sketch.
- `platformio.ini` — `esp32dev` env for USB flashing, `esp32dev-ota` for
  WiFi flashing.
- `include/secrets.h` — gitignored, holds WiFi/OTA credentials as
  `#define`s. Create it yourself; not in the repo.

## Setup

1. Install [PlatformIO](https://platformio.org/) and open `Code/HID-Mouse`.
2. Create `include/secrets.h` with `WIFI_SSID`, `WIFI_PASSWORD`,
   `OTA_HOSTNAME`, `OTA_PASSWORD`.
3. First flash over USB: `pio run -e esp32dev -t upload`.
4. Later flashes over WiFi: `pio run -e esp32dev-ota -t upload`.
