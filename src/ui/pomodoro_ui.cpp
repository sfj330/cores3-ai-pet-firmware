#include "pomodoro_ui.h"
#include "config/app_config.h"
#include <M5CoreS3.h>
#include <cmath>

PomodoroUI::PomodoroUI() : canvas_(&M5.Lcd) {}

bool PomodoroUI::ensureSprite() {
    int w = M5.Lcd.width();
    int h = M5.Lcd.height();
    if (spriteReady_ && canvas_.width() == w && canvas_.height() == h) {
        return true;
    }

    if (spriteReady_) {
        canvas_.deleteSprite();
        spriteReady_ = false;
    }

    canvas_.setPsram(true);
    canvas_.setColorDepth(16);
    spriteReady_ = canvas_.createSprite(w, h) != nullptr;
    if (spriteReady_) {
        canvas_.fillSprite(TFT_BLACK);
    }
    dirty_ = true;
    return spriteReady_;
}

void PomodoroUI::begin() {
    baseRotation_ = M5.Lcd.getRotation() & 3;
    appliedRotation_ = baseRotation_;
    ensureSprite();
}

void PomodoroUI::show() {
    visible_ = true;
    baseRotation_ = M5.Lcd.getRotation() & 3;
    appliedRotation_ = baseRotation_;
    currentOrientation_ = PomoOrientation::UNKNOWN;
    ensureSprite();
    reset();
    dirty_ = true;
}

void PomodoroUI::hide() {
    visible_ = false;
    restoreDisplayRotation();
}

void PomodoroUI::restoreDisplayRotation() {
    if ((M5.Lcd.getRotation() & 3) != baseRotation_) {
        M5.Lcd.setRotation(baseRotation_);
    }
    appliedRotation_ = baseRotation_;
}

bool PomodoroUI::isVisible() const {
    return visible_;
}

PomodoroState PomodoroUI::getState() const {
    return state_;
}

int PomodoroUI::getActivePreset() const {
    return activePreset_;
}

void PomodoroUI::selectPreset(int index) {
    if (index >= 0 && index < PRESET_COUNT && index != activePreset_) {
        activePreset_ = index;
        if (state_ == PomodoroState::RUNNING) {
            elapsed_ = 0;
            lastTick_ = millis();
        }
        dirty_ = true;
    }
}

uint8_t PomodoroUI::rotationForOrientation(PomoOrientation o) const {
    uint8_t offset = 0;
    switch (o) {
        case PomoOrientation::RIGHT: offset = 1; break;
        case PomoOrientation::BOTTOM: offset = 2; break;
        case PomoOrientation::LEFT: offset = 3; break;
        case PomoOrientation::TOP:
        default: offset = 0; break;
    }
    return (baseRotation_ + offset) & 3;
}

void PomodoroUI::applyDisplayRotation(PomoOrientation o) {
    uint8_t nextRotation = rotationForOrientation(o);
    if (nextRotation != appliedRotation_) {
        M5.Lcd.setRotation(nextRotation);
        appliedRotation_ = nextRotation;
    }
    ensureSprite();
    dirty_ = true;
}

void PomodoroUI::setOrientation(PomoOrientation o) {
    if (o == PomoOrientation::UNKNOWN || o == PomoOrientation::FLAT || o == currentOrientation_) {
        return;
    }

    currentOrientation_ = o;
    applyDisplayRotation(o);

    for (int i = 0; i < PRESET_COUNT; ++i) {
        if (presets_[i].orientation == o) {
            selectPreset(i);
            break;
        }
    }
}

void PomodoroUI::togglePause() {
    if (state_ == PomodoroState::IDLE) {
        state_ = PomodoroState::RUNNING;
        elapsed_ = 0;
        lastTick_ = millis();
        paused_ = false;
    } else if (state_ == PomodoroState::RINGING) {
        reset();
    } else {
        paused_ = !paused_;
        if (!paused_) {
            lastTick_ = millis();
        }
    }
    dirty_ = true;
}

void PomodoroUI::reset() {
    state_ = PomodoroState::IDLE;
    elapsed_ = 0;
    paused_ = false;
    dirty_ = true;
}

void PomodoroUI::markDirty() {
    dirty_ = true;
}

PomoHitZone PomodoroUI::hitTest(int x, int y) const {
    int w = M5.Lcd.width();
    int h = M5.Lcd.height();
    int btnY = h - 42;
    int startX = w / 4 - BTN_W / 2;
    int resetX = (w * 3) / 4 - BTN_W / 2;
    if (startX < 8) startX = 8;
    if (resetX + BTN_W > w - 8) resetX = w - BTN_W - 8;

    if (x >= BACK_X && x < BACK_X + BACK_W && y >= BACK_Y && y < BACK_Y + BACK_H) {
        return PomoHitZone::POMO_HIT_BACK;
    }
    if (y >= btnY && y < btnY + BTN_H) {
        if (x >= startX && x < startX + BTN_W) return PomoHitZone::POMO_HIT_START;
        if (x >= resetX && x < resetX + BTN_W) return PomoHitZone::POMO_HIT_RESET;
    }
    return PomoHitZone::POMO_HIT_NONE;
}

