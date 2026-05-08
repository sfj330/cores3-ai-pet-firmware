#pragma once

#include <cstdint>
#include <M5GFX.h>
#include "config/app_config.h"

enum class CameraHitZone {
    HIT_NONE,
    HIT_BACK,
    HIT_SHOT
};

class CameraDebugUI {
public:
    CameraDebugUI();
    void begin();
    void show();
    void hide();
    void update();

    void setFps(float fps);
    void setCameraReady(bool ready);
    void setLastPhotoPath(const char* path);
    void setSdReady(bool ready);
    void setCaptureStatus(const char* status);

    CameraHitZone hitTest(int x, int y) const;

    void pushCameraFrame(const uint16_t* data, int w, int h);

private:
    void drawOverlay();
    void drawBackButton();
    void drawShotButton();

    M5Canvas canvas_;
    bool spriteReady_ = false;
    bool visible_ = false;
    bool cameraReady_ = false;
    bool sdReady_ = false;
    float currentFps_ = 0.0f;
    String lastPhotoPath_;
    String captureStatus_;

    static constexpr int BACK_X = DISPLAY_WIDTH - 60;
    static constexpr int BACK_Y = 2;
    static constexpr int BACK_W = 56;
    static constexpr int BACK_H = 24;

    static constexpr int SHOT_X = DISPLAY_WIDTH - 50;
    static constexpr int SHOT_Y = 40;
    static constexpr int SHOT_W = 46;
    static constexpr int SHOT_H = DISPLAY_HEIGHT - 80;
};
