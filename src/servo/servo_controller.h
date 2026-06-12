#pragma once

#include <Arduino.h>
#include <cstdint>
#include "config/app_config.h"

class ServoController {
public:
    bool begin();
    bool isReady() const;
    bool isReleased() const;
    bool forceRescan();
    uint8_t i2cAddress() const;
    const String& statusText() const;

    bool center();
    bool release();
    bool setPanTilt(float panDeg, float tiltDeg);

    float panAngle() const;
    float tiltAngle() const;

private:
    bool probeAddress(uint8_t address);
    uint8_t findPca9685Address(int& foundCount);
    void scanBusForDiagnostics();
    bool write8(uint8_t address, uint8_t reg, uint8_t value);
    bool write8(uint8_t reg, uint8_t value);
    bool setPwmFrequency(float frequencyHz);
    bool setPwm(uint8_t channel, uint16_t onTick, uint16_t offTick);
    bool setChannelOff(uint8_t channel);

    float clampServoAngle(float angleDeg) const;
    uint16_t angleToTicks(float angleDeg) const;
    void setStatus(const char* text);

    bool ready_ = false;
    bool released_ = true;
    int scanFailCount_ = 0;
    uint8_t i2cAddr_ = static_cast<uint8_t>(PCA9685_I2C_ADDR);
    float panAngle_ = SERVO_PAN_CENTER_DEG;
    float tiltAngle_ = SERVO_TILT_CENTER_DEG;
    String status_ = "Servo not initialized";
};
