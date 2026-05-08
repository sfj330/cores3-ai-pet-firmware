#pragma once

#include <cstdint>
#include <Arduino.h>
#include <M5GFX.h>
#include "config/app_config.h"
#include "app/app_state.h"

class FaceUI {
public:
    FaceUI();
    bool begin();
    void update();

    void setGazeOffset(float dx, float dy);
    void setExpression(FaceEmotion emotion);
    void setSpeakingMouthOpen(bool open);
    void setAiOverlay(bool visible, const char* status, const char* detail, bool wifiConnected);
    void setVisionStatus(const char* status);
    void wake();

    void setTemporaryGaze(float dx, float dy, unsigned long durationMs);

private:
    void drawFace();
    void drawEyes(int centerX, int centerY);
    void drawEyebrows(int centerX, int centerY);
    void drawMouth(int centerX, int centerY);
    void drawCheeks(int centerX, int centerY);
    void drawStatusOverlay();

    void updateBlink();
    void updateGaze();
    void updateSpeakingAnimation();

    M5Canvas canvas_;
    bool spriteReady_ = false;

    bool isBlinking_ = false;
    unsigned long blinkStartTime_ = 0;
    unsigned long nextBlinkTime_ = 3000;

    float gazeX_ = 0.0f;
    float gazeY_ = 0.0f;
    float targetGazeX_ = 0.0f;
    float targetGazeY_ = 0.0f;

    float tempGazeX_ = 0.0f;
    float tempGazeY_ = 0.0f;
    bool tempGazeActive_ = false;
    unsigned long tempGazeEndTime_ = 0;

    FaceEmotion currentEmotion_ = FaceEmotion::NORMAL;

    bool speakingMouthOpen_ = false;
    unsigned long lastSpeakAnimTime_ = 0;
    float mouthOpenAmount_ = 0.0f;

    bool aiOverlayVisible_ = false;
    bool aiWifiConnected_ = false;
    String aiStatus_;
    String aiDetail_;
    String visionStatus_;

    bool sleeping_ = false;
    int brightness_ = 255;
};
