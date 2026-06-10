#include "info_ui.h"
#include "ui/ui_theme.h"
#include <M5CoreS3.h>
#include <cmath>

InfoUI::InfoUI() : canvas_(&M5.Lcd) {}

void InfoUI::begin() {
    canvas_.setPsram(true);
    canvas_.setColorDepth(16);
    spriteReady_ = canvas_.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT) != nullptr;
    if (spriteReady_) {
        canvas_.fillSprite(UiTheme::BG);
    }
}

void InfoUI::show(InfoPageMode mode) {
    mode_ = mode;
    visible_ = true;
    dirty_ = true;
}

void InfoUI::hide() {
    visible_ = false;
}

void InfoUI::markDirty() {
    dirty_ = true;
}

void InfoUI::setWifiStatus(const char* status, const char* ip, int rssi, bool configured) {
    String newStatus = status ? String(status) : String();
    String newIp = ip ? String(ip) : String();
    if (newStatus != wifiStatus_ || newIp != wifiIp_ || rssi != wifiRssi_ || configured != wifiConfigured_) {
        wifiStatus_ = newStatus;
        wifiIp_ = newIp;
        wifiRssi_ = rssi;
        wifiConfigured_ = configured;
        dirty_ = true;
    }
}

void InfoUI::setSystemStatus(const SystemStatusViewModel& status) {
    if (!systemStatusEquals(systemStatus_, status)) {
        systemStatus_ = status;
        dirty_ = true;
    }
}

InfoHitZone InfoUI::hitTest(int x, int y) const {
    if (x >= BACK_X && x < BACK_X + BACK_W && y >= BACK_Y && y < BACK_Y + BACK_H) {
        return InfoHitZone::INFO_HIT_BACK;
    }
    return InfoHitZone::INFO_HIT_NONE;
}

void InfoUI::update() {
    if (!visible_ || !spriteReady_) return;
    if (millis() < backPressedUntil_) dirty_ = true;
    if (!dirty_) return;

    canvas_.fillSprite(UiTheme::BG);
    if (mode_ == InfoPageMode::WIFI) {
        drawWifiPage();
    } else {
        drawSystemPage();
    }
    drawBackButton();
    canvas_.pushSprite(0, 0);
    dirty_ = false;
}

void InfoUI::drawWifiPage() {
    bool online = wifiStatus_ == "Connected";
    uint16_t accent = online ? UiTheme::GREEN : UiTheme::CYAN;
    UiTheme::drawTitle(canvas_, "Wi-Fi", online ? "Connected" : "Status", accent);

    canvas_.drawCircle(70, 93, 34, UiTheme::PANEL_LIGHT);
    canvas_.drawArc(70, 101, 28, 25, 218, 322, accent);
    canvas_.drawArc(70, 101, 19, 16, 224, 316, accent);
    canvas_.drawArc(70, 101, 10, 8, 232, 308, accent);
    canvas_.fillCircle(70, 116, 5, accent);

    drawRow(124, 62, "Config", wifiConfigured_ ? "Ready" : "Missing", UiTheme::CYAN);
    drawRow(124, 92, "Status", wifiStatus_, accent);
    drawRow(124, 122, "IP", wifiIp_, UiTheme::TEXT);
    drawRow(124, 152, "RSSI", online ? String(wifiRssi_) + " dBm" : String("--"), UiTheme::AMBER);
}

void InfoUI::drawSystemPage() {
    String subtitle = String("XiaoZhi ") + systemStatusXiaoZhiSummary(systemStatus_) +
                      " / Vision " + systemStatusVisionSummary(systemStatus_);
    UiTheme::drawTitle(canvas_, "System", subtitle.c_str(), UiTheme::GREEN);

    canvas_.fillRoundRect(22, 54, 72, 72, 16, UiTheme::GREEN);
    canvas_.drawRoundRect(22, 54, 72, 72, 16, UiTheme::TEXT);
    canvas_.drawCircle(58, 90, 23, UiTheme::BG);
    canvas_.drawCircle(58, 90, 10, UiTheme::BG);
    for (int i = 0; i < 8; ++i) {
        float angle = i * 0.785398f;
        int x1 = 58 + static_cast<int>(cos(angle) * 27.0f);
        int y1 = 90 + static_cast<int>(sin(angle) * 27.0f);
        int x2 = 58 + static_cast<int>(cos(angle) * 33.0f);
        int y2 = 90 + static_cast<int>(sin(angle) * 33.0f);
        canvas_.drawLine(x1, y1, x2, y2, UiTheme::BG);
    }

    drawRow(104, 48, "Wi-Fi", systemStatusWifiLine(systemStatus_), UiTheme::CYAN);
    drawRow(104, 76, "SD", systemStatusSdLine(systemStatus_), systemStatus_.sd.ready ? UiTheme::GREEN : UiTheme::AMBER);
    drawRow(104, 104, "Audio", systemStatusAudioLine(systemStatus_), UiTheme::AMBER);
    drawRow(104, 132, "Control", systemStatusControlLine(systemStatus_), UiTheme::BLUE);
    drawRow(104, 160, "Memory", systemStatusMemoryLine(systemStatus_), UiTheme::TEXT_DIM);
}

void InfoUI::drawRow(int x, int y, const char* label, const String& value, uint16_t accent) {
    String displayValue = value;
    if (displayValue.length() > 20) {
        displayValue = displayValue.substring(0, 17) + "...";
    }
    canvas_.fillRoundRect(x, y, DISPLAY_WIDTH - x - 18, 22, 6, UiTheme::PANEL);
    canvas_.drawFastVLine(x + 6, y + 5, 12, accent);
    canvas_.setTextDatum(TL_DATUM);
    canvas_.setTextSize(1);
    canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::PANEL);
    canvas_.setCursor(x + 14, y + 7);
    canvas_.print(label);
    canvas_.setTextColor(UiTheme::TEXT, UiTheme::PANEL);
    canvas_.setCursor(x + 62, y + 7);
    canvas_.print(displayValue);
}

void InfoUI::drawBackButton() {
    UiTheme::drawBackButton(canvas_, BACK_X, BACK_Y, BACK_W, BACK_H, millis() < backPressedUntil_);
}
