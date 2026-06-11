#pragma once

#include <cstdint>
#include <functional>

class PowerManager {
public:
    using LowBatteryCallback = std::function<void(float voltage)>;

    PowerManager();
    bool begin();

    // Call every 1s to update readings
    void update();

    // Get battery voltage (V)
    float getVoltage() const;

    // Get battery percentage (0.0 - 1.0)
    float getPercentage() const;

    // Whether battery is critically low
    bool isLowBattery() const;

    void setLowBatteryCallback(LowBatteryCallback cb);

    // Trigger low-power sleep
    void enterSleep();
    void exitSleep();
    bool isSleeping() const;

    void enterDeepSleep(uint64_t sleepSeconds = 0);
    int getAmbientLight() const { return ambientLight_; }
    void updateAmbientLight();

private:
    float readVoltage();

    float voltage_ = 0.0f;
    float percentage_ = 0.0f;
    bool lowBattery_ = false;
    LowBatteryCallback callback_ = nullptr;
    bool sleeping_ = false;
    int ambientLight_ = 0;
    unsigned long lastLightReadMs_ = 0;
};
