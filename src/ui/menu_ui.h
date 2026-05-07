#pragma once

#include <cstdint>

class MenuUI {
public:
    MenuUI();
    void begin();
    void update();
    void show();
    void hide();

    void setActiveCard(int card);
    int getActiveCard() const;

    static constexpr int CARD_COUNT = 4;
    static constexpr int CARDS_PER_SCREEN = 1; // One card at a time

private:
    void drawCard(int index, bool focused);
    void drawWiFiCard(int x, int y, int w, int h, bool focused);
    void drawCameraCard(int x, int y, int w, int h, bool focused);
    void drawPomodoroCard(int x, int y, int w, int h, bool focused);
    void drawSystemCard(int x, int y, int w, int h, bool focused);
    void drawBackButton();
    void drawPageIndicator();

    int activeCard_ = 0;
    bool visible_ = false;
};
