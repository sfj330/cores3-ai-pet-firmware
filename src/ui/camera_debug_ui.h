#pragma once

#include <cstdint>

class CameraDebugUI {
public:
    CameraDebugUI();
    void begin();
    void show();
    void hide();
    void update();

    void setFps(float fps);
    void setCameraReady(bool ready);
    void setDetectionOverlay(bool enabled);
    bool getDetectionOverlay() const;

private:
    void drawDebugOverlay();
    void drawBackButton();

    bool visible_ = false;
    bool cameraReady_ = false;
    float currentFps_ = 0.0f;
    bool showDetection_ = false;
    unsigned long lastFrameTime_ = 0;
    int frameCount_ = 0;
};
