# ESP32-HID-Mouse

An ESP32-WROOM-32 acting as a Bluetooth (BLE) HID mouse. Once paired, it
appears as a normal Bluetooth mouse and moves/clicks/scrolls automatically,
while the user's real mouse keeps working alongside it.

## Hardware

- ESP32-WROOM-32 Dev Board, 38-pin, USB cable for flashing/power.
- Pushbutton on **GPIO 22** — pause/resume automation.
- LED on **GPIO 21** — on/off shows automation state, blinks during an OTA
  update.

## Why BLE, not Classic Bluetooth HID

BLE HID has a solid Arduino library (`ESP32 BLE Mouse` by T-vK) and works
natively on Windows/Linux/macOS/Android. Classic Bluetooth HID has no mature
Arduino library for the ESP32 — would mean writing directly against
ESP-IDF's Bluedroid API. Not worth it unless a target device turns out to
lack BLE HID support.

## Project layout

PlatformIO project at [`Code/HID-Mouse`](Code/HID-Mouse).

- `platformio.ini` — two environments:
  - `esp32dev`: USB flashing (`upload_port = COM4`).
  - `esp32dev-ota`: WiFi flashing (`upload_protocol = espota`, IP-based
    `upload_port`, OTA password in `upload_flags`).
- `src/main.cpp` — the whole sketch:
  - BLE mouse (`BleMouse`, advertised as "Logitech MX Master 3"),
    automation loop (`runMouse()`, a repeating square-move + scroll pattern).
  - Pause/resume button on GPIO 22, toggles `automationEnabled`.
  - Status LED on GPIO 21 via `updateLed()`.
  - WiFi + `ArduinoOTA` for wireless updates.
- `include/secrets.h` — gitignored. Holds `WIFI_SSID`, `WIFI_PASSWORD`,
  `OTA_HOSTNAME`, `OTA_PASSWORD` as `#define`s. Required for `main.cpp` to
  compile — create it yourself, it's not in the repo.

## Setup

1. Install [PlatformIO](https://platformio.org/).
2. Open `Code/HID-Mouse` — dependencies pull automatically.
3. Create `include/secrets.h` with the four `#define`s above.
4. First flash must be over USB: `pio run -e esp32dev -t upload`.
5. After that, flash wirelessly: set `upload_port` under `[env:esp32dev-ota]`
   to the board's IP, then `pio run -e esp32dev-ota -t upload`.
6. Serial monitor: `pio device monitor` (115200 baud).

## Pairing

Pair with **"Logitech MX Master 3"** from the host's Bluetooth settings — no
PIN. Automation starts once connected, gated by the GPIO 22 button.

## OTA notes worth remembering

- `ArduinoOTA.handle()` must run in every `loop()` iteration of every future
  version, or OTA breaks and USB is needed to recover.
- BLE and WiFi fighting for the radio broke OTA transfers — fixed by calling
  `bleMouse.end()` in `onStart` (BLE comes back automatically on the reboot
  after a successful update).
- Needed `board_build.partitions = min_spiffs.csv` — the default partition
  table is too small and has no OTA dual-slot support.
- mDNS (started automatically by `ArduinoOTA.begin()`, unused otherwise) was
  crashing the device mid-transfer — fixed by calling `MDNS.end()` right
  after `ArduinoOTA.begin()`.
- Any `Serial`/callback code added to `onProgress` must be cheap — it runs
  inside `ArduinoOTA.handle()`'s blocking transfer loop, not on a normal
  `loop()` tick. This is also why the LED blink logic lives in `onProgress`
  rather than `updateLed()`.

## Known issues

- `platformio.ini`'s `[env:esp32dev-ota]` has the OTA password **hardcoded**
  (`--auth=1234567890`) instead of pulled from `${sysenv.OTA_PASSWORD}`.
  This file isn't gitignored — fix before committing.

## Possible future work

- Driving the mouse from an external program (Python over Serial/WiFi)
  instead of the fixed automation pattern — `runMouse()`'s
  `bleMouse.move()`/`click()` calls are the seam to hook a command parser
  into later.

## Development log

- 2026-08-07 — BLE mouse working (PlatformIO, `ESP32 BLE Mouse` lib), pairs
  and runs the automation pattern.
- 2026-08-09 — GPIO 22 pause/resume button done. OTA added; found and fixed
  the BLE/WiFi coexistence bug, partition size, and a Windows Firewall
  block.
- 2026-08-10 — Found and fixed an mDNS-related crash during OTA transfers.
  Added the GPIO 21 status LED (solid = automation state, blink = OTA in
  progress).
