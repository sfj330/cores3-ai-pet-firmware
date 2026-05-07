#include "menu_ui.h"
#include "config/app_config.h"
#include <M5CoreS3.h>

MenuUI::MenuUI() : canvas_(&M5.Lcd) {}

void MenuUI::begin() {
    canvas_.setPsram(true);
    canvas_.setColorDepth(16);
    spriteReady_ = canvas_.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT) != nullptr;
    if (spriteReady_) {
        canvas_.fillSprite(TFT_BLACK);
    }
}

void MenuUI::show() {
    visible_ = true;
    activeCard_ = 0;
    dirty_ = true;
}

void MenuUI::hide() {
    visible_ = false;
}

void MenuUI::setActiveCard(int card) {
    if (card >= 0 && card < CARD_COUNT && card != activeCard_) {
        activeCard_ = card;
        dirty_ = true;
    }
}

int MenuUI::getActiveCard() const {
    return activeCard_;
}

void MenuUI::setWifiStatus(const char* status, const char* ip) {
    String newStatus = status ? String(status) : String();
    String newIp = ip ? String(ip) : String();
    if (newStatus != wifiStatus_ || newIp != wifiIp_) {
        wifiStatus_ = newStatus;
        wifiIp_ = newIp;
        dirty_ = true;
    }
}

void MenuUI::markDirty() {
    dirty_ = true;
}

MenuHitZone MenuUI::hitTest(int x, int y) const {
    if (x >= BACK_X && x < BACK_X + BACK_W && y >= BACK_Y && y < BACK_Y + BACK_H) {
        return MenuHitZone::MENU_HIT_BACK;
    }
    int cardX = 20, cardY = 20;
    int cardW = DISPLAY_WIDTH - 40, cardH = DISPLAY_HEIGHT - 80;
    if (x >= cardX && x < cardX + cardW && y >= cardY && y < cardY + cardH) {
        return MenuHitZone::MENU_HIT_CARD;
    }
    return MenuHitZone::MENU_HIT_NONE;
}

void MenuUI::update() {
    if (!visible_ || !spriteReady_) return;
    if (!dirty_) return;

    canvas_.fillSprite(TFT_BLACK);
    drawCard(activeCard_, true);
    drawPageIndicator();
    drawBackButton();

    canvas_.pushSprite(0, 0);
    dirty_ = false;
}

void MenuUI::drawCard(int index, bool focused) {
    int cardW = DISPLAY_WIDTH - 40;
    int cardH = DISPLAY_HEIGHT - 80;
    int cardX = 20;
    int cardY = 20;

    uint16_t bgColor = focused ? TFT_WHITE : TFT_DARKGREY;
    canvas_.fillRoundRect(cardX, cardY, cardW, cardH, 8, bgColor);

    switch (index) {
        case 0: drawWiFiCard(cardX, cardY, cardW, cardH, focused); break;
        case 1: drawCameraCard(cardX, cardY, cardW, cardH, focused); break;
        case 2: drawPomodoroCard(cardX, cardY, cardW, cardH, focused); break;
        case 3: drawSystemCard(cardX, cardY, cardW, cardH, focused); break;
    }
}

void MenuUI::drawWiFiCard(int x, int y, int w, int h, bool focused) {
    uint16_t textColor = focused ? TFT_BLACK : TFT_WHITE;

    canvas_.setTextColor(textColor);
    canvas_.setTextSize(2);
    canvas_.setCursor(x + 20, y + 20);
    canvas_.print("Wi-Fi");

    canvas_.setTextSize(1);
    canvas_.setCursor(x + 20, y + 55);
    canvas_.print("Status: ");
    bool isConnected = wifiStatus_ == "Connected";
    canvas_.setTextColor(isConnected ? TFT_GREEN : TFT_RED);
    canvas_.print(wifiStatus_);

    canvas_.setTextColor(textColor);
    canvas_.setCursor(x + 20, y + 75);
    canvas_.print("IP: ");
    canvas_.print(wifiIp_);

    canvas_.setCursor(x + 20, y + 95);
    canvas_.print("Swipe to switch card");
}

void MenuUI::drawCameraCard(int x, int y, int w, int h, bool focused) {
    uint16_t textColor = focused ? TFT_BLACK : TFT_WHITE;

    canvas_.setTextColor(textColor);
    canvas_.setTextSize(2);
    canvas_.setCursor(x + 20, y + 20);
    canvas_.print("Camera Debug");

    canvas_.setTextSize(1);
    canvas_.setCursor(x + 20, y + 55);
    canvas_.print("View camera preview");

    canvas_.setCursor(x + 20, y + 75);
    canvas_.print("with FPS overlay");

    canvas_.setCursor(x + 20, y + 95);
    canvas_.setTextColor(TFT_BLUE);
    canvas_.print("Tap to enter");
}

void MenuUI::drawPomodoroCard(int x, int y, int w, int h, bool focused) {
    uint16_t textColor = focused ? TFT_BLACK : TFT_WHITE;

    canvas_.setTextColor(textColor);
    canvas_.setTextSize(2);
    canvas_.setCursor(x + 20, y + 20);
    canvas_.print("Pomodoro");

    canvas_.setTextSize(1);
    canvas_.setCursor(x + 20, y + 55);
    canvas_.print("25 min focus timer");

    canvas_.setCursor(x + 20, y + 75);
    canvas_.print("5 min break timer");

    canvas_.setCursor(x + 20, y + 95);
    canvas_.setTextColor(TFT_BLUE);
    canvas_.print("Tap to enter");
}

void MenuUI::drawSystemCard(int x, int y, int w, int h, bool focused) {
    uint16_t textColor = focused ? TFT_BLACK : TFT_WHITE;

    canvas_.setTextColor(textColor);
    canvas_.setTextSize(2);
    canvas_.setCursor(x + 20, y + 20);
    canvas_.print("System Info");

    canvas_.setTextSize(1);
    canvas_.setCursor(x + 20, y + 55);
    canvas_.print("Battery: ");
    canvas_.print("--.-V");

    canvas_.setCursor(x + 20, y + 75);
    canvas_.print("Free Heap: ");
    canvas_.print(ESP.getFreeHeap() / 1024);
    canvas_.print(" KB");

    canvas_.setCursor(x + 20, y + 95);
    canvas_.print("PSRAM: ");
    canvas_.print(ESP.getFreePsram() / 1024);
    canvas_.print(" KB free");
}

void MenuUI::drawBackButton() {
    canvas_.fillRoundRect(BACK_X, BACK_Y, BACK_W, BACK_H, 6, TFT_DARKGREY);
    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(MC_DATUM);
    canvas_.setCursor(BACK_X + BACK_W / 2 - 16, BACK_Y + BACK_H / 2 - 4);
    canvas_.print("Back");
    canvas_.setTextDatum(TL_DATUM);
}

void MenuUI::drawPageIndicator() {
    int dotY = DISPLAY_HEIGHT - 10;
    int startX = DISPLAY_WIDTH / 2 - (CARD_COUNT * 8) / 2;

    for (int i = 0; i < CARD_COUNT; ++i) {
        uint16_t color = (i == activeCard_) ? TFT_WHITE : TFT_DARKGREY;
        canvas_.fillCircle(startX + i * 12, dotY, 3, color);
    }
}
