# CoreS3 AI Desktop Pet

M5Stack CoreS3 firmware for an embedded competition desktop pet demo. The project combines a touch expression face, Wi-Fi, camera debug capture, IMU Pomodoro interaction, shake-to-sick expression, and a working XiaoZhi AI voice assistant path.

## Current Status

- Face UI: touch expressions, blinking, gaze offset, and SICK expression on body shake.
- Menu: Wi-Fi status, Camera Debug, Pomodoro, and System Info.
- Camera Debug: preview, Back button, SHOT button, JPEG save path for SD card.
- Pomodoro: IMU-based preset selection and four-direction screen rotation.
- XiaoZhi AI: real OTA activation request, WebSocket connection, Opus microphone upload, Opus TTS playback, and minimal MCP handshake are implemented.
- Face detection: disabled. No fake face detection or skin-color detection is active.

## Hardware

- M5Stack CoreS3
- MicroSD/TF card formatted as FAT/FAT32, used by Camera Debug photo capture
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

This project was tested with:

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

```powershell
Copy-Item src\config\wifi_secrets.example.h src\config\wifi_secrets.h
```

Edit `src\config\wifi_secrets.h`:

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

The current XiaoZhi build depends on:

- `links2004/WebSockets`
- `sh123/esp32_opus`
- `bblanchon/ArduinoJson`
- `M5CoreS3`, `M5Unified`, `M5GFX`

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

Useful XiaoZhi logs:

```text
AI task: requesting activation code...
XiaoZhi: OTA POST identity=m5stack-core-s3/2.2.6
XiaoZhi: device activated, got WS config
XiaoZhi: WS connected
XiaoZhi: session_id=...
XiaoZhi: MCP method=initialize id=1
XiaoZhi: MCP method=tools/list id=2
XiaoZhi: sent listen start
XiaoZhi: WS text: {"type":"stt",...}
XiaoZhi: WS text: {"type":"tts",...}
```

## Controls

- Face page:
  - Right swipe: open Menu
  - Left swipe or double tap: open XiaoZhi AI
  - Tap top: HAPPY
  - Tap bottom: SHY
  - Tap left/right: CURIOUS with gaze direction
  - Shake body: SICK expression for about 2 seconds
  - Long press: Sleep
- AI page:
  - Connects to XiaoZhi after Wi-Fi is ready
  - Single tap toggles listening
  - Right swipe or long press returns to Face
- Menu:
  - Tap active card to enter
  - Right swipe to switch card
  - Back returns to Face
- Camera Debug:
  - SHOT saves `/photos/IMG_####.jpg` to SD when SD is available
  - Back or left swipe returns to Menu
- Pomodoro:
  - Rotate device to select timer preset and rotate UI
  - Start/Pause and Reset buttons control the timer

## How XiaoZhi AI Works

The implementation is in `src/ai/xiaozhi_client.h` and `src/ai/xiaozhi_client.cpp`.

1. The AI page is entered from the Face page by left swipe or double tap.
2. The AI task requires Wi-Fi, then sends an OTA activation request to the XiaoZhi OTA endpoint.
3. The OTA request includes the CoreS3 identity, device MAC, generated client UUID, firmware identity `m5stack-core-s3/2.2.6`, chip info, flash size, and app version.
4. The OTA response returns WebSocket configuration: URL and token. No fake verification code is generated locally.
5. The client opens a TLS WebSocket and sends headers:
   - `Authorization: Bearer <token>`
   - `Protocol-Version`
   - `Device-Id`
   - `Client-Id`
6. The client sends a `hello` message declaring:
   - transport: `websocket`
   - audio format: `opus`
   - microphone input: 16 kHz mono
   - frame duration: 60 ms
   - MCP feature enabled
7. The server returns a session id and TTS audio parameters, normally 24 kHz Opus.
8. The device replies to the required MCP bootstrap:
   - `initialize`: returns protocol version, empty tool capability set, and device info.
   - `tools/list`: returns an empty tools list.
   - `tools/call`: returns a clear error because this build does not expose device tools yet.
9. After MCP is ready, the client sends `listen start`.
10. Microphone PCM is captured at 16 kHz, encoded to Opus, queued, and sent as WebSocket binary frames.
11. Server binary Opus audio is decoded to PCM and played through the CoreS3 speaker.

CoreS3 microphone and speaker cannot run at the same time, so the implementation is half-duplex:

- Listening: speaker is stopped, microphone records, Opus audio is uploaded.
- Speaking: microphone is stopped, speaker starts, received Opus TTS is decoded and played.
- After `tts stop`, the client restarts listening if the AI page still wants listening mode.

Playback details:

- Opus encode: 16 kHz mono, 60 ms frames, 24 kbps target bitrate.
- Opus decode: server sample rate from `hello`, currently observed as 24 kHz.
- Speaker output: M5Unified speaker path, 48 kHz I2S output, three internal RAM playback buffers, no per-frame forced interruption.
- Audio capture task stack is enlarged to avoid Opus stack overflow.

## Known Limitations

- XiaoZhi voice is now connected, but no custom MCP device tools are exposed yet.
- Music or long TTS playback depends on the XiaoZhi service behavior and CoreS3 speaker quality.
- CoreS3 built-in mic and speaker are half-duplex, so true full-duplex interruption or echo cancellation is not implemented.
- SD card detection is still hardware/card-format sensitive. The firmware should not reboot on SD failure, but photo save requires a working FAT/FAT32 card.
- Face detection remains intentionally disabled in this Arduino/PlatformIO build.

## Face Detection Notes

Face detection is intentionally disabled. The previous skin-color detector is not used because it can produce false positives and would misrepresent real face recognition.

Future real face detection should use a verified ESP-DL/ESP-WHO integration path, likely via ESP-IDF or Arduino-as-IDF-component.

## Troubleshooting

- Upload loop or failed upload: long-press RESET for about 3 seconds, upload again, then press RESET once.
- XiaoZhi OTA `400`: check `XIAOZHI_BOARD_TYPE`, `XIAOZHI_FIRMWARE_VERSION`, and OTA JSON identity in serial logs.
- XiaoZhi WebSocket disconnects after MCP: check that `initialize` and `tools/list` responses appear before `listen start`.
- No AI response: confirm Wi-Fi is connected, enter AI page, and check serial logs for `WS connected`, `session_id`, `sent listen start`, `stt`, and `tts`.
- Speaker is noisy: confirm the latest firmware is flashed; current playback uses internal RAM triple buffering and 48 kHz speaker output.
- SD not found: format the card as FAT/FAT32, reinsert it, and check serial logs for `SD.begin failed`, `CARD_NONE`, `Root open failed`, or `Probe write failed`.
