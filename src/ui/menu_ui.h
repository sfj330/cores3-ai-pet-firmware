#pragma once

#include <cstdint>
#include <M5GFX.h>
#include "config/app_config.h"

enum class MenuHitZone {
    MENU_HIT_NONE,
    MENU_HIT_BACK,
    MENU_HIT_CARD
};

class MenuUI {
public:
    MenuUI();
    void begin();
    void update();
    void show();
    void hide();

    void setActiveCard(int card);
    int getActiveCard() const;

    void setWifiStatus(const char* status, const char* ip);
    void setVisionStatus(const char* status);

    MenuHitZone hitTest(int x, int y) const;

    void markDirty();

    static constexpr int CARD_COUNT = 4;

private:
    void drawCard(int index, bool focused);
    void drawWiFiCard(int x, int y, int w, int h, bool focused);
    void drawCameraCard(int x, int y, int w, int h, bool focused);
    void drawPomodoroCard(int x, int y, int w, int h, bool focused);
    void drawSystemCard(int x, int y, int w, int h, bool focused);
    void drawBackButton();
    void drawPageIndicator();

    M5Canvas canvas_;
    bool spriteReady_ = false;
    int activeCard_ = 0;
    bool visible_ = false;
    bool dirty_ = true;
    String wifiStatus_ = "Disconnected";
    String wifiIp_ = "--.--.--.--";
    String visionStatus_ = "Vision: unknown";

    static constexpr int BACK_X = 5;
    static constexpr int BACK_Y = DISPLAY_HEIGHT - 40;
    static constexpr int BACK_W = 100;
    static constexpr int BACK_H = 30;
};
