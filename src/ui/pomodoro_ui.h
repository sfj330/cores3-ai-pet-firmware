#pragma once

#include <cstdint>
#include <M5GFX.h>
#include "config/app_config.h"

enum class PomoHitZone {
    POMO_HIT_NONE,
    POMO_HIT_BACK,
    POMO_HIT_START,
    POMO_HIT_RESET,
    POMO_HIT_SKIP
};

enum class PomodoroState {
    IDLE,
    FOCUS,
    BREAK,
    RINGING
};

class PomodoroUI {
public:
    PomodoroUI();
    void begin();
    void show();
    void hide();
    void update();

    void startFocus();
    void startBreak();
    void pause();
    void reset();
    void togglePause();

    PomodoroState getState() const;
    bool isVisible() const;

    PomoHitZone hitTest(int x, int y) const;
    void markDirty();

private:
    void drawTimer();
    void drawProgressRing();
    void drawControls();
    void drawBackButton();
    void drawNotification();

    M5Canvas canvas_;
    bool spriteReady_ = false;
    bool visible_ = false;
    PomodoroState state_ = PomodoroState::IDLE;

    unsigned long focusDuration_ = 25 * 60 * 1000UL;
    unsigned long breakDuration_ = 5 * 60 * 1000UL;
    unsigned long elapsed_ = 0;
    unsigned long lastTick_ = 0;
    unsigned long ringStart_ = 0;

    bool paused_ = false;
    bool dirty_ = true;
    unsigned long lastDrawTime_ = 0;

    static constexpr int BACK_X = 5;
    static constexpr int BACK_Y = 5;
    static constexpr int BACK_W = 80;
    static constexpr int BACK_H = 28;

    static constexpr int BTN_Y = 130;
    static constexpr int BTN_W = 60;
    static constexpr int BTN_H = 30;
    static constexpr int BTN_SPACING = 15;
    static constexpr int BTN_START_X = DISPLAY_WIDTH / 2 - (BTN_W * 3 + BTN_SPACING * 2) / 2;
    static constexpr int BTN_RESET_X = BTN_START_X + BTN_W + BTN_SPACING;
    static constexpr int BTN_SKIP_X = BTN_RESET_X + BTN_W + BTN_SPACING;

    static constexpr unsigned long REDRAW_INTERVAL_MS = 250;
};
