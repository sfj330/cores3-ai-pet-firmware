#pragma once

#include <Arduino.h>
#include <M5GFX.h>
#include "config/app_config.h"

enum class ServoTestHitZone {
    SERVO_TEST_HIT_NONE,
    SERVO_TEST_HIT_BACK,
    SERVO_TEST_HIT_LEFT,
    SERVO_TEST_HIT_RIGHT,
    SERVO_TEST_HIT_UP,
    SERVO_TEST_HIT_DOWN,
    SERVO_TEST_HIT_CENTER,
    SERVO_TEST_HIT_NOD,
    SERVO_TEST_HIT_SHAKE,
    SERVO_TEST_HIT_DANCE,
    SERVO_TEST_HIT_RELEASE,
    SERVO_TEST_HIT_SCAN
};

class ServoTestUI {
public:
    ServoTestUI();
    void begin();
    void show();
    void hide();
    void update();

    void setServoState(bool ready, bool released, float pan, float tilt, const String& status);
    ServoTestHitZone hitTest(int x, int y) const;

    void setBackPressed() { backPressedUntil_ = millis() + 150; dirty_ = true; }

private:
    void drawBackButton();
    void drawStatusPanel();
    void drawDirectionPad();
    void drawActionButtons();

    M5Canvas canvas_;
    bool spriteReady_ = false;
    bool visible_ = false;
    bool dirty_ = true;
    unsigned long backPressedUntil_ = 0;

    bool servoReady_ = false;
    bool servoReleased_ = true;
    float panAngle_ = SERVO_PAN_CENTER_DEG;
    float tiltAngle_ = SERVO_TILT_CENTER_DEG;
    String servoStatus_ = "Not initialized";

    static constexpr int BACK_X = 5;
    static constexpr int BACK_Y = 5;
    static constexpr int BACK_W = 74;
    static constexpr int BACK_H = 26;

    // Direction pad center
    static constexpr int DPAD_CX = 100;
    static constexpr int DPAD_CY = 150;
    static constexpr int DPAD_BTN = 48;
    static constexpr int DPAD_GAP = 4;

    // Action buttons row
    static constexpr int ACT_Y = 215;
    static constexpr int ACT_W = 44;
    static constexpr int ACT_H = 30;
    static constexpr int ACT_GAP = 5;
};
