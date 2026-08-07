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
- Pushbutton on **GPIO 22** (pause/resume automation — see
  [Future expansion](#future-expansion)).

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

- [`platformio.ini`](Code/HID-Mouse/platformio.ini): `env:esp32dev`,
  `framework = arduino`, with `lib_deps = t-vk/ESP32 BLE Mouse@^0.3.0`.
- [`src/main.cpp`](Code/HID-Mouse/src/main.cpp): creates a `BleMouse`
  instance advertised as **"Logitech MX Master 3"**; `setup()` starts BLE
  advertising (`bleMouse.begin()`); `loop()` runs `runMouse()` once a host
  is connected (`bleMouse.isConnected()`), which traces a small square with
  the cursor, scrolling up/down between moves, on a ~150 ms step pace.
- Confirmed working on real hardware: flashes and boots cleanly, serial log
  prints correctly at 115200 baud (`monitor_speed = 115200` needed in
  `platformio.ini` — PlatformIO's monitor defaults to 9600), and the board
  pairs as a standard BLE mouse.

## Setup

1. Install [PlatformIO](https://platformio.org/) (VS Code extension or CLI).
2. Open [`Code/HID-Mouse`](Code/HID-Mouse) as a PlatformIO project — the
   `espressif32` platform and `ESP32 BLE Mouse` library are pulled
   automatically from `platformio.ini`'s `lib_deps` on first build.
3. Set `upload_port` in `platformio.ini` to match your board's COM port
   (Windows) or device path (Linux/macOS).
4. Build & flash: `pio run -t upload`.
5. Monitor serial output: `pio device monitor` (make sure `monitor_speed`
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

## Future expansion

- **Pause/resume button (in progress)**: a pushbutton on **GPIO 22** will
  toggle a boolean flag (e.g. `automationEnabled`) that gates whether
  `runMouse()` is called in `loop()`. This pauses/resumes the automated
  actions without dropping the BLE connection (`bleMouse.end()` is not
  used for this — that would fully disconnect/unadvertise instead).
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
