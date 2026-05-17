# AGENTS.md

This file provides guidance to Codex/Coding agents when working with this repository.

## Project Overview

**CoreS3 AI Desktop Pet Interaction System** is an embedded firmware project for M5Stack CoreS3 / ESP32-S3. It is used as a National College Embedded Competition desktop pet demo (全国大学生嵌入式比赛). The CoreS3 acts as the pet head/brain, with optional base hardware for battery, PCA9685 servo control, and two-axis movement.

The current design separates the face UI from camera/AI backends: the screen renders the pet expression, while camera preview, AI voice, AI vision, storage, and timer features run as separate state-driven modules.

## Project Status

Active demo firmware. The project is no longer in the planning-only stage.

Implemented:

- Animated face UI with 11 emotions: `NORMAL`, `HAPPY`, `CURIOUS`, `LISTENING`, `THINKING`, `SPEAKING`, `SURPRISED`, `SLEEPY`, `TRACKING`, `SHY`, `SICK`.
- Touch gestures: tap, double tap, left/right/up/down swipe, long press.
- Menu with Wi-Fi, Camera, Timer, Music, System, and Servo app icons.
- Camera preview and JPEG capture to SD.
- AI Vision preview and JPEG description request through a XiaoZhi-provided vision endpoint.
- IMU orientation based Pomodoro timer with four presets and screen rotation.
- SD MP3/WAV music player. MP3 output is mixed to mono for the CoreS3 speaker at a conservative default volume.
- PCA9685 servo communication test through CoreS3 PortA, mapping IMU X/Y tilt to horizontal channel 0 and vertical channel 1 in a 10-170 degree test range. Servo Test uses an independent 90/90 test center.
- Shared safe servo motion layer for Face touch/expression reactions, XiaoZhi pet reactions, XiaoZhi `self.servo.control` commands, and the dance demo. Face and XiaoZhi expression poses keep the mechanical center at pan 90 degrees and tilt 140 degrees.
- Memory-only Bond/affinity page opened by down swipe from Face.
- Photo face-centering framework that can drive the servos from real `FaceResult` boxes when a real detector backend is added.
- XiaoZhi OTA activation/config, TLS WebSocket, Opus mic upload, Opus TTS playback, MCP handshake, and MCP tools.
- Wi-Fi auto reconnect, SD retry/fallback, and basic power/sleep handling.

Not implemented or intentionally disabled:

- Real local face detection. `FaceDetector` returns no detections and does not fake results.
- PID gimbal tracking or autonomous base behavior. Current servo support is safe open-loop pose control plus a real-detector tracking hook, not autonomous motion.
- Real battery voltage reading. `PowerManager::readVoltage()` is still a placeholder.
- Full-duplex audio interruption or echo cancellation.

## State Machine

- `FACE` - default expression page.
- `MENU` - six-icon app menu.
- `WIFI_INFO` - Wi-Fi status page.
- `CAMERA_DEBUG` - camera preview and SD photo capture.
- `AI_VISION` - camera preview for XiaoZhi vision requests.
- `POMODORO` - IMU selected timer.
- `MUSIC` - SD MP3/WAV player.
- `SYSTEM_INFO` - heap, PSRAM, power, and vision status page.
- `SERVO_TEST` - PCA9685 communication and IMU tilt servo test page.
- `AFFINITY` - memory-only Bond page with score, level, mood, and recent interaction.
- `AI` - XiaoZhi voice interaction page.
- `SLEEP` - dim screen sleep page.

## Code Structure

```text
src/
├── main.cpp
├── app/
│   ├── app_state.h/.cpp
│   ├── affinity_manager.h/.cpp
│   ├── gesture_manager.h/.cpp
│   └── event_bus.h/.cpp
├── ai/
│   ├── xiaozhi_client.h/.cpp
│   └── voice_state.h
├── audio/
│   └── music_manager.h/.cpp
├── config/
│   ├── app_config.h
│   └── wifi_secrets.example.h
├── network/
│   ├── wifi_manager.h/.cpp
│   └── vision_client.h/.cpp
├── power/
│   └── power_manager.h/.cpp
├── servo/
│   ├── servo_controller.h/.cpp
│   └── servo_motion_controller.h/.cpp
├── storage/
│   └── storage_manager.h/.cpp
├── ui/
│   ├── face_ui.h/.cpp
│   ├── menu_ui.h/.cpp
│   ├── camera_debug_ui.h/.cpp
│   ├── pomodoro_ui.h/.cpp
│   ├── info_ui.h/.cpp
│   ├── music_ui.h/.cpp
│   ├── servo_test_ui.h/.cpp
│   ├── affinity_ui.h/.cpp
│   └── ui_theme.h
└── vision/
    ├── camera_manager.h/.cpp
    ├── face_detector.h/.cpp
    ├── face_tracker.h/.cpp
    ├── face_tracking_controller.h/.cpp
    └── imu_orientation.h/.cpp
```

## FreeRTOS Tasks