void PomodoroUI::update() {
    if (!visible_) return;
    if (!ensureSprite()) return;

    unsigned long now = millis();

    if (!paused_ && state_ == PomodoroState::RUNNING) {
        elapsed_ += now - lastTick_;
        lastTick_ = now;

        unsigned long duration = presets_[activePreset_].durationMs;
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
    drawPresetSelector();
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

void PomodoroUI::drawPresetSelector() {
    int w = canvas_.width();
    int boxGap = 6;
    int boxW = (w - 20 - boxGap * (PRESET_COUNT - 1)) / PRESET_COUNT;
    if (boxW < 48) boxW = 48;
    int boxH = 28;
    int startX = 10;
    int y = 38;

    canvas_.setTextSize(1);
    canvas_.setTextDatum(TL_DATUM);
    for (int i = 0; i < PRESET_COUNT; ++i) {
        int x = startX + i * (boxW + boxGap);
        uint16_t bgColor = (i == activePreset_) ? TFT_WHITE : TFT_DARKGREY;
        uint16_t txtColor = (i == activePreset_) ? TFT_BLACK : TFT_WHITE;
        canvas_.fillRoundRect(x, y, boxW, boxH, 4, bgColor);
        canvas_.setTextColor(txtColor, bgColor);
        canvas_.setCursor(x + 4, y + 10);
        canvas_.print(presets_[i].label);
    }
}

void PomodoroUI::drawTimer() {
    int w = canvas_.width();
    int h = canvas_.height();
    unsigned long duration = presets_[activePreset_].durationMs;
    unsigned long remaining = elapsed_ >= duration ? 0 : duration - elapsed_;
    int minutes = (remaining / 60000) % 60;
    int seconds = (remaining / 1000) % 60;
    int timerY = h / 2 - 52;

    canvas_.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas_.setTextSize(4);
    canvas_.setCursor(w / 2 - 70, timerY);
    canvas_.printf("%02d:%02d", minutes, seconds);

    canvas_.setTextSize(1);
    canvas_.setCursor(w / 2 - 30, timerY + 45);
    switch (state_) {
        case PomodoroState::RUNNING: canvas_.print(paused_ ? "PAUSED" : "RUNNING"); break;
        case PomodoroState::RINGING: canvas_.print("TIME'S UP!"); break;
        default: canvas_.print("READY"); break;
    }

    canvas_.setCursor(w / 2 - 44, timerY + 60);
    canvas_.print(ImuOrientation::orientationName(currentOrientation_));
}

void PomodoroUI::drawProgressRing() {
    int w = canvas_.width();
    int h = canvas_.height();
    int cx = w / 2;
    int cy = h / 2 + 14;
    int r = 38;

    unsigned long duration = presets_[activePreset_].durationMs;
    float progress = (duration > 0) ? (float)elapsed_ / duration : 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    canvas_.drawCircle(cx, cy, r, TFT_DARKGREY);

    int segments = 36;
    for (int i = 0; i < segments; ++i) {
        float angle = -PI / 2 + (i / (float)segments) * 2 * PI;
        float endAngle = -PI / 2 + ((i + 1) / (float)segments) * 2 * PI;
        if ((float)i / segments <= progress) {
            int x1 = cx + (int)((r - 2) * cos(angle));
            int y1 = cy + (int)((r - 2) * sin(angle));
            int x2 = cx + (int)((r - 2) * cos(endAngle));
            int y2 = cy + (int)((r - 2) * sin(endAngle));
            canvas_.drawLine(x1, y1, x2, y2, TFT_ORANGE);
        }
    }
}

void PomodoroUI::drawControls() {
    int w = canvas_.width();
    int h = canvas_.height();
    int btnY = h - 42;
    int startX = w / 4 - BTN_W / 2;
    int resetX = (w * 3) / 4 - BTN_W / 2;
    if (startX < 8) startX = 8;
    if (resetX + BTN_W > w - 8) resetX = w - BTN_W - 8;

    canvas_.setTextSize(1);
    canvas_.setTextDatum(TL_DATUM);

    uint16_t btnColor = (state_ == PomodoroState::IDLE || paused_) ? TFT_GREEN : TFT_ORANGE;
    const char* label = (state_ == PomodoroState::IDLE || paused_) ? "Start" : "Pause";
    canvas_.fillRoundRect(startX, btnY, BTN_W, BTN_H, 4, btnColor);
    canvas_.setTextColor(TFT_WHITE, btnColor);
    canvas_.setCursor(startX + 20, btnY + 8);
    canvas_.print(label);

    canvas_.fillRoundRect(resetX, btnY, BTN_W, BTN_H, 4, TFT_RED);
    canvas_.setTextColor(TFT_WHITE, TFT_RED);
    canvas_.setCursor(resetX + 16, btnY + 8);
    canvas_.print("Reset");
}

void PomodoroUI::drawBackButton() {
    canvas_.fillRoundRect(BACK_X, BACK_Y, BACK_W, BACK_H, 6, TFT_DARKGREY);
    canvas_.setTextColor(TFT_WHITE, TFT_DARKGREY);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(MC_DATUM);
    canvas_.setCursor(BACK_X + BACK_W / 2 - 16, BACK_Y + BACK_H / 2 - 4);
    canvas_.print("Back");
    canvas_.setTextDatum(TL_DATUM);
}

void PomodoroUI::drawNotification() {
    int w = canvas_.width();
    int h = canvas_.height();
    canvas_.fillRect(w / 2 - 90, h / 2 - 30, 180, 60, TFT_GREEN);
    canvas_.setTextColor(TFT_WHITE, TFT_GREEN);
    canvas_.setTextSize(3);
    canvas_.setCursor(w / 2 - 70, h / 2 - 15);
    canvas_.print("TIME'S UP!");
}
