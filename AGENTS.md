# AGENTS.md

This file provides guidance to Codex (Codex.ai/code) when working with code in this repository.

## Project Overview

**CoreS3 AI Desktop Pet Interaction System** — an embedded system for the National College Embedded Competition (全国大学生嵌入式比赛). Uses M5Stack CoreS3 (ESP32-S3) as the main controller. The CoreS3 acts as the pet's "head/brain", with a custom base for battery, power, PCA9685 servo driver, and 2-axis servos.

The core idea: decouple the front-end expression UI from backend camera/vision — the screen shows the pet's face (expressions, eyes), while the camera runs face detection invisibly in the background. Eye gaze direction follows the detected face position.

## Project Status

**Active development — interactive demo phase.** Core UI, Wi-Fi, SD storage, camera preview, IMU-based pomodoro, and AI placeholder are implemented. Face detection is blocked by ESP-DL/Arduino framework incompatibility (see Blocker section below).

## High-Level Architecture

### Main States (State Machine)
- `STATE_FACE` — Default expression UI with 4-zone touch (NORMAL, HAPPY, CURIOUS, TRACKING, LISTENING, THINKING, SPEAKING, SURPRISED, SLEEPY, SHY)
- `STATE_MENU` — Right-swipe function menu (Wi-Fi status, Camera Debug, Pomodoro Timer, System Info)
- `STATE_CAMERA_DEBUG` — Camera preview with FPS overlay, right-side SHOT button, top-right Back
- `STATE_POMODORO` — IMU flip-select 4-preset timer (Focus 25m / Short 5m / Long 15m / Deep 50m)
- `STATE_AI` — XiaoZhi AI placeholder (left-swipe from Face, right-swipe back; shows LISTENING/THINKING/SPEAKING expressions)
- `STATE_SLEEP` — Low-power sleep mode

### System Layers
1. **Expression Interaction Layer** — 10 expressions (incl. SHY), 4-zone touch, temporary gaze override, left-swipe AI entry
2. **Background Vision Layer** — Camera capture, face detection (BLOCKED), eye gaze / servo tracking
3. **AI Voice Layer** — XiaoZhi AI placeholder mode; real WebSocket/OPUS deferred
4. **Base Control Layer** — PCA9685, 2-axis servos, battery/power monitoring
5. **Storage Layer** — SD/TF with auto-retry (25→10→4 MHz fallback), ensureReady() before capture

### Code Structure
```
src/
├── main.cpp
├── app/
│   ├── app_state.h/.cpp          // Main state machine (FACE/MENU/CAMERA_DEBUG/POMODORO/AI/SLEEP)
│   ├── gesture_manager.h/.cpp    // Tap, double-tap, swipe, long-press with coordinates
│   └── event_bus.h/.cpp          // Inter-module event passing
├── ui/
│   ├── face_ui.h/.cpp            // 10 expressions + eye gaze + temporary gaze override
│   ├── menu_ui.h/.cpp            // Right-swipe function menu with real Wi-Fi status
│   ├── camera_debug_ui.h/.cpp    // Camera debug with right-side SHOT, PSRAM canvas
│   └── pomodoro_ui.h/.cpp        // IMU flip-select 4-preset timer
├── vision/
│   ├── camera_manager.h/.cpp     // Camera capture + captureJpegToFile
│   ├── face_detector.h/.cpp      // Face detection (BLOCKED — see below)
│   ├── face_tracker.h/.cpp       // Face center smoothing & tracking
│   └── imu_orientation.h/.cpp    // IMU accelerometer gravity → PomoOrientation
├── ai/
│   ├── xiaozhi_client.h/.cpp     // XiaoZhi AI placeholder (no real WebSocket)
│   └── voice_state.h             // Listening/Thinking/Speaking states
├── network/
│   └── wifi_manager.h/.cpp       // Wi-Fi connect/auto-reconnect with status
├── storage/
│   └── storage_manager.h/.cpp    // SD/TF with refresh/ensureReady/freq fallback
├── power/
│   └── power_manager.h/.cpp      // Battery, power, low-power strategy
└── config/
    ├── app_config.h              // Parameter configuration
    ├── wifi_secrets.h            // Wi-Fi credentials (git-ignored)
    └── wifi_secrets.example.h    // Wi-Fi credentials template
```

### FreeRTOS Tasks
| Task | Function | Frequency |
|------|----------|-----------|
| UI Task | Draw expressions, menus, pomodoro via PSRAM canvas | 20 FPS |
| Touch Task | Read touch points, recognize gestures | 50 Hz |
| Camera Task | Camera frame capture for debug preview | 10-15 FPS |
| Vision Task | Face detection / tracking (blocked) | 2-5 FPS |
| AI Task | XiaoZhi AI placeholder process | 50 Hz tick |
| Power Task | Battery voltage, low-power protection | 1 Hz |
| Network Task | Wi-Fi auto-reconnect, status updates | 10 Hz |

### Touch Gesture Routing
- **Face page**: Right-swipe → Menu, Left-swipe → AI, Single-tap → 4-zone touch (top=HAPPY, bottom=SHY, left/right=CURIOUS+gaze), Double-tap → AI (if WiFi), Long-press → Sleep
- **AI page**: Right-swipe → Face, Single-tap → LISTENING, Long-press → Face
- **Menu**: Left-swipe → Face, Right-swipe → next card, tap card → enter, tap Back → Face
- **Camera Debug**: Tap SHOT (right side) → capture, tap Back (top-right) → Menu, Left-swipe → Menu
- **Pomodoro**: IMU flip selects preset, tap Start/Pause/Reset, tap Back → Menu

### Face Touch Zones
- Top (y < cy/2): HAPPY + gaze up (摸头)
- Bottom (y > cy+cy/2): SHY + gaze down (害羞)
- Left (x < cx): CURIOUS + gaze left
- Right (x >= cx): CURIOUS + gaze right
- Temporary gaze overrides vision tracking for 1200-1500ms

## Blocker: Real Face Detection

**ESP-DL/ESP-WHO requires ESP-IDF v5.3+ CMake build system.** Current project uses Arduino framework on PlatformIO (espressif32). ESP-DL's `.espdl` model format and component system cannot be linked under Arduino framework.

FaceDetector currently returns `detected=false` and prints a BLOCKER report at startup.

**Resolution options:**
1. Switch entire project to ESP-IDF framework (requires rewriting M5CoreS3/M5GFX integration)
2. Use Arduino-as-IDF-component approach (complex but feasible)
3. Manually port ESP-DL C++ inference code + model data as Arduino-compatible library

## Key Technical References

- **M5CoreS3 Arduino Library** — Base library for CoreS3 hardware
- **M5Unified** — Unified M5Stack device management (Display, Touch, Speaker, Microphone, IMU)
- **M5GFX** — Graphics library with PSRAM M5Canvas support
- **xiaozhi-esp32** (https://github.com/78/xiaozhi-esp32) — Open-source ESP32-S3 voice assistant reference
- **ESP-DL** (https://github.com/espressif/esp-dl) — Deep learning inference (requires ESP-IDF, incompatible with Arduino)
- **ESP-WHO** (https://github.com/espressif/esp-who) — Face detection/recognition framework (requires ESP-IDF)

## Development Environment

- Platform: Arduino framework for ESP32-S3 (M5Stack CoreS3)
- Libraries: M5CoreS3, M5Unified, M5GFX, ArduinoJSON
- Tool: PlatformIO
- Build: `pio run` — RAM ~18.8%, Flash ~48.0%
- Wi-Fi secrets: `src/config/wifi_secrets.h` (git-ignored, use `wifi_secrets.example.h` as template)
