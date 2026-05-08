# CoreS3 AI Desktop Pet

M5Stack CoreS3 based AI desktop pet firmware for an embedded competition demo. The project focuses on a touch-driven expression face, camera debug photo capture, Wi-Fi status, SD card photo storage, IMU Pomodoro interaction, shake-to-sick expression, and a visible XiaoZhi AI placeholder.

## Current Status

- Face UI: touch expressions, eye gaze, blinking, SICK expression on body shake.
- Menu: Wi-Fi status, Camera Debug, Pomodoro, System Info.
- Camera Debug: preview, Back button, SHOT button, JPEG save to SD card.
- Pomodoro: IMU-based preset selection and four-direction screen rotation.
- XiaoZhi AI: placeholder only. Real device verification, WebSocket, OPUS, microphone, and speaker streaming are not implemented in this repository.
- Face detection: disabled. No fake detection and no skin-color detection are active.

## Hardware

- M5Stack CoreS3
- MicroSD/TF card formatted as FAT/FAT32
- Optional base hardware for battery, PCA9685, and servos

CoreS3 SD card SPI pins used by this firmware:

```text
SCK  36
MISO 35
MOSI 37
CS   4
```

## Software Requirements

- Windows with PlatformIO installed
- USB cable connected to CoreS3
- Git

This project was built with the local PlatformIO executable:

```powershell
C:\Users\shenf\.platformio\penv\Scripts\platformio.exe
```

## Clone

```powershell
git clone https://github.com/sfj330/cores3.git
cd cores3
```

## Wi-Fi Setup

Real credentials are kept out of Git.

1. Copy the example file:

```powershell
Copy-Item src\config\wifi_secrets.example.h src\config\wifi_secrets.h
```

2. Edit `src\config\wifi_secrets.h`:

```cpp
#pragma once
constexpr const char* WIFI_SSID = "your_ssid";
constexpr const char* WIFI_PASSWORD = "your_password";
```

`src/config/wifi_secrets.h` is ignored by Git.

## Build

```powershell
C:\Users\shenf\.platformio\penv\Scripts\platformio.exe run
```

Expected result: build succeeds for `m5stack-cores3`.

## Upload

Try automatic upload first:

```powershell
C:\Users\shenf\.platformio\penv\Scripts\platformio.exe run -t upload
```

If automatic port detection fails, use the current known port:

```powershell
C:\Users\shenf\.platformio\penv\Scripts\platformio.exe run -t upload --upload-port COM5
```

If upload does not start, long-press RESET for about 3 seconds to enter download mode. After upload finishes, press RESET once to run the firmware.

## Serial Monitor

```powershell
C:\Users\shenf\.platformio\penv\Scripts\platformio.exe device monitor -p COM5 -b 115200
```

Expected boot log includes:

- `CoreS3 AI Pet Boot`
- `FaceDetector: disabled by configuration`
- `Face detection off`
- task creation logs
- Wi-Fi connection attempts if Wi-Fi is configured
- SD card probe logs such as `SD try 25 MHz`

## Controls

- Face page:
  - Right swipe: open Menu
  - Left swipe or double tap: open XiaoZhi AI placeholder
  - Tap top: HAPPY
  - Tap bottom: SHY
  - Tap left/right: CURIOUS with gaze direction
  - Shake body: SICK expression for about 2 seconds
  - Long press: Sleep
- AI page:
  - Shows XiaoZhi placeholder and device verification requirement
  - Right swipe or long press: return to Face
- Menu:
  - Tap active card to enter
  - Right swipe to switch card
  - Back returns to Face
- Camera Debug:
  - SHOT saves `/photos/IMG_####.jpg` to SD
  - Back or left swipe returns to Menu
- Pomodoro:
  - Rotate device to select timer preset and rotate UI
  - Start/Pause and Reset buttons control the timer

## XiaoZhi AI Notes

This firmware does not generate a real XiaoZhi verification code. In the official XiaoZhi flow, the device firmware connects to XiaoZhi services, displays or speaks a verification code, and the user enters that code in the XiaoZhi console Add Device flow.

References:

- M5Stack CoreS3 XiaoZhi guide: https://docs.m5stack.switch-science.com/en/guide/realtime/xiaozhi/m5cores3
- XiaoZhi WebSocket protocol: https://home.xiaozhi.me/xz-docs/docs/tutorial-comm/websocket-comm/

Future real integration needs OTA activation, WebSocket protocol handling, OPUS audio encoding/decoding, microphone capture, and speaker playback.

## Face Detection Notes

Face detection is intentionally disabled in this Arduino/PlatformIO build. The previous skin-color detector is not used because it can produce false positives and would misrepresent real face recognition.

Future real face detection should use a verified ESP-DL/ESP-WHO integration path, likely via ESP-IDF or Arduino-as-IDF-component.

## Troubleshooting

- Upload loop or failed upload: long-press RESET for about 3 seconds, upload again, then press RESET once.
- SD not found: format the card as FAT/FAT32, reinsert it, and check serial logs for `SD.begin failed`, `CARD_NONE`, `Root open failed`, or `Probe write failed`.
- No Wi-Fi: check `src/config/wifi_secrets.h`, SSID, password, and signal strength.
- Blank or flickering camera preview: enter Camera Debug only after boot completes and check PSRAM is detected in serial logs.
