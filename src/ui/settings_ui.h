#pragma once

#include <Arduino.h>
#include <M5GFX.h>
#include "config/app_config.h"

enum class SettingsHitZone {
    NONE,
    BACK,
    BRIGHTNESS_DIM,
    BRIGHTNESS_NORMAL,
    BRIGHTNESS_BRIGHT,
    VOLUME_QUIET,
    VOLUME_NORMAL,
    VOLUME_LOUD
};

class SettingsUI {
public:
    SettingsUI();
    void begin();
    void show();
    void hide();
    void update();
    void markDirty();

    void setBrightness(int level);
    void setVolume(int level);
    void setBattery(float voltage, float percentage, bool lowBattery);

    SettingsHitZone hitTest(int x, int y) const;

private:
    void drawBackButton();
    void drawBrightnessRow();
    void drawVolumeRow();
    void drawBatteryRow();
    void drawSliderBar(int x, int y, int w, int activeIndex, int count,
                       const char* labels[], uint16_t color);

    M5Canvas canvas_;
    bool spriteReady_ = false;
    bool visible_ = false;
    bool dirty_ = true;

    int brightnessLevel_ = 2;
    int volumeLevel_ = 1;

    float batteryVoltage_ = 0;
    int batteryPercent_ = 0;
    bool batteryLow_ = false;

    static constexpr int BATTERY_ROW_Y = 200;
};
