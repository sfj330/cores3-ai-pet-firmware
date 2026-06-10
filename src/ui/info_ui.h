#pragma once

#include <Arduino.h>
#include <M5GFX.h>
#include "config/app_config.h"
#include "app/system_status.h"

enum class InfoPageMode {
    WIFI,
    SYSTEM
};

enum class InfoHitZone {
    INFO_HIT_NONE,
    INFO_HIT_BACK
};

class InfoUI {
public:
    InfoUI();
    void begin();
    void show(InfoPageMode mode);
    void hide();
    void update();
    void markDirty();

    void setWifiStatus(const char* status, const char* ip, int rssi, bool configured);
    void setSystemStatus(const SystemStatusViewModel& status);

    InfoHitZone hitTest(int x, int y) const;

    void setBackPressed() { backPressedUntil_ = millis() + 150; dirty_ = true; }

private:
    void drawWifiPage();
    void drawSystemPage();
    void drawBackButton();
    void drawRow(int x, int y, const char* label, const String& value, uint16_t accent);

    M5Canvas canvas_;
    bool spriteReady_ = false;
    bool visible_ = false;
    bool dirty_ = true;
    unsigned long backPressedUntil_ = 0;
    InfoPageMode mode_ = InfoPageMode::WIFI;

    String wifiStatus_ = "Unknown";
    String wifiIp_ = "--.--.--.--";
    int wifiRssi_ = 0;
    bool wifiConfigured_ = false;

    SystemStatusViewModel systemStatus_;

    static constexpr int BACK_X = 5;
    static constexpr int BACK_Y = 5;
    static constexpr int BACK_W = 74;
    static constexpr int BACK_H = 26;
};
