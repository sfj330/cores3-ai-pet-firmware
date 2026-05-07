#pragma once

#include <cstdint>

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

private:
    void drawTimer();
    void drawProgressRing();
    void drawControls();
    void drawBackButton();
    void drawNotification();

    bool visible_ = false;
    PomodoroState state_ = PomodoroState::IDLE;

    unsigned long focusDuration_ = 25 * 60 * 1000UL;  // 25 min
    unsigned long breakDuration_ = 5 * 60 * 1000UL;   // 5 min
    unsigned long elapsed_ = 0;
    unsigned long lastTick_ = 0;
    unsigned long ringStart_ = 0;

    bool paused_ = false;
};
