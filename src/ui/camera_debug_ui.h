#pragma once

#include <cstdint>
#include <M5GFX.h>
#include "config/app_config.h"

enum class CameraHitZone {
    HIT_NONE,
    HIT_BACK,
    HIT_SHOT
};

enum class CameraViewMode {
    DEBUG,
    AI_VISION
};

class CameraDebugUI {
public:
    CameraDebugUI();
    void begin();
    void show(CameraViewMode mode = CameraViewMode::DEBUG);
    void hide();
    void update();

    void setMode(CameraViewMode mode);
    void setFps(float fps);
    void setCameraReady(bool ready);
    void setLastPhotoPath(const char* path);
    void setSdReady(bool ready);
    void setCaptureStatus(const char* status);
    void setVisionStatus(const char* status);
    void setVisionResult(const char* result);

    CameraHitZone hitTest(int x, int y) const;

    void pushCameraFrame(const uint16_t* data, int w, int h);

    void setBackPressed() { backPressedUntil_ = millis() + 150; }

private:
    void drawOverlay();
    void drawBackButton();
    void drawShotButton();
    void drawAiVisionOverlay();

    M5Canvas canvas_;
    bool spriteReady_ = false;
    bool visible_ = false;
    unsigned long backPressedUntil_ = 0;
    CameraViewMode mode_ = CameraViewMode::DEBUG;
    bool cameraReady_ = false;
    bool sdReady_ = false;
    float currentFps_ = 0.0f;
    String lastPhotoPath_;
    String captureStatus_;
    String visionStatus_;
    String visionResult_;

    static constexpr int BACK_X = 5;
    static constexpr int BACK_Y = 5;
    static constexpr int BACK_W = 74;
    static constexpr int BACK_H = 26;

    static constexpr int SHOT_X = DISPLAY_WIDTH - 50;
    static constexpr int SHOT_Y = 40;
    static constexpr int SHOT_W = 46;
    static constexpr int SHOT_H = DISPLAY_HEIGHT - 80;
};
