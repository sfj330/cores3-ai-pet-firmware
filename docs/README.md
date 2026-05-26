# Learning Guide

Chinese version: [README_CN.md](README_CN.md).

This repository is a complete CoreS3 desktop pet firmware, but it is easier to learn if you read it by layers instead of opening `main.cpp` first.

## Suggested Reading Order

1. Read the root `README.md` to understand what the firmware currently does.
2. Read `src/config/app_config.h` to learn the main hardware pins, timing constants, task priorities, and safety limits.
3. Read `src/app/app_state.h` and `src/main.cpp` state handling to understand page transitions.
4. Read the UI classes under `src/ui/`, starting with `face_ui.*`, `menu_ui.*`, and `camera_debug_ui.*`. Then look at `album_ui.*` if you want a compact example of SD-backed thumbnail browsing.
5. Read `src/vision/camera_manager.*` before modifying camera code, because framebuffer lifetime and CoreS3 internal I2C handling are important.
6. Read `src/ai/xiaozhi_client.*` after you understand the local UI, because voice, WebSocket, Opus, and MCP are the heaviest subsystem.
7. Read `src/network/web_server.*` after Wi-Fi and storage if you want to understand the lightweight LAN control surface, MJPEG preview, local OTA upload, and photo browser.
8. Read `src/servo/servo_motion_controller.*` before changing servo behavior, because direct large servo jumps can cause brownout resets.

## Good First Experiments

- Change one face expression color or mouth shape in `src/ui/face_ui.cpp`.
- Add a short status string to one page without changing task timing.
- Add a new menu subtitle or system status row.
- Extend the Album page subtitle or web control status payload without changing shared resource ownership.
- Add one more status line or warning to the web control page without changing camera or OTA ownership rules.
- Tune a servo motion speed in `src/config/app_config.h`, then test with an external servo power supply.
- Add serial logs around one state transition and watch them at 115200 baud.

## Rules Worth Keeping

- Keep UI rendering in UI classes and push complete frames through `M5Canvas` where possible.
- Never keep a camera framebuffer after releasing it.
- Do not block touch/state callbacks with heavy camera, Wi-Fi, or AI work.
- Treat camera, speaker, microphone, SD, and servos as shared resources.
- If you add a new external control path, update both the local UI flow and the remote entry points consistently.
- If you expose OTA or camera features over the web, document the resource ownership rules clearly so future changes do not accidentally start background capture or unsafe flashing paths.
- Prefer short, state-driven actions over long loops in `loop()`.
- Keep real Wi-Fi credentials in `src/config/wifi_secrets.h`, which is ignored by Git.
