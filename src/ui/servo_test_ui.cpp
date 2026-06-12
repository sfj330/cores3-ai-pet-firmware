#include "servo_test_ui.h"
#include "ui/ui_theme.h"
#include <M5CoreS3.h>

ServoTestUI::ServoTestUI() : canvas_(&M5.Lcd) {}

void ServoTestUI::begin() {
    canvas_.setPsram(true);
    canvas_.setColorDepth(16);
    spriteReady_ = canvas_.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT) != nullptr;
}

void ServoTestUI::show() {
    visible_ = true;
    dirty_ = true;
}

void ServoTestUI::hide() {
    visible_ = false;
}

void ServoTestUI::update() {
    if (!visible_ || !spriteReady_) return;
    if (!dirty_) return;
    dirty_ = false;

    canvas_.fillSprite(UiTheme::BG);

    drawBackButton();
    drawStatusPanel();
    drawDirectionPad();
    drawActionButtons();

    canvas_.pushSprite(0, 0);
}

void ServoTestUI::setServoState(bool ready, bool released, float pan, float tilt, const String& status) {
    if (ready != servoReady_ || released != servoReleased_ ||
        pan != panAngle_ || tilt != tiltAngle_ || status != servoStatus_) {
        servoReady_ = ready;
        servoReleased_ = released;
        panAngle_ = pan;
        tiltAngle_ = tilt;
        servoStatus_ = status;
        dirty_ = true;
    }
}

ServoTestHitZone ServoTestUI::hitTest(int x, int y) const {
    // Back button
    if (x >= BACK_X && x < BACK_X + BACK_W && y >= BACK_Y && y < BACK_Y + BACK_H)
        return ServoTestHitZone::SERVO_TEST_HIT_BACK;

    // Direction pad: left
    int lx = DPAD_CX - DPAD_BTN - DPAD_GAP;
    int ly = DPAD_CY - DPAD_BTN / 2;
    if (x >= lx && x < lx + DPAD_BTN && y >= ly && y < ly + DPAD_BTN)
        return ServoTestHitZone::SERVO_TEST_HIT_LEFT;

    // Direction pad: right
    int rx = DPAD_CX + DPAD_GAP;
    int ry = DPAD_CY - DPAD_BTN / 2;
    if (x >= rx && x < rx + DPAD_BTN && y >= ry && y < ry + DPAD_BTN)
        return ServoTestHitZone::SERVO_TEST_HIT_RIGHT;

    // Direction pad: up
    int ux = DPAD_CX - DPAD_BTN / 2;
    int uy = DPAD_CY - DPAD_BTN - DPAD_GAP;
    if (x >= ux && x < ux + DPAD_BTN && y >= uy && y < uy + DPAD_BTN)
        return ServoTestHitZone::SERVO_TEST_HIT_UP;

    // Direction pad: down
    int dx = DPAD_CX - DPAD_BTN / 2;
    int dy = DPAD_CY + DPAD_GAP;
    if (x >= dx && x < dx + DPAD_BTN && y >= dy && y < dy + DPAD_BTN)
        return ServoTestHitZone::SERVO_TEST_HIT_DOWN;

    // Action buttons
    int totalActW = 6 * ACT_W + 5 * ACT_GAP;
    int actStartX = (DISPLAY_WIDTH - totalActW) / 2;
    const ServoTestHitZone actZones[] = {
        ServoTestHitZone::SERVO_TEST_HIT_CENTER,
        ServoTestHitZone::SERVO_TEST_HIT_NOD,
        ServoTestHitZone::SERVO_TEST_HIT_SHAKE,
        ServoTestHitZone::SERVO_TEST_HIT_DANCE,
        ServoTestHitZone::SERVO_TEST_HIT_RELEASE,
        ServoTestHitZone::SERVO_TEST_HIT_SCAN
    };
    for (int i = 0; i < 6; i++) {
        int bx = actStartX + i * (ACT_W + ACT_GAP);
        if (x >= bx && x < bx + ACT_W && y >= ACT_Y && y < ACT_Y + ACT_H)
            return actZones[i];
    }

    return ServoTestHitZone::SERVO_TEST_HIT_NONE;
}

void ServoTestUI::drawBackButton() {
    bool pressed = millis() < backPressedUntil_;
    UiTheme::drawBackButton(canvas_, BACK_X, BACK_Y, BACK_W, BACK_H, pressed);
}

void ServoTestUI::drawStatusPanel() {
    // Title
    UiTheme::drawTitle(canvas_, "Servo Test", servoReady_ ? "PCA9685 Connected" : "PCA9685 Not Found",
                        servoReady_ ? UiTheme::GREEN : UiTheme::RED);

    // Status panel
    constexpr int PANEL_X = 10;
    constexpr int PANEL_Y = 38;
    constexpr int PANEL_W = 300;
    constexpr int PANEL_H = 50;
    UiTheme::drawPanel(canvas_, PANEL_X, PANEL_Y, PANEL_W, PANEL_H, UiTheme::CYAN);

    canvas_.setTextDatum(TL_DATUM);
    canvas_.setTextSize(1);

    // Pan angle
    canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::PANEL);
    canvas_.setCursor(PANEL_X + 10, PANEL_Y + 8);
    canvas_.print("Pan:");
    canvas_.setTextColor(UiTheme::CYAN, UiTheme::PANEL);
    canvas_.printf("%.0f", panAngle_);

    // Tilt angle
    canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::PANEL);
    canvas_.setCursor(PANEL_X + 100, PANEL_Y + 8);
    canvas_.print("Tilt:");
    canvas_.setTextColor(UiTheme::CYAN, UiTheme::PANEL);
    canvas_.printf("%.0f", tiltAngle_);

    // Released state
    canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::PANEL);
    canvas_.setCursor(PANEL_X + 200, PANEL_Y + 8);
    canvas_.print(servoReleased_ ? "Released" : "Active");

    // Status text
    canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::PANEL);
    canvas_.setCursor(PANEL_X + 10, PANEL_Y + 28);
    canvas_.print(servoStatus_.c_str());
}

