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
Network task -> Wi-Fi reconnect, NTP sync, web server lifecycle, and status updates
Music task  -> SD MP3/WAV streaming when music is active
```

## Main States

- `FACE`: default expression page, short-burst heuristic face detection, touch and IMU reactions.
- `MENU`: six app icons: Wi-Fi, Camera, Timer, Music, System, Album.
- `CAMERA_DEBUG`: foreground camera preview and SD photo capture.
- `AI_VISION`: foreground camera preview for XiaoZhi vision description.
- `AI`: XiaoZhi voice interaction page.
- `ALBUM`: local `/photos` browser with thumbnail grid, full-screen view, and deletion.
- `POMODORO`, `MUSIC`, `SYSTEM_INFO`, `AFFINITY`, `SETTINGS`, `SLEEP`: focused utility pages.

## Resource Ownership

- Camera is lazy-started. Face-page detection uses short bursts; Camera Debug and AI Vision own foreground capture.
- Camera foreground startup is deferred to the main loop so touch/state callbacks stay responsive.
- XiaoZhi voice owns microphone and speaker while listening/speaking. Music is stopped or paused before exclusive audio use.
- The local web server only runs while Wi-Fi is connected and exposes page control, status, servo actions, MJPEG stream hooks, local OTA upload, and photo browsing hooks.
- Servo motions go through `ServoMotionController` so large angle changes are rate-limited.
- PCA9685 is optional. If it is missing for three scans in one boot, the driver disables itself instead of polling forever.
- Power policy is tiered: low battery changes UI behavior first, and sustained critical battery triggers deep sleep.

## Folder Responsibilities

- `src/app`: app state, gestures, event bus, affinity, shared status snapshot.
- `src/ui`: display-only page classes.
- `src/vision`: camera manager, heuristic detector, tracker, IMU orientation.
- `src/ai`: XiaoZhi activation, WebSocket, Opus audio, MCP tools.
- `src/audio`: SD music playback.
- `src/servo`: PCA9685 driver and safe motion planner.
- `src/power`: battery voltage, low and critical battery handling, and sleep helpers.
- `src/storage`: SD card probing, photo path generation, file writes, and file deletion.
- `src/network`: Wi-Fi, remote AI Vision HTTP client, and local web control server.

## Extension Tips

- Add a new page by extending `AppStateEnum`, adding a UI class under `src/ui`, then routing it in `stateChangeHandler()`, `gestureEventHandler()`, and any external control surfaces such as XiaoZhi tools or the web server if needed.
- Add new AI tools in `xiaozhi_client.*` and process them through pending-tool handlers in `main.cpp`.
- If you add new web features, update both `web_server.*` and `web_ui.h`, and document whether they only mirror active foreground resources or can allocate those resources themselves.
- Keep feature flags and timing constants in `src/config/app_config.h` so behavior can be tuned without hunting through implementation files.
