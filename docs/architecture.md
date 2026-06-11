# Architecture Notes

Chinese version: [architecture_CN.md](architecture_CN.md).

## Runtime Shape

The firmware is organized around a small app state machine plus several background FreeRTOS tasks.

```text
Touch task -> GestureManager -> AppState -> stateChangeHandler()
UI task    -> active UI class draws the current page
Camera task -> CameraManager frames -> Camera Debug / AI Vision UI
Vision task -> heuristic FaceDetector -> FaceTracker / photo centering
AI task     -> XiaoZhi WebSocket, Opus audio, MCP tools, tap/silence end-of-input
Power task  -> battery/sleep status and low-voltage callbacks
Network task -> Wi-Fi reconnect, NTP sync, web server lifecycle, and status updates
Music task  -> SD MP3/WAV streaming when music is active
```

## Main States

- `FACE`: default expression page, short-burst heuristic face detection, touch and IMU reactions.
- `MENU`: seven app icons: Wi-Fi, Camera, Timer, Music, System, Album, Memo, shown across pages.
- `CAMERA_DEBUG`: foreground camera preview and SD photo capture.
- `AI_VISION`: foreground camera preview for XiaoZhi vision description.
- `AI`: XiaoZhi voice interaction page with tap-to-finish input and about 900 ms local-silence auto-stop.
- `ALBUM`: local `/photos` browser with thumbnail grid, full-screen view, and deletion.
- `MEMO`: NVS memo and reminder list page with tap-to-detail and delete support.
- `POMODORO`, `MUSIC`, `SYSTEM_INFO`, `AFFINITY`, `SETTINGS`, `SLEEP`: focused utility pages. `AFFINITY` displays short-term Mood and long-term Bond.

## Resource Ownership

- Camera is lazy-started. Face-page detection uses short bursts; Camera Debug and AI Vision own foreground capture.
- Camera foreground startup is deferred to the main loop so touch/state callbacks stay responsive.
- XiaoZhi voice owns microphone and speaker while listening/speaking. Music is stopped or paused before exclusive audio use. End-of-input uses tap-to-finish plus local silence detection without changing the current Opus audio parameters.
- The local web server only runs while Wi-Fi is connected and exposes page control, status, servo actions, and photo browsing hooks.
- Servo motions go through `ServoMotionController` so large angle changes are rate-limited.
- PCA9685 is optional. If it is missing for three scans in one boot, the driver disables itself instead of polling forever.

## Folder Responsibilities

- `src/app`: app state, gestures, event bus, Mood/Bond affinity, memos, shared status snapshot.
- `src/ui`: display-only page classes.
- `src/vision`: camera manager, heuristic detector, tracker, IMU orientation.
- `src/ai`: XiaoZhi activation, WebSocket, Opus audio, MCP tools.
- `src/audio`: SD music playback.
- `src/servo`: PCA9685 driver and safe motion planner.
- `src/power`: battery voltage, low battery, and sleep helpers.
- `src/storage`: SD card probing, photo path generation, file writes, and file deletion.
- `src/network`: Wi-Fi, remote AI Vision HTTP client, and local web control server.

## Peripheral List For Reports

These are already used by the firmware and can be cited in lab reports. No System Resources display page was added.

- LCD + M5GFX/PSRAM canvas: UI, camera preview, album, and status pages.
- Touch: gestures, buttons, and navigation.
- IMU: shake, twist, Pomodoro orientation selection, and wakeup.
- Camera: photo capture, AI Vision, and short heuristic detection bursts.
- Mic/Speaker: XiaoZhi voice, TTS, alerts, and music.
- SD: photo storage, album browsing, and MP3/WAV reads.
- Wi-Fi: XiaoZhi OTA/WebSocket, AI Vision HTTP, web control, and NTP.
- PMU/NVS: battery protection plus Mood/Bond and memo persistence.
- PortA I2C/PCA9685: optional two-axis servo base with automatic fallback when missing.
- ADC: ambient light detection on GPIO1 (PortA).
- RTC: deep sleep with timer wakeup for low-battery protection.
- DMA: I2S audio DMA for microphone capture and speaker playback.

## Extension Tips

- Add a new page by extending `AppStateEnum`, adding a UI class under `src/ui`, then routing it in `stateChangeHandler()`, `gestureEventHandler()`, and any external control surfaces such as XiaoZhi tools or the web server if needed.
- Add new AI tools in `xiaozhi_client.*` and process them through pending-tool handlers in `main.cpp`.
- Keep feature flags and timing constants in `src/config/app_config.h` so demos can be tuned without hunting through implementation files.
