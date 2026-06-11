#include "ui/affinity_ui.h"
#include "ui/ui_theme.h"
#include <M5CoreS3.h>

AffinityUI::AffinityUI() : canvas_(&M5.Lcd) {}

void AffinityUI::begin() {
    canvas_.setPsram(true);
    canvas_.setColorDepth(16);
    spriteReady_ = canvas_.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT) != nullptr;
    if (spriteReady_) {
        canvas_.fillSprite(UiTheme::BG);
    }
}

void AffinityUI::show() {
    visible_ = true;
    dirty_ = true;
}

void AffinityUI::hide() {
    visible_ = false;
}

void AffinityUI::markDirty() {
    dirty_ = true;
}

void AffinityUI::setState(int value, const char* level, const char* mood, const char* recent) {
    String newLevel = level ? String(level) : String();
    String newMood = mood ? String(mood) : String();
    String newRecent = recent ? String(recent) : String();
    if (value != value_ || newLevel != level_ || newMood != mood_ || newRecent != recent_) {
        value_ = value;
        level_ = newLevel;
        mood_ = newMood;
        recent_ = newRecent;
        dirty_ = true;
    }
}

AffinityHitZone AffinityUI::hitTest(int x, int y) const {
    if (x >= BACK_X && x < BACK_X + BACK_W && y >= BACK_Y && y < BACK_Y + BACK_H) {
        return AffinityHitZone::AFFINITY_HIT_BACK;
    }
    return AffinityHitZone::AFFINITY_HIT_NONE;
}

void AffinityUI::update() {
    if (!visible_ || !spriteReady_) return;
    if (millis() < backPressedUntil_) dirty_ = true;
    if (!dirty_) return;

    canvas_.fillSprite(UiTheme::BG);
    UiTheme::drawTitle(canvas_, "Bond", "Companion status", UiTheme::PINK);
    drawMeter();
    drawRow(34, 132, "Lv", level_, UiTheme::CYAN);
    String moodWithIcon = mood_;
    if (mood_ == "Lonely") moodWithIcon = ":(  Lonely";
    else if (mood_ == "Quiet") moodWithIcon = ":|  Quiet";
    else if (mood_ == "Warm") moodWithIcon = ":)  Warm";
    else if (mood_ == "Happy") moodWithIcon = ":D  Happy";
    else if (mood_ == "Lively") moodWithIcon = ";D  Lively";
    drawRow(34, 162, "Mood", moodWithIcon, UiTheme::GREEN);
    drawRow(34, 192, "Recent", recent_, UiTheme::AMBER);
    drawBackButton();
    canvas_.pushSprite(0, 0);
    dirty_ = false;
}

void AffinityUI::drawMeter() {
    int cx = DISPLAY_WIDTH / 2;
    int y = 54;
    int w = 220;
    int h = 42;
    int x = cx - w / 2;
    int fill = map(value_, AFFINITY_MIN_VALUE, AFFINITY_MAX_VALUE, 0, w - 8);
    if (fill < 0) fill = 0;
    if (fill > w - 8) fill = w - 8;

    canvas_.fillRoundRect(x, y, w, h, 8, UiTheme::PANEL);
    canvas_.drawRoundRect(x, y, w, h, 8, UiTheme::PANEL_LIGHT);
    canvas_.fillRoundRect(x + 4, y + 4, fill, h - 8, 6, UiTheme::PINK);
    canvas_.setTextDatum(MC_DATUM);
    canvas_.setTextSize(2);
    canvas_.setTextColor(UiTheme::TEXT, UiTheme::PANEL);
    canvas_.drawString(String(value_) + "%", cx, y + h / 2 + 1);
    canvas_.setTextDatum(TL_DATUM);
    canvas_.setTextSize(1);
}

void AffinityUI::drawRow(int x, int y, const char* label, const String& value, uint16_t accent) {
    String displayValue = value;
    if (displayValue.length() > 20) {
        displayValue = displayValue.substring(0, 17) + "...";
    }
    canvas_.fillRoundRect(x, y, DISPLAY_WIDTH - x * 2, 22, 6, UiTheme::PANEL);
    canvas_.drawFastVLine(x + 8, y + 5, 12, accent);
    canvas_.setTextDatum(TL_DATUM);
    canvas_.setTextSize(1);
    canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::PANEL);
    canvas_.setCursor(x + 18, y + 7);
    canvas_.print(label);
    canvas_.setTextColor(UiTheme::TEXT, UiTheme::PANEL);
    canvas_.setCursor(x + 86, y + 7);
    canvas_.print(displayValue);
}

void AffinityUI::drawBackButton() {
    UiTheme::drawBackButton(canvas_, BACK_X, BACK_Y, BACK_W, BACK_H, millis() < backPressedUntil_);
}
