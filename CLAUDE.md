# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**CoreS3 AI Desktop Pet Interaction System** — an embedded system for the National College Embedded Competition (全国大学生嵌入式比赛). Uses M5Stack CoreS3 (ESP32-S3) as the main controller. The CoreS3 acts as the pet's "head/brain", with a custom base for battery, power, PCA9685 servo driver, and 2-axis servos.

The core idea: decouple the front-end expression UI from backend camera/vision — the screen shows the pet's face (expressions, eyes), while the camera runs face detection invisibly in the background. Eye gaze direction follows the detected face position.

## Project Status

This is a **new project in the planning stage**. No source code has been written yet. The project plan is documented in `CoreS3_AI桌宠交互系统完整方案_M5Burner参考版.docx`.

## High-Level Architecture

### Main States (State Machine)
- `STATE_FACE` — Default expression UI (NORMAL, HAPPY, CURIOUS, TRACKING, LISTENING, THINKING, SPEAKING, SURPRISED, SLEEPY)
- `STATE_MENU` — Right-swipe function menu (Wi-Fi, Camera Debug, Pomodoro Timer, System Info)
- `STATE_CAMERA_DEBUG` — Camera preview with FPS/detection overlay
- `STATE_POMODORO` — Pomodoro timer page
- `STATE_AI` — XiaoZhi AI interaction flow (listening → thinking → speaking)
- `STATE_SLEEP` — Low-power sleep mode

### System Layers
1. **Expression Interaction Layer** — 8 basic expressions, gesture recognition, right-swipe menu
2. **Background Vision Layer** — Camera capture, face detection, eye gaze / servo tracking
3. **AI Voice Layer** — XiaoZhi AI integration: ASR/LLM/TTS, expression/action linkage
4. **Base Control Layer** — PCA9685, 2-axis servos, battery/power monitoring

### Recommended Code Structure
```
src/
├── main.cpp
├── app/
│   ├── app_state.h/.cpp          // Main state machine
│   ├── gesture_manager.h/.cpp    // Tap, double-tap, swipe, long-press
│   └── event_bus.h/.cpp          // Inter-module event passing
├── ui/
│   ├── face_ui.h/.cpp            // 8 expressions + eye gaze
│   ├── menu_ui.h/.cpp            // Right-swipe function menu
│   ├── camera_debug_ui.h/.cpp    // Camera debug page
│   └── pomodoro_ui.h/.cpp        // Pomodoro timer
├── vision/
│   ├── camera_manager.h/.cpp     // Camera capture
│   ├── face_detector.h/.cpp      // Face detection interface
│   └── face_tracker.h/.cpp       // Face center smoothing & tracking
├── ai/
│   ├── xiaozhi_client.h/.cpp     // XiaoZhi AI connection/interaction
│   └── voice_state.h             // Listening/Thinking/Speaking states
├── servo/
│   └── servo_controller.h/.cpp   // PCA9685 + 2-axis servo
├── power/
│   └── power_manager.h/.cpp      // Battery, power, low-power strategy
└── config/
    └── app_config.h              // Parameter configuration
```

### FreeRTOS Task Suggestions
| Task | Function | Frequency |
|------|----------|-----------|
| UI Task | Draw expressions, menus, pomodoro | 20-30 FPS |
| Touch Task | Read touch points, recognize gestures | 50 Hz / event-driven |
| Camera Task | Camera frame capture | 10-15 FPS |
| Vision Task | Face detection / tracking | 2-5 FPS, on-demand |
| Servo Task | Smooth servo control | 20-50 Hz |
| AI Task | XiaoZhi AI audio upload/receive | Triggered on double-tap |
| Power Task | Battery voltage, low-power protection | 1 Hz |

### Touch Gesture Priority
Double-tap > Right-swipe/Left-swipe > Long-press > Single-tap

Double-tap window: 250-350ms. Long-press threshold: 800ms+.

## Key Technical References

- **M5CoreS3 Arduino Library** — Base library for CoreS3 hardware
- **M5Unified** — Unified M5Stack device management (Display, Touch, Speaker, Microphone)
- **xiaozhi-esp32** (https://github.com/78/xiaozhi-esp32) — Open-source ESP32-S3 voice assistant reference
- **ESP-WHO** — Espressif's embedded vision examples (face detection, recognition)
- **M5Burner** — Used for hardware validation only (not for final competition code)

## Development Strategy

- Phase 1: UI prototype — expressions, touch gestures, right-swipe menu (no camera/AI)
- Phase 2: Peripheral validation — screen, touch, speaker, mic, Wi-Fi, camera debug
- Phase 3: Background vision — face detection with eye gaze (camera hidden behind expression UI)
- Phase 4: XiaoZhi AI — double-tap to enter listening, expression linkage during AI reply
- Phase 5: Servo tracking — PCA9685 + 2-axis servos synchronizing with eye gaze
- Phase 6: Pomodoro timer & competition packaging

## Development Environment

- Platform: Arduino framework for ESP32-S3 (M5Stack CoreS3)
- Libraries: M5CoreS3, M5Unified, M5GFX, ESP-WHO, ArduinoJSON
- Tool: PlatformIO (preferred) or Arduino IDE
