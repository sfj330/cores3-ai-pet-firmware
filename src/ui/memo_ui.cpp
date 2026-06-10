#include "ui/memo_ui.h"
#include "ui/ui_theme.h"
#include <M5CoreS3.h>

MemoUI::MemoUI() : canvas_(&M5.Lcd) {}

void MemoUI::begin() {
    canvas_.setPsram(true);
    canvas_.setColorDepth(16);
    spriteReady_ = canvas_.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT) != nullptr;
    if (spriteReady_) {
        canvas_.fillSprite(UiTheme::BG);
    }
}

void MemoUI::show() {
    visible_ = true;
    dirty_ = true;
}

void MemoUI::hide() {
    visible_ = false;
}

void MemoUI::markDirty() {
    dirty_ = true;
}

void MemoUI::setMemos(const MemoDisplayEntry* memos, int count, uint32_t now) {
    int n = count > MAX_MEMO_COUNT ? MAX_MEMO_COUNT : count;
    if (n < 0) n = 0;
    for (int i = 0; i < n; i++) {
        memos_[i] = memos[i];
    }
    memoCount_ = n;
    memoNow_ = now;
    dirty_ = true;
}

MemoHitZone MemoUI::hitTest(int x, int y) const {
    if (x >= BACK_X && x < BACK_X + BACK_W && y >= BACK_Y && y < BACK_Y + BACK_H) {
        return MemoHitZone::MEMO_HIT_BACK;
    }
    return MemoHitZone::MEMO_HIT_NONE;
}

void MemoUI::update() {
    if (!visible_ || !spriteReady_) return;
    if (millis() < backPressedUntil_) dirty_ = true;
    if (!dirty_) return;

    canvas_.fillSprite(UiTheme::BG);
    UiTheme::drawTitle(canvas_, "Memos", "AI memos & reminders", UiTheme::AMBER);

    if (memoCount_ == 0) {
        canvas_.setTextDatum(MC_DATUM);
        canvas_.setTextSize(1);
        canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::BG);
        canvas_.drawString("No memos yet", DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
        canvas_.setTextDatum(TL_DATUM);
    } else {
        for (int i = 0; i < memoCount_; i++) {
            int y = ROW_START_Y + i * ROW_H;
            drawRow(y, i, memos_[i]);
        }
    }

    drawBackButton();
    canvas_.pushSprite(0, 0);
    dirty_ = false;
}

void MemoUI::drawRow(int y, int index, const MemoDisplayEntry& entry) {
    canvas_.fillRoundRect(8, y, 304, 20, 5, UiTheme::PANEL);

    uint16_t accentColor = entry.remindAt > 0 ? UiTheme::AMBER : UiTheme::TEXT_DIM;
    canvas_.drawFastVLine(16, y + 4, 12, accentColor);

    canvas_.setTextDatum(TL_DATUM);
    canvas_.setTextSize(1);

    // Index prefix "N."
    canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::PANEL);
    canvas_.setCursor(26, y + 5);
    canvas_.printf("%d.", index + 1);

    // Memo text (truncated to fit)
    String displayText = entry.text;
    if (displayText.length() > 28) {
        displayText = displayText.substring(0, 25) + "...";
    }
    canvas_.setTextColor(UiTheme::TEXT, UiTheme::PANEL);
    canvas_.setCursor(50, y + 5);
    canvas_.print(displayText);

    // Reminder time indicator
    if (entry.remindAt > 0 && memoNow_ > 0) {
        char buf[8] = {0};
        formatRemaining(entry.remindAt, memoNow_, buf, sizeof(buf));
        canvas_.setTextColor(UiTheme::AMBER, UiTheme::PANEL);
        canvas_.setTextDatum(TR_DATUM);
        canvas_.drawString(buf, 304, y + 5);
        canvas_.setTextDatum(TL_DATUM);
    }
}

void MemoUI::drawBackButton() {
    UiTheme::drawBackButton(canvas_, BACK_X, BACK_Y, BACK_W, BACK_H, millis() < backPressedUntil_);
}

void MemoUI::formatRemaining(uint32_t remindAt, uint32_t now, char* buf, int bufLen) {
    int32_t diff = (int32_t)(remindAt - now);
    if (diff < 0) diff = 0;
    if (diff < 3600) {
        snprintf(buf, bufLen, "%dm", diff / 60);
    } else if (diff < 86400) {
        snprintf(buf, bufLen, "%dh", diff / 3600);
    } else {
        snprintf(buf, bufLen, "%dd", diff / 86400);
    }
}
