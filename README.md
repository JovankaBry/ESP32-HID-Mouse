# ESP32-HID-Mouse

## Goal

Program an ESP32-WROOM-32 Dev Board to act as a **Bluetooth HID mouse**. Once
paired, the ESP32 should appear to the host OS as a standard Bluetooth mouse.
Instead of being moved by hand, it generates mouse movement, clicks, and
scroll events under program control, while the user's own physical mouse
keeps working normally alongside it (both are independent HID input devices
to the OS).

## Requirements

- Bluetooth HID mouse — not USB HID (the ESP32-WROOM-32 has no native USB
  peripheral, so USB HID isn't an option on this board).
- Appears as a normal Bluetooth mouse to the host.
- User's physical mouse remains usable at the same time.
- Automated actions to support: move cursor, left click, right click, scroll.
- Cross-platform: Windows, Linux, macOS, Android (any OS with standard
  Bluetooth HID mouse support).
- Built with Arduino IDE + C++.

## Hardware

- ESP32-WROOM-32 Dev Board, 38-pin variant — has Bluetooth Classic and BLE,
  no native USB.
- USB cable for flashing and power.
- Pushbutton on **GPIO 22** (pause/resume automation — done, see
  [Code structure](#code-structure)).

## Approach: BLE HID, not Bluetooth Classic HID

The ESP32 supports both Bluetooth Classic and BLE, and HID mice exist under
both. Decision: use **BLE HID over GATT**.

- Classic Bluetooth HID (the profile a lot of commercial BT mice use) is not
  well supported in the Arduino ESP32 ecosystem — there's no mature Arduino
  library for it, and building it means writing directly against ESP-IDF's
  Bluedroid HID-device API.
- BLE HID has a solid, actively used Arduino library (`ESP32 BLE Mouse` by
  T-vK), and BLE HID mice are natively supported by Windows 10/11, Linux
  (BlueZ), macOS, and Android — every OS in the requirements — with no
  extra drivers.

If a target device ever turns out to lack BLE HID support, that's the
trigger to revisit Classic HID; not expected for anything listed above.

## Code structure

Implemented in [`Code/HID-Mouse`](Code/HID-Mouse) as a PlatformIO project
(not Arduino IDE — the project moved to PlatformIO during development).

- [`platformio.ini`](Code/HID-Mouse/platformio.ini): two environments.
  `[env:esp32dev]` is the normal USB-serial build (`upload_port = COM4`).
  `[env:esp32dev-ota]` uploads over WiFi instead (`upload_protocol =
  espota`, `upload_port` set to the ESP32's LAN IP, `--auth=` flag with the
  OTA password) — see [OTA firmware updates](#ota-firmware-updates) below.
  Both share `framework = arduino`, `lib_deps = t-vk/ESP32 BLE
  Mouse@^0.3.0`, and `board_build.partitions = min_spiffs.csv`.
- [`src/main.cpp`](Code/HID-Mouse/src/main.cpp): creates a `BleMouse`
  instance advertised as **"Logitech MX Master 3"**; `setup()` connects to
  WiFi, starts BLE advertising (`bleMouse.begin()`), and starts the OTA
  listener; `loop()` runs `runMouse()` once a host is connected
  (`bleMouse.isConnected()`) **and** automation is enabled, which traces a
  small square with the cursor, scrolling up/down between moves, on a
  ~150 ms step pace.
- **Pause/resume button**: a pushbutton on **GPIO 22**
  (`pinMode(BUTTON_PIN, INPUT_PULLUP)`) toggles a global `automationEnabled`
  bool via `handleButton()`/`buttonPressed()` (press-and-200ms-delay
  debounce), gating whether `runMouse()` is called in `loop()`. This
  pauses/resumes the automated actions without dropping the BLE connection
  (`bleMouse.end()` is not used for this — that would fully
  disconnect/unadvertise instead). Confirmed working on hardware: one
  press toggles automation on/off reliably with no dropped connection.
- Confirmed working on real hardware: flashes and boots cleanly, serial log
  prints correctly at 115200 baud (`monitor_speed = 115200` needed in
  `platformio.ini` — PlatformIO's monitor defaults to 9600), and the board
  pairs as a standard BLE mouse.

## OTA firmware updates

WiFi-based over-the-air updates were added using the `ArduinoOTA` library,
so new firmware can be pushed without a USB cable once the board is
deployed somewhere inconvenient to reach.

- `main.cpp` includes `WiFi.h` (via `<wifi.h>`), `ESPmDNS.h`, `WiFiUdp.h`,
  `ArduinoOTA.h`, and a local `"secrets.h"`.
- [`include/secrets.h`](Code/HID-Mouse/include/secrets.h) holds
  `WIFI_SSID`, `WIFI_PASSWORD`, `OTA_HOSTNAME`, and `OTA_PASSWORD` as
  `#define`s. It's gitignored deliberately — it contains real network and
  OTA credentials and must never be committed.
- `setup()` connects to WiFi and blocks until connected, then calls
  `ArduinoOTA.setHostname()` / `setPassword()`, registers
  `onStart`/`onEnd`/`onProgress`/`onError` callbacks, and calls
  `ArduinoOTA.begin()`. `loop()` calls `ArduinoOTA.handle()` on every
  iteration — this is required in every future version of the firmware, or
  OTA silently stops working and USB is needed to recover.
- **BLE/WiFi coexistence bug**: running BLE and WiFi/OTA at the same time
  on the ESP32 caused OTA transfers to fail mid-update (`"Receive Failed"`
  / `"Connect Failed"`), a known ESP32 BLE+WiFi radio coexistence issue —
  confirmed by testing (OTA succeeded with BLE off, failed with BLE
  active). Fix: `bleMouse.end()` is called inside `ArduinoOTA.onStart()`,
  freeing the BLE radio right when an update begins. BLE comes back
  automatically after the OTA-triggered reboot re-runs `setup()` and calls
  `bleMouse.begin()` again, so BLE mouse operation is normal day-to-day and
  only pauses for the brief window an actual OTA push happens.
- **Partition scheme**: the default ESP32 partition table allows only
  ~1.25MB per app image and no OTA dual-slot support; the combined
  BLE+WiFi+OTA firmware (~1.6MB) didn't fit and also needs two OTA slots to
  work at all. Fixed with `board_build.partitions = min_spiffs.csv` in
  `platformio.ini`, giving two ~1.9MB OTA-capable app partitions on the 4MB
  flash.
- **Firewall**: espota's host-side server needs an inbound connection from
  the ESP32 back to the PC; Windows Firewall initially blocked this and had
  to be configured to allow the relevant process/port (rather than
  disabling the firewall).

## Setup

1. Install [PlatformIO](https://platformio.org/) (VS Code extension or CLI).
2. Open [`Code/HID-Mouse`](Code/HID-Mouse) as a PlatformIO project — the
   `espressif32` platform and `ESP32 BLE Mouse` library are pulled
   automatically from `platformio.ini`'s `lib_deps` on first build.
3. Create `include/secrets.h` (gitignored, not provided in the repo) with
   `#define`s for `WIFI_SSID`, `WIFI_PASSWORD`, `OTA_HOSTNAME`, and
   `OTA_PASSWORD` — required for `main.cpp` to compile, since WiFi/OTA are
   always active.
4. For USB flashing, set `upload_port` under `[env:esp32dev]` in
   `platformio.ini` to match your board's COM port (Windows) or device path
   (Linux/macOS), then build & flash with `pio run -e esp32dev -t upload`.
5. For OTA flashing (after the first USB flash), set `upload_port` under
   `[env:esp32dev-ota]` to the board's LAN IP and run
   `pio run -e esp32dev-ota -t upload`.
6. Monitor serial output: `pio device monitor` (make sure `monitor_speed`
   matches the `Serial.begin()` rate used in `main.cpp`, currently 115200).

## Pairing

1. Power the ESP32 (USB or 5V supply) and flash the firmware.
2. On the host device, open Bluetooth settings and scan for new devices.
3. Pair with **"Logitech MX Master 3"** — no PIN expected.
4. Once connected, `runMouse()` starts running automatically (square
   movement + scroll pattern, repeating).

Confirmed working via BLE pairing on Windows during development; expected
to behave the same on Linux, macOS, and Android since all four treat a BLE
HID mouse identically.

## Known issues / TODO

- **Hardcoded OTA password in `platformio.ini`**: `[env:esp32dev-ota]`
  currently has `--auth=1234567890` hardcoded in plaintext instead of
  referencing it via `${sysenv.OTA_PASSWORD}` as originally intended.
  `platformio.ini` is **not** gitignored (only `secrets.h` is), so
  committing it as-is would leak the OTA password into git history. Needs
  to be fixed to pull from the environment before this is committed.

## Future expansion

- **External command source**: later, the ESP32 may take commands from an
  external program (e.g. Python over Serial or Wi-Fi) that tells it when
  to move/click/scroll, instead of running a fixed pattern on its own. The
  mouse-control calls already used in `runMouse()` are the intended seam
  for this — a future command parser would call the same
  `bleMouse.move()` / `click()` operations, so this doesn't require
  redesigning the mouse-control code, only what decides *when* to call it.

## Development log

Chronological record of what's been done and why. Newest entries at the
bottom.

- **2026-08-07** — Defined project scope and the BLE-vs-Classic-HID
  decision (BLE chosen); wrote this README as the plan before any code was
  written.
- **2026-08-07** — Implemented the BLE mouse in
  [`Code/HID-Mouse`](Code/HID-Mouse) using PlatformIO instead of Arduino
  IDE, with the `t-vk/ESP32 BLE Mouse` library. Flashed to a real
  ESP32-WROOM-32 (38-pin) board and confirmed working: boots, logs over
  serial at 115200 baud, pairs over BLE, and runs the square-move/scroll
  automation once connected.
- **2026-08-07** — Started wiring a physical pushbutton to pause/resume the
  automation (toggling a bool that gates `runMouse()`) without dropping the
  BLE connection; code for the button not yet written. Pin moved from
  GPIO 15 to **GPIO 22** to avoid GPIO 15's strapping-pin behavior
  (silences the boot log if held low at reset) — GPIO 22 has no boot-time
  role.
- **2026-08-09** — Finished the GPIO 22 pause/resume button:
  `handleButton()`/`buttonPressed()` in
  [`main.cpp`](Code/HID-Mouse/src/main.cpp) toggle `automationEnabled` on
  each debounced press, gating `runMouse()` in `loop()`. Confirmed working
  on hardware — one press toggles automation cleanly without dropping the
  BLE connection.
- **2026-08-09** — Added OTA (over-the-air) WiFi firmware updates via
  `ArduinoOTA` in [`main.cpp`](Code/HID-Mouse/src/main.cpp), with
  credentials pulled from a gitignored
  [`include/secrets.h`](Code/HID-Mouse/include/secrets.h). Found and fixed
  a BLE+WiFi radio coexistence bug that broke OTA transfers, by calling
  `bleMouse.end()` in `ArduinoOTA.onStart()`. Changed the partition table
  to `min_spiffs.csv` in
  [`platformio.ini`](Code/HID-Mouse/platformio.ini) so the combined
  BLE+WiFi+OTA firmware fits and has the two OTA-capable app slots it
  needs; split `platformio.ini` into separate `esp32dev` (USB) and
  `esp32dev-ota` (WiFi) environments. Also hit and fixed a Windows Firewall
  block on the inbound espota connection. Left unresolved: the OTA
  password in `[env:esp32dev-ota]` is currently hardcoded in plaintext
  rather than pulled from the environment — see
  [Known issues / TODO](#known-issues--todo).
