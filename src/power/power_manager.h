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

private:
    float readVoltage();

    float voltage_ = 0.0f;
    float percentage_ = 0.0f;
    bool lowBattery_ = false;
    LowBatteryCallback callback_ = nullptr;
    bool sleeping_ = false;
};
