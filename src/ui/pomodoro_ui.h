#pragma once

#include <cstdint>
#include <M5GFX.h>
#include "config/app_config.h"
#include "vision/imu_orientation.h"

enum class PomoHitZone {
    POMO_HIT_NONE,
    POMO_HIT_BACK,
    POMO_HIT_START,
    POMO_HIT_RESET
};

enum class PomodoroState {
    IDLE,
    RUNNING,
    RINGING
};

struct PomoPreset {
    const char* label;
    unsigned long durationMs;
    PomoOrientation orientation;
};

class PomodoroUI {
public:
    PomodoroUI();
    void begin();
    void show();
    void hide();
    void update();

    void selectPreset(int index);
    void setOrientation(PomoOrientation o);

    void togglePause();
    void reset();

    PomodoroState getState() const;
    bool isVisible() const;
    int getActivePreset() const;

    PomoHitZone hitTest(int x, int y) const;
    void markDirty();
    void restoreDisplayRotation();

private:
    bool ensureSprite();
    void applyDisplayRotation(PomoOrientation o);
    uint8_t rotationForOrientation(PomoOrientation o) const;
    void drawTimer();
    void drawProgressRing();
    void drawPresetSelector();
    void drawControls();
    void drawBackButton();
    void drawNotification();

    M5Canvas canvas_;
    bool spriteReady_ = false;
    bool visible_ = false;
    PomodoroState state_ = PomodoroState::IDLE;

    static constexpr int PRESET_COUNT = 4;
    PomoPreset presets_[PRESET_COUNT] = {
        {"Focus 25m", 25 * 60 * 1000UL, PomoOrientation::TOP},
        {"Short 5m",  5 * 60 * 1000UL,  PomoOrientation::RIGHT},
        {"Long 15m",  15 * 60 * 1000UL, PomoOrientation::BOTTOM},
        {"Deep 50m",  50 * 60 * 1000UL, PomoOrientation::LEFT}
    };

    int activePreset_ = 0;
    unsigned long elapsed_ = 0;
    unsigned long lastTick_ = 0;
    unsigned long ringStart_ = 0;
    bool paused_ = false;
    bool dirty_ = true;
    unsigned long lastDrawTime_ = 0;
    PomoOrientation currentOrientation_ = PomoOrientation::FLAT;
    uint8_t baseRotation_ = 1;
    uint8_t appliedRotation_ = 1;

    static constexpr int BACK_X = 5;
    static constexpr int BACK_Y = 5;
    static constexpr int BACK_W = 80;
    static constexpr int BACK_H = 28;

    static constexpr int BTN_Y = 190;
    static constexpr int BTN_W = 80;
    static constexpr int BTN_H = 28;

    static constexpr unsigned long REDRAW_INTERVAL_MS = 250;
};
