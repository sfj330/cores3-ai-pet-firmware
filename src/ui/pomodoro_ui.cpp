#include "pomodoro_ui.h"
#include "config/app_config.h"
#include <M5CoreS3.h>
#include <cmath>

PomodoroUI::PomodoroUI() = default;

void PomodoroUI::begin() {
    // No special init needed
}

void PomodoroUI::show() {
    visible_ = true;
    reset();
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
}

void PomodoroUI::startBreak() {
    state_ = PomodoroState::BREAK;
    elapsed_ = 0;
    lastTick_ = millis();
    paused_ = false;
}

void PomodoroUI::pause() {
    paused_ = true;
}

void PomodoroUI::reset() {
    state_ = PomodoroState::IDLE;
    elapsed_ = 0;
    paused_ = false;
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
    }
}

void PomodoroUI::update() {
    if (!visible_) return;

    unsigned long now = millis();

    // Update timer
    if (!paused_ && (state_ == PomodoroState::FOCUS || state_ == PomodoroState::BREAK)) {
        elapsed_ += now - lastTick_;
        lastTick_ = now;

        unsigned long duration = (state_ == PomodoroState::FOCUS) ? focusDuration_ : breakDuration_;

        if (elapsed_ >= duration) {
            state_ = PomodoroState::RINGING;
            ringStart_ = now;
            elapsed_ = duration;
        }
    }

    // Ringing timeout
    if (state_ == PomodoroState::RINGING && (now - ringStart_ > POMODORO_RING_MS)) {
        // Auto-switch
        // If was focus, go to break. If was break, go back to idle.
        // We determine context by what just finished
        reset();
    }

    M5.Lcd.fillScreen(TFT_BLACK);
    drawTimer();
    drawProgressRing();
    drawControls();
    drawBackButton();

    if (state_ == PomodoroState::RINGING) {
        drawNotification();
    }
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

    // Large timer text
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.setTextSize(4);
    M5.Lcd.setCursor(DISPLAY_WIDTH / 2 - 70, 30);
    M5.Lcd.printf("%02d:%02d", minutes, seconds);

    // State label
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(DISPLAY_WIDTH / 2 - 30, 80);
    switch (state_) {
        case PomodoroState::FOCUS: M5.Lcd.print("FOCUS"); break;
        case PomodoroState::BREAK: M5.Lcd.print("BREAK"); break;
        case PomodoroState::RINGING: M5.Lcd.print("TIME'S UP!"); break;
        default: M5.Lcd.print("READY"); break;
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

    // Draw background arc
    M5.Lcd.drawCircle(cx, cy, r, TFT_DARKGREY);

    // Draw progress arc (simple: draw segments)
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
            M5.Lcd.drawLine(x1, y1, x2, y2, progressColor);
        }
    }
}

void PomodoroUI::drawControls() {
    int btnY = 130;
    int btnW = 60;
    int btnH = 30;
    int spacing = 15;
    int totalW = btnW * 3 + spacing * 2;
    int startX = DISPLAY_WIDTH / 2 - totalW / 2;

    M5.Lcd.setTextSize(1);

    // Start / Pause button
    int x = startX;
    uint16_t btnColor = (state_ == PomodoroState::IDLE || paused_) ? TFT_GREEN : TFT_ORANGE;
    const char* label = (state_ == PomodoroState::IDLE || paused_) ? "Start" : "Pause";
    M5.Lcd.fillRoundRect(x, btnY, btnW, btnH, 4, btnColor);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setCursor(x + 10, btnY + 8);
    M5.Lcd.print(label);

    // Reset button
    x += btnW + spacing;
    M5.Lcd.fillRoundRect(x, btnY, btnW, btnH, 4, TFT_RED);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setCursor(x + 12, btnY + 8);
    M5.Lcd.print("Reset");

    // Skip / Switch button
    x += btnW + spacing;
    M5.Lcd.fillRoundRect(x, btnY, btnW, btnH, 4, TFT_BLUE);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setCursor(x + 8, btnY + 8);
    M5.Lcd.print("Skip");
}

void PomodoroUI::drawBackButton() {
    // Back arrow
    M5.Lcd.fillTriangle(20, 20, 30, 10, 30, 30, TFT_WHITE);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(35, 16);
    M5.Lcd.print("Back");
}

void PomodoroUI::drawNotification() {
    // Non-blocking notification overlay when time is up
    M5.Lcd.fillRect(DISPLAY_WIDTH / 2 - 90, DISPLAY_HEIGHT / 2 - 30, 180, 60, TFT_GREEN);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(DISPLAY_WIDTH / 2 - 70, DISPLAY_HEIGHT / 2 - 15);
    M5.Lcd.print("TIME'S UP!");
}
