#include "power_manager.h"
#include "config/app_config.h"
#include <M5CoreS3.h>
#include <esp_sleep.h>

PowerManager::PowerManager() = default;

bool PowerManager::begin() {
    voltage_ = readVoltage();
    update();
    return true;
}

void PowerManager::update() {
    voltage_ = readVoltage();

    if (voltage_ >= BATTERY_FULL_VOLTAGE) {
        percentage_ = 1.0f;
    } else if (voltage_ <= BATTERY_EMPTY_VOLTAGE) {
        percentage_ = 0.0f;
    } else {
        percentage_ = (voltage_ - BATTERY_EMPTY_VOLTAGE) /
                      (BATTERY_FULL_VOLTAGE - BATTERY_EMPTY_VOLTAGE);
    }

    bool wasLow = lowBattery_;
    lowBattery_ = (voltage_ < BATTERY_LOW_THRESHOLD && voltage_ > 0.1f);

    if (lowBattery_ && !wasLow && callback_) {
        callback_(voltage_);
    }

    bool isCritical = (voltage_ < BATTERY_CRITICAL_VOLTAGE && voltage_ > 0.1f);
    if (isCritical) {
        if (!criticalBattery_) {
            criticalBattery_ = true;
            criticalStartMs_ = millis();
        } else if (millis() - criticalStartMs_ >= BATTERY_CRITICAL_DURATION_MS) {
            if (criticalCallback_) {
                criticalCallback_(voltage_);
            }
        }
    } else {
        criticalBattery_ = false;
        criticalStartMs_ = 0;
    }
}

float PowerManager::readVoltage() {
    int mv = M5.Power.getBatteryVoltage();
    if (mv < 100) return 0.0f;
    return mv / 1000.0f;
}

float PowerManager::getVoltage() const {
    return voltage_;
}

float PowerManager::getPercentage() const {
    return percentage_;
}

bool PowerManager::isLowBattery() const {
    return lowBattery_;
}

bool PowerManager::isCriticalBattery() const {
    return criticalBattery_;
}

void PowerManager::setLowBatteryCallback(LowBatteryCallback cb) {
    callback_ = std::move(cb);
}

void PowerManager::setCriticalBatteryCallback(CriticalBatteryCallback cb) {
    criticalCallback_ = std::move(cb);
}

void PowerManager::enterSleep() {
    sleeping_ = true;
    M5.Lcd.setBrightness(24);
}

void PowerManager::exitSleep() {
    sleeping_ = false;
}

bool PowerManager::isSleeping() const {
    return sleeping_;
}

void PowerManager::enterDeepSleep() {
    M5.Lcd.setBrightness(0);
    M5.Speaker.end();
    esp_sleep_enable_timer_wakeup(300 * 1000000ULL);
    esp_deep_sleep_start();
}
