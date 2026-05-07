#include "menu_ui.h"
#include "config/app_config.h"
#include <M5CoreS3.h>

MenuUI::MenuUI() = default;

void MenuUI::begin() {
    // No special init needed
}

void MenuUI::show() {
    visible_ = true;
    activeCard_ = 0;
}

void MenuUI::hide() {
    visible_ = false;
}

void MenuUI::setActiveCard(int card) {
    if (card >= 0 && card < CARD_COUNT) {
        activeCard_ = card;
    }
}

int MenuUI::getActiveCard() const {
    return activeCard_;
}

void MenuUI::update() {
    if (!visible_) return;
    M5.Lcd.fillScreen(TFT_BLACK);

    int cardW = DISPLAY_WIDTH - 40;
    int cardH = DISPLAY_HEIGHT - 80;
    int cardX = 20;
    int cardY = 20;

    drawCard(activeCard_, true);
    drawPageIndicator();
    drawBackButton();
}

void MenuUI::drawCard(int index, bool focused) {
    int cardW = DISPLAY_WIDTH - 40;
    int cardH = DISPLAY_HEIGHT - 80;
    int cardX = 20;
    int cardY = 20;

    uint16_t bgColor = focused ? TFT_WHITE : TFT_DARKGREY;
    M5.Lcd.fillRoundRect(cardX, cardY, cardW, cardH, 8, bgColor);

    switch (index) {
        case 0: drawWiFiCard(cardX, cardY, cardW, cardH, focused); break;
        case 1: drawCameraCard(cardX, cardY, cardW, cardH, focused); break;
        case 2: drawPomodoroCard(cardX, cardY, cardW, cardH, focused); break;
        case 3: drawSystemCard(cardX, cardY, cardW, cardH, focused); break;
    }
}

void MenuUI::drawWiFiCard(int x, int y, int w, int h, bool focused) {
    uint16_t textColor = focused ? TFT_BLACK : TFT_WHITE;

    M5.Lcd.setTextColor(textColor);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(x + 20, y + 20);
    M5.Lcd.print("Wi-Fi");

    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(x + 20, y + 55);
    M5.Lcd.print("Status: ");
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.print("Disconnected");

    M5.Lcd.setTextColor(textColor);
    M5.Lcd.setCursor(x + 20, y + 75);
    M5.Lcd.print("IP: --.--.--.--");

    M5.Lcd.setCursor(x + 20, y + 95);
    M5.Lcd.print("Tap to configure");
}

void MenuUI::drawCameraCard(int x, int y, int w, int h, bool focused) {
    uint16_t textColor = focused ? TFT_BLACK : TFT_WHITE;

    M5.Lcd.setTextColor(textColor);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(x + 20, y + 20);
    M5.Lcd.print("Camera Debug");

    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(x + 20, y + 55);
    M5.Lcd.print("View camera preview");

    M5.Lcd.setCursor(x + 20, y + 75);
    M5.Lcd.print("with FPS overlay");

    M5.Lcd.setCursor(x + 20, y + 95);
    M5.Lcd.setTextColor(TFT_BLUE);
    M5.Lcd.print("Tap to enter");
}

void MenuUI::drawPomodoroCard(int x, int y, int w, int h, bool focused) {
    uint16_t textColor = focused ? TFT_BLACK : TFT_WHITE;

    M5.Lcd.setTextColor(textColor);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(x + 20, y + 20);
    M5.Lcd.print("Pomodoro");

    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(x + 20, y + 55);
    M5.Lcd.print("25 min focus timer");

    M5.Lcd.setCursor(x + 20, y + 75);
    M5.Lcd.print("5 min break timer");

    M5.Lcd.setCursor(x + 20, y + 95);
    M5.Lcd.setTextColor(TFT_BLUE);
    M5.Lcd.print("Tap to enter");
}

void MenuUI::drawSystemCard(int x, int y, int w, int h, bool focused) {
    uint16_t textColor = focused ? TFT_BLACK : TFT_WHITE;

    M5.Lcd.setTextColor(textColor);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(x + 20, y + 20);
    M5.Lcd.print("System Info");

    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(x + 20, y + 55);
    M5.Lcd.print("Battery: ");
    M5.Lcd.print("--.-V");

    M5.Lcd.setCursor(x + 20, y + 75);
    M5.Lcd.print("Free Heap: ");
    M5.Lcd.print(ESP.getFreeHeap() / 1024);
    M5.Lcd.print(" KB");

    M5.Lcd.setCursor(x + 20, y + 95);
    M5.Lcd.print("PSRAM: ");
    M5.Lcd.print(ESP.getFreePsram() / 1024);
    M5.Lcd.print(" KB free");
}

void MenuUI::drawBackButton() {
    // Back arrow in top-left corner
    M5.Lcd.fillTriangle(15, DISPLAY_HEIGHT - 25, 25, DISPLAY_HEIGHT - 35,
                        25, DISPLAY_HEIGHT - 15, TFT_WHITE);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(30, DISPLAY_HEIGHT - 22);
    M5.Lcd.print("Back (left-swipe)");
}

void MenuUI::drawPageIndicator() {
    // Dots at the bottom
    int dotY = DISPLAY_HEIGHT - 10;
    int startX = DISPLAY_WIDTH / 2 - (CARD_COUNT * 8) / 2;

    for (int i = 0; i < CARD_COUNT; ++i) {
        uint16_t color = (i == activeCard_) ? TFT_WHITE : TFT_DARKGREY;
        M5.Lcd.fillCircle(startX + i * 12, dotY, 3, color);
    }
}
