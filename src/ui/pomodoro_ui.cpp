#include "pomodoro_ui.h"
#include "config/app_config.h"
#include <M5CoreS3.h>
#include <cmath>

PomodoroUI::PomodoroUI() : canvas_(&M5.Lcd) {}

void PomodoroUI::begin() {
    canvas_.setPsram(true);
    canvas_.setColorDepth(16);
    spriteReady_ = canvas_.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT) != nullptr;
    if (spriteReady_) {
        canvas_.fillSprite(TFT_BLACK);
    }
}

void PomodoroUI::show() {
    visible_ = true;
    reset();
    dirty_ = true;
}

void PomodoroUI::hide() {
    visible_ = false;
}

bool PomodoroUI::isVisible() const {
    return visible_;
}

PomodoroState PomodoroUI::getState() const {
    return state_;
}

void PomodoroUI::startFocus() {
    state_ = PomodoroState::FOCUS;
    elapsed_ = 0;
    lastTick_ = millis();
    paused_ = false;
    dirty_ = true;
}

void PomodoroUI::startBreak() {
    state_ = PomodoroState::BREAK;
    elapsed_ = 0;
    lastTick_ = millis();
    paused_ = false;
    dirty_ = true;
}

void PomodoroUI::pause() {
    paused_ = true;
    dirty_ = true;
}

void PomodoroUI::reset() {
    state_ = PomodoroState::IDLE;
    elapsed_ = 0;
    paused_ = false;
    dirty_ = true;
}

void PomodoroUI::togglePause() {
    if (state_ == PomodoroState::IDLE) {
        startFocus();
    } else if (state_ == PomodoroState::RINGING) {
        reset();
    } else {
        paused_ = !paused_;
        if (!paused_) {
            lastTick_ = millis();
        }
        dirty_ = true;
    }
}

void PomodoroUI::markDirty() {
    dirty_ = true;
}

PomoHitZone PomodoroUI::hitTest(int x, int y) const {
    if (x >= BACK_X && x < BACK_X + BACK_W && y >= BACK_Y && y < BACK_Y + BACK_H) {
        return PomoHitZone::POMO_HIT_BACK;
    }
    if (y >= BTN_Y && y < BTN_Y + BTN_H) {
        if (x >= BTN_START_X && x < BTN_START_X + BTN_W) return PomoHitZone::POMO_HIT_START;
        if (x >= BTN_RESET_X && x < BTN_RESET_X + BTN_W) return PomoHitZone::POMO_HIT_RESET;
        if (x >= BTN_SKIP_X && x < BTN_SKIP_X + BTN_W) return PomoHitZone::POMO_HIT_SKIP;
    }
    return PomoHitZone::POMO_HIT_NONE;
}

void PomodoroUI::update() {
    if (!visible_ || !spriteReady_) return;

    unsigned long now = millis();

    if (!paused_ && (state_ == PomodoroState::FOCUS || state_ == PomodoroState::BREAK)) {
        elapsed_ += now - lastTick_;
        lastTick_ = now;

        unsigned long duration = (state_ == PomodoroState::FOCUS) ? focusDuration_ : breakDuration_;

        if (elapsed_ >= duration) {
            state_ = PomodoroState::RINGING;
            ringStart_ = now;
            elapsed_ = duration;
            dirty_ = true;
        }
    }

    if (state_ == PomodoroState::RINGING && (now - ringStart_ > POMODORO_RING_MS)) {
        reset();
    }

    if (!dirty_ && (now - lastDrawTime_ < REDRAW_INTERVAL_MS)) return;

    canvas_.fillSprite(TFT_BLACK);
    drawTimer();
    drawProgressRing();
    drawControls();
    drawBackButton();

    if (state_ == PomodoroState::RINGING) {
        drawNotification();
    }

    canvas_.pushSprite(0, 0);
    lastDrawTime_ = now;
    dirty_ = false;
}

