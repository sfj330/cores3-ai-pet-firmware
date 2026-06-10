#pragma once

#include <cstdint>
#include <M5GFX.h>
#include "config/app_config.h"

struct AppLayout {
    int cx;
    int y;
};

enum class MenuHitZone {
    MENU_HIT_NONE,
    MENU_HIT_BACK,
    MENU_HIT_APP
};

class MenuUI {
public:
    MenuUI();
    void begin();
    void update();
    void show();
    void hide();

    int appAt(int x, int y) const;

    void setWifiStatus(const char* status, const char* ip);
    void setVisionStatus(const char* status);

    MenuHitZone hitTest(int x, int y) const;

    void markDirty();

    void nextPage();
    void prevPage();
    int pageCount() const;
    int currentPage() const;

    void setBackPressed() { backPressedUntil_ = millis() + 150; dirty_ = true; }

    static constexpr int APP_COUNT = 7;
    static constexpr int PAGE_SIZE = MENU_PAGE_SIZE;

private:
    void drawApp(int index, AppLayout layout);
    void drawAppIcon(int index, int x, int y, int size, uint16_t accent);
    void drawBackButton();
    void drawPageDots();

    M5Canvas canvas_;
    bool spriteReady_ = false;
    bool visible_ = false;
    bool dirty_ = true;
    unsigned long backPressedUntil_ = 0;
    String wifiStatus_ = "Disconnected";
    String wifiIp_ = "--.--.--.--";
    String visionStatus_ = "Vision: unknown";
    int currentPage_ = 0;

    static constexpr int BACK_X = 5;
    static constexpr int BACK_Y = 5;
    static constexpr int BACK_W = 74;
    static constexpr int BACK_H = 26;
    static constexpr int APP_W = 74;
    static constexpr int APP_H = 74;
    static constexpr int ICON_SIZE = 50;
};
