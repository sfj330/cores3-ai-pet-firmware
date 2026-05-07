#pragma once

#include <cstdint>
#include <M5GFX.h>
#include "config/app_config.h"

class FaceUI {
public:
    FaceUI();
    bool begin();
    void update();

    // Eye gaze offset (normalized -1.0 to 1.0)
    void setGazeOffset(float dx, float dy);

    // Force an expression (overrides state machine)
    void setExpression(int emotion);  // takes FaceEmotion as int for Arduino compat

    void setSpeakingMouthOpen(bool open);
    void wake();

private:
    void drawFace();
    void drawEyes(int centerX, int centerY);
    void drawEyebrows(int centerX, int centerY);
    void drawMouth(int centerX, int centerY);
    void drawCheeks(int centerX, int centerY);

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

    int currentEmotion_ = 0; // FaceEmotion

    // Speaking animation
    bool speakingMouthOpen_ = false;
    unsigned long lastSpeakAnimTime_ = 0;
    float mouthOpenAmount_ = 0.0f;

    // Sleeping dim state
    bool sleeping_ = false;
    int brightness_ = 255;
};
