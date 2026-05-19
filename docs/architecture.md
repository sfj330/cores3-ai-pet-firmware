# Architecture Notes

Chinese version: [architecture_CN.md](architecture_CN.md).

## Runtime Shape

The firmware is organized around a small app state machine plus several background FreeRTOS tasks.

```text
Touch task -> GestureManager -> AppState -> stateChangeHandler()
UI task    -> active UI class draws the current page
Camera task -> CameraManager frames -> Camera Debug / AI Vision UI
Vision task -> heuristic FaceDetector -> FaceTracker / photo centering
AI task     -> XiaoZhi WebSocket, Opus audio, MCP tools
Power task  -> battery/sleep status and low-voltage callbacks
Network task -> Wi-Fi reconnect and status updates
Music task  -> SD MP3/WAV streaming when music is active
```

## Main States

- `FACE`: default expression page, short-burst heuristic face detection, touch and IMU reactions.
- `MENU`: five app icons: Wi-Fi, Camera, Timer, Music, System.
- `CAMERA_DEBUG`: foreground camera preview and SD photo capture.
- `AI_VISION`: foreground camera preview for XiaoZhi vision description.
- `AI`: XiaoZhi voice interaction page.
- `POMODORO`, `MUSIC`, `SYSTEM_INFO`, `AFFINITY`, `SETTINGS`, `SLEEP`: focused utility pages.

## Resource Ownership

- Camera is lazy-started. Face-page detection uses short bursts; Camera Debug and AI Vision own foreground capture.
- Camera foreground startup is deferred to the main loop so touch/state callbacks stay responsive.
- XiaoZhi voice owns microphone and speaker while listening/speaking. Music is stopped or paused before exclusive audio use.
- Servo motions go through `ServoMotionController` so large angle changes are rate-limited.
- PCA9685 is optional. If it is missing for three scans in one boot, the driver disables itself instead of polling forever.

## Folder Responsibilities

- `src/app`: app state, gestures, event bus, affinity, shared status snapshot.
- `src/ui`: display-only page classes.
- `src/vision`: camera manager, heuristic detector, tracker, IMU orientation.
- `src/ai`: XiaoZhi activation, WebSocket, Opus audio, MCP tools.
- `src/audio`: SD music playback.
- `src/servo`: PCA9685 driver and safe motion planner.
- `src/power`: battery voltage, low battery, and sleep helpers.
- `src/storage`: SD card probing and file writing.
- `src/network`: Wi-Fi and remote AI Vision HTTP client.

## Extension Tips

- Add a new page by extending `AppStateEnum`, adding a UI class under `src/ui`, then routing it in `stateChangeHandler()` and `gestureEventHandler()`.
- Add new AI tools in `xiaozhi_client.*` and process them through pending-tool handlers in `main.cpp`.
- Keep feature flags and timing constants in `src/config/app_config.h` so demos can be tuned without hunting through implementation files.