void PomodoroUI::drawTimer() {
    unsigned long duration = 0;
    switch (state_) {
        case PomodoroState::FOCUS: duration = focusDuration_; break;
        case PomodoroState::BREAK: duration = breakDuration_; break;
        default: duration = focusDuration_; break;
    }

    unsigned long remaining = duration - elapsed_;
    int minutes = (remaining / 60000) % 60;
    int seconds = (remaining / 1000) % 60;

    canvas_.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas_.setTextSize(4);
    canvas_.setCursor(DISPLAY_WIDTH / 2 - 70, 30);
    canvas_.printf("%02d:%02d", minutes, seconds);

    canvas_.setTextSize(1);
    canvas_.setCursor(DISPLAY_WIDTH / 2 - 30, 80);
    switch (state_) {
        case PomodoroState::FOCUS: canvas_.print("FOCUS"); break;
        case PomodoroState::BREAK: canvas_.print("BREAK"); break;
        case PomodoroState::RINGING: canvas_.print("TIME'S UP!"); break;
        default: canvas_.print("READY"); break;
    }
}

void PomodoroUI::drawProgressRing() {
    int cx = DISPLAY_WIDTH / 2;
    int cy = 60;
    int r = 55;

    unsigned long duration = 0;
    switch (state_) {
        case PomodoroState::FOCUS: duration = focusDuration_; break;
        case PomodoroState::BREAK: duration = breakDuration_; break;
        default: duration = focusDuration_; break;
    }

    float progress = (duration > 0) ? (float)elapsed_ / duration : 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    canvas_.drawCircle(cx, cy, r, TFT_DARKGREY);

    int segments = 36;
    uint16_t progressColor = (state_ == PomodoroState::BREAK) ? TFT_GREEN : TFT_ORANGE;
    for (int i = 0; i < segments; ++i) {
        float angle = -PI / 2 + (i / (float)segments) * 2 * PI;
        float endAngle = -PI / 2 + ((i + 1) / (float)segments) * 2 * PI;
        if ((float)i / segments <= progress) {
            int x1 = cx + (int)((r - 2) * cos(angle));
            int y1 = cy + (int)((r - 2) * sin(angle));
            int x2 = cx + (int)((r - 2) * cos(endAngle));
            int y2 = cy + (int)((r - 2) * sin(endAngle));
            canvas_.drawLine(x1, y1, x2, y2, progressColor);
        }
    }
}

void PomodoroUI::drawControls() {
    canvas_.setTextSize(1);

    int x = BTN_START_X;
    uint16_t btnColor = (state_ == PomodoroState::IDLE || paused_) ? TFT_GREEN : TFT_ORANGE;
    const char* label = (state_ == PomodoroState::IDLE || paused_) ? "Start" : "Pause";
    canvas_.fillRoundRect(x, BTN_Y, BTN_W, BTN_H, 4, btnColor);
    canvas_.setTextColor(TFT_WHITE);
    canvas_.setCursor(x + 10, BTN_Y + 8);
    canvas_.print(label);

    x = BTN_RESET_X;
    canvas_.fillRoundRect(x, BTN_Y, BTN_W, BTN_H, 4, TFT_RED);
    canvas_.setTextColor(TFT_WHITE);
    canvas_.setCursor(x + 12, BTN_Y + 8);
    canvas_.print("Reset");

    x = BTN_SKIP_X;
    canvas_.fillRoundRect(x, BTN_Y, BTN_W, BTN_H, 4, TFT_BLUE);
    canvas_.setTextColor(TFT_WHITE);
    canvas_.setCursor(x + 8, BTN_Y + 8);
    canvas_.print("Skip");
}

void PomodoroUI::drawBackButton() {
    canvas_.fillRoundRect(BACK_X, BACK_Y, BACK_W, BACK_H, 6, TFT_DARKGREY);
    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(MC_DATUM);
    canvas_.setCursor(BACK_X + BACK_W / 2 - 16, BACK_Y + BACK_H / 2 - 4);
    canvas_.print("Back");
    canvas_.setTextDatum(TL_DATUM);
}

void PomodoroUI::drawNotification() {
    canvas_.fillRect(DISPLAY_WIDTH / 2 - 90, DISPLAY_HEIGHT / 2 - 30, 180, 60, TFT_GREEN);
    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(3);
    canvas_.setCursor(DISPLAY_WIDTH / 2 - 70, DISPLAY_HEIGHT / 2 - 15);
    canvas_.print("TIME'S UP!");
}