| Task | Function | Frequency |
|------|----------|-----------|
| UI | Draw active screen with PSRAM canvas where possible | 20 FPS |
| Touch | Read touch and emit gestures | 50 Hz |
| Camera | Push camera frames to Camera Debug / AI Vision UI | 15 FPS |
| Vision | Reserved local face detection/tracking loop; feeds photo servo centering only when a real backend is available | 5 FPS |
| AI | Process XiaoZhi WebSocket, activation, audio channel requests | 50 Hz tick |
| Power | Battery/sleep status update | 1 Hz |
| Network | Wi-Fi reconnect and menu status update | 5 s interval |
| Music | Background MP3/WAV streaming task inside `MusicManager` | event-driven |
| Servo Test | IMU tilt sampling and PCA9685 writes from the main loop while `SERVO_TEST` is active | 25 Hz max |

## Gesture Routing

- Face: right swipe -> Menu; left swipe or double tap -> AI; down swipe -> Bond; tap top -> HAPPY + nod; tap bottom -> SHY + look down; tap left/right -> CURIOUS with gaze and pan servo; long press -> Sleep.
- AI: single tap toggles listening; right swipe or long press -> Face.
- Bond: Back, up swipe, or left swipe -> Face.
- Menu: tap app icon -> selected page; Back -> Face; left swipe -> Face.
- Camera Debug: SHOT -> optionally center a real detected face, then save JPEG to `/photos`; Back or left swipe -> Menu.
- AI Vision: Back or left swipe -> close preview and return to AI.
- Pomodoro: IMU orientation selects preset before start; Start/Pause/Reset buttons control timer; Back or left swipe -> Menu.
- Music: Play/Pause, Stop, Next, Back; left swipe -> Menu.
- Servo Test: Back or left swipe -> Menu and restore the Face/XiaoZhi 90/140 center; tap center area -> center servos at 90/90; long press -> release PWM.
- Sleep: tap or double tap -> Face.

## XiaoZhi AI Notes

- OTA endpoint is `https://api.tenclass.net/xiaozhi/ota/`.
- Firmware identity comes from `XIAOZHI_BOARD_TYPE` and `XIAOZHI_FIRMWARE_VERSION` in `src/config/app_config.h`.
- The client sends a WebSocket `hello` with Opus audio parameters and MCP enabled.
- Audio upload is 16 kHz mono Opus, 60 ms frames, 24 kbps target bitrate.
- Returned TTS Opus is decoded at the server-provided sample rate and played through M5Unified Speaker.
- CoreS3 mic and speaker are handled as half-duplex.
- MCP tools currently exposed:
  - `self.camera.open`
  - `self.camera.close`
  - `self.camera.capture_photo`
  - `self.vision.describe_scene`
  - `self.pomodoro.open`
  - `self.music.control`
  - `self.pet.react`
  - `self.servo.control`
- AI Vision only works when the XiaoZhi MCP initialize params provide a vision URL/token.
- `self.servo.control` supports `center`, `left`, `right`, `up`, `down`, `nod`, `shake`, `dance`, and `release`. It must not switch pages or reopen the XiaoZhi audio channel. `dance` starts servo motion first, then uses `/music` when available; music or SD failure must not cancel the servo motion.

## Face Detection Blocker

Do not fake real face detection.

The current firmware uses Arduino framework on PlatformIO. ESP-DL/ESP-WHO face detection models and components require an ESP-IDF CMake integration path, so `FaceDetector` in `src/vision/face_detector.*` intentionally reports unavailable backend and returns `detected=false`.

Likely future paths:

1. Migrate the firmware to ESP-IDF.
2. Use Arduino as an ESP-IDF component.
3. Port the needed ESP-DL inference code/model loading into an Arduino-compatible library.

## Development Rules

- Preserve user changes. Check `git status --short` before editing.
- Keep Wi-Fi credentials in `src/config/wifi_secrets.h`; never commit real SSID/password files.
- Build with `pio run`. If `pio` is not on `PATH`, locate the PlatformIO executable.
- Keep display rendering centralized through UI classes and PSRAM `M5Canvas` where possible.
- Keep camera framebuffer lifetime explicit: release frames after use and never use data after returning/freeing the framebuffer.
- SD card handling must tolerate missing/flaky cards without rebooting. The retry sequence is 25, 10, 4, and 1 MHz.
- Stop music before exclusive speaker/mic use by XiaoZhi AI or Pomodoro completion melody.
- Do not claim local face recognition, closed-loop servo tracking, or battery voltage measurement unless those paths are actually implemented and tested.
- Photo face tracking must report unavailable when `FaceDetector::backendAvailable()` is false; do not fake detections for demos.
- Bond/affinity is intentionally memory-only for now; do not describe it as persistent.

## GitHub Hygiene

- Commit `src/config/wifi_secrets.example.h`, not `src/config/wifi_secrets.h`.
- Do not commit build output, monitor logs, local `.pio`, or local editor database files.
- The GitHub Actions workflow should run PlatformIO, not the default CMake starter workflow.
- README files should describe current firmware behavior, not the older planning-stage design.