void ServoTestUI::drawDirectionPad() {
    constexpr uint16_t DIR_BG = UiTheme::PANEL;
    constexpr uint16_t DIR_BORDER = UiTheme::PANEL_LIGHT;
    constexpr uint16_t DIR_ACCENT = UiTheme::CYAN;

    // Left
    int lx = DPAD_CX - DPAD_BTN - DPAD_GAP;
    int ly = DPAD_CY - DPAD_BTN / 2;
    canvas_.fillRoundRect(lx, ly, DPAD_BTN, DPAD_BTN, 6, DIR_BG);
    canvas_.drawRoundRect(lx, ly, DPAD_BTN, DPAD_BTN, 6, DIR_BORDER);
    canvas_.setTextDatum(MC_DATUM);
    canvas_.setTextSize(2);
    canvas_.setTextColor(DIR_ACCENT, DIR_BG);
    canvas_.drawString("<", lx + DPAD_BTN / 2, ly + DPAD_BTN / 2);

    // Right
    int rx = DPAD_CX + DPAD_GAP;
    int ry = DPAD_CY - DPAD_BTN / 2;
    canvas_.fillRoundRect(rx, ry, DPAD_BTN, DPAD_BTN, 6, DIR_BG);
    canvas_.drawRoundRect(rx, ry, DPAD_BTN, DPAD_BTN, 6, DIR_BORDER);
    canvas_.setTextColor(DIR_ACCENT, DIR_BG);
    canvas_.drawString(">", rx + DPAD_BTN / 2, ry + DPAD_BTN / 2);

    // Up
    int ux = DPAD_CX - DPAD_BTN / 2;
    int uy = DPAD_CY - DPAD_BTN - DPAD_GAP;
    canvas_.fillRoundRect(ux, uy, DPAD_BTN, DPAD_BTN, 6, DIR_BG);
    canvas_.drawRoundRect(ux, uy, DPAD_BTN, DPAD_BTN, 6, DIR_BORDER);
    canvas_.setTextColor(DIR_ACCENT, DIR_BG);
    canvas_.drawString("^", ux + DPAD_BTN / 2, uy + DPAD_BTN / 2);

    // Down
    int dx = DPAD_CX - DPAD_BTN / 2;
    int dy = DPAD_CY + DPAD_GAP;
    canvas_.fillRoundRect(dx, dy, DPAD_BTN, DPAD_BTN, 6, DIR_BG);
    canvas_.drawRoundRect(dx, dy, DPAD_BTN, DPAD_BTN, 6, DIR_BORDER);
    canvas_.setTextColor(DIR_ACCENT, DIR_BG);
    canvas_.drawString("v", dx + DPAD_BTN / 2, dy + DPAD_BTN / 2);

    // Angle labels next to dpad
    canvas_.setTextSize(1);
    canvas_.setTextDatum(CL_DATUM);
    canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::BG);

    int labelX = DPAD_CX + DPAD_BTN / 2 + DPAD_GAP + DPAD_BTN + 10;
    canvas_.setCursor(labelX, DPAD_CY - 14);
    canvas_.printf("Pan: %.0f", panAngle_);
    canvas_.setCursor(labelX, DPAD_CY + 4);
    canvas_.printf("Tilt: %.0f", tiltAngle_);

    canvas_.setTextDatum(TL_DATUM);
}

void ServoTestUI::drawActionButtons() {
    const char* labels[] = {"Ctr", "Nod", "Shk", "Dnc", "Rel", "Scan"};
    const uint16_t colors[] = {
        UiTheme::GREEN, UiTheme::AMBER, UiTheme::AMBER,
        UiTheme::PINK, UiTheme::TEXT_DIM, UiTheme::CYAN
    };

    int totalW = 6 * ACT_W + 5 * ACT_GAP;
    int startX = (DISPLAY_WIDTH - totalW) / 2;

    for (int i = 0; i < 6; i++) {
        int bx = startX + i * (ACT_W + ACT_GAP);
        canvas_.fillRoundRect(bx, ACT_Y, ACT_W, ACT_H, 5, UiTheme::PANEL);
        canvas_.drawRoundRect(bx, ACT_Y, ACT_W, ACT_H, 5, colors[i]);
        canvas_.setTextDatum(MC_DATUM);
        canvas_.setTextSize(1);
        canvas_.setTextColor(colors[i], UiTheme::PANEL);
        canvas_.drawString(labels[i], bx + ACT_W / 2, ACT_Y + ACT_H / 2);
    }

    canvas_.setTextDatum(TL_DATUM);
}
