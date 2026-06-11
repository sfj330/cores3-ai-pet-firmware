#pragma once

#include <Arduino.h>
#include <M5GFX.h>
#include "config/app_config.h"

enum class AffinityHitZone {
    AFFINITY_HIT_NONE,
    AFFINITY_HIT_BACK
};

class AffinityUI {
public:
    AffinityUI();
    void begin();
    void show();
    void hide();
    void update();
    void markDirty();

    void setState(int value, const char* level, const char* mood, const char* recent);
    AffinityHitZone hitTest(int x, int y) const;

    void setBackPressed() { backPressedUntil_ = millis() + 150; dirty_ = true; }

private:
    void drawBackButton();
    void drawMeter();
    void drawRow(int x, int y, const char* label, const String& value, uint16_t accent);

    M5Canvas canvas_;
    bool spriteReady_ = false;
    bool visible_ = false;
    bool dirty_ = true;
    unsigned long backPressedUntil_ = 0;

    int value_ = MOOD_DEFAULT_VALUE;
    String level_ = "Familiar";
    String mood_ = "Warm";
    String recent_ = "Ready";

    static constexpr int BACK_X = 5;
    static constexpr int BACK_Y = 5;
    static constexpr int BACK_W = 74;
    static constexpr int BACK_H = 26;
};
