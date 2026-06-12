#include "servo_controller.h"
#include "config/app_config.h"
#include <M5CoreS3.h>
#include <cmath>

namespace {
constexpr uint8_t REG_MODE1 = 0x00;
constexpr uint8_t REG_MODE2 = 0x01;
constexpr uint8_t REG_LED0_ON_L = 0x06;
constexpr uint8_t REG_PRESCALE = 0xFE;

constexpr uint8_t MODE1_RESTART = 0x80;
constexpr uint8_t MODE1_AUTO_INCREMENT = 0x20;
constexpr uint8_t MODE1_SLEEP = 0x10;
constexpr uint8_t MODE2_OUTDRV = 0x04;
constexpr uint8_t FULL_OFF_BIT = 0x10;
constexpr uint8_t PCA9685_ADDR_MIN = 0x40;
constexpr uint8_t PCA9685_ADDR_MAX = 0x7F;
constexpr float PCA9685_OSCILLATOR_HZ = 25000000.0f;
constexpr float PWM_STEPS = 4096.0f;
}

bool ServoController::begin() {
    // Always re-init PortA I2C; camera/touch/AI tasks may have disturbed the bus.
    if (!M5.Ex_I2C.begin()) {
        ready_ = false;
        released_ = true;
        setStatus("PortA I2C init failed");
        Serial.println("Servo: PortA I2C init failed");
        scanFailCount_++;
        return false;
    }
    delay(2);

    const uint8_t configuredAddr = static_cast<uint8_t>(PCA9685_I2C_ADDR);
    uint8_t detectedAddr = 0;
    if (probeAddress(configuredAddr)) {
        detectedAddr = configuredAddr;
    } else {
        int candidateCount = 0;
        detectedAddr = findPca9685Address(candidateCount);
        if (detectedAddr != 0) {
            Serial.printf("Servo: PCA9685 candidate at 0x%02X (configured 0x%02X, candidates=%d)\n",
                          detectedAddr, configuredAddr, candidateCount);
        }
    }

    if (detectedAddr == 0) {
        ready_ = false;
        released_ = true;
        scanFailCount_++;
        status_ = String("PCA9685 no ACK @0x") + String(configuredAddr, HEX) +
                  " (try " + String(scanFailCount_) + ")";
        Serial.printf("Servo: PCA9685 no ACK at 0x%02X (attempt %d, will keep retrying)\n",
                      configuredAddr, scanFailCount_);
        if (scanFailCount_ == 1) {
            scanBusForDiagnostics();
        }
        return false;
    }

    i2cAddr_ = detectedAddr;
    if (!write8(REG_MODE2, MODE2_OUTDRV) ||
        !setPwmFrequency(static_cast<float>(SERVO_PWM_FREQ_HZ))) {
        ready_ = false;
        released_ = true;
        scanFailCount_++;
        status_ = String("PCA9685 init failed @0x") + String(i2cAddr_, HEX);
        Serial.printf("Servo: PCA9685 init failed at 0x%02X (will retry)\n", i2cAddr_);
        return false;
    }

    scanFailCount_ = 0;
    ready_ = true;
    released_ = true;  // PWM channels are off until first setPanTilt() call.
    status_ = String("PCA9685 ready @0x") + String(i2cAddr_, HEX);
    Serial.printf("Servo: PCA9685 ready at 0x%02X, pan ch=%d, tilt ch=%d\n",
                  i2cAddr_, SERVO_PAN_CHANNEL, SERVO_TILT_CHANNEL);
    return true;
}

bool ServoController::forceRescan() {
    scanFailCount_ = 0;
    ready_ = false;
    released_ = true;
    M5.Ex_I2C.release();
    delay(2);
    Serial.println("Servo: force rescan");
    return begin();
}

bool ServoController::isReady() const {
    return ready_;
}

bool ServoController::isReleased() const {
    return released_;
}

uint8_t ServoController::i2cAddress() const {
    return i2cAddr_;
}

const String& ServoController::statusText() const {
    return status_;
}

bool ServoController::center() {
    bool ok = setPanTilt(SERVO_PAN_CENTER_DEG, SERVO_TILT_CENTER_DEG);
    if (ok) {
        setStatus("Centered");
    }
    return ok;
}

bool ServoController::release() {
    if (!ready_) {
        setStatus("Servo unavailable");
        return false;
    }

    bool ok = setChannelOff(SERVO_PAN_CHANNEL) && setChannelOff(SERVO_TILT_CHANNEL);
    if (ok) {
        released_ = true;
        setStatus("PWM released");
        Serial.println("Servo: PWM released");
    } else {
        ready_ = false;
        setStatus("PWM release failed");
        Serial.println("Servo: PWM release failed");
    }
    return ok;
}

bool ServoController::setPanTilt(float panDeg, float tiltDeg) {
    if (!ready_) {
        setStatus("Servo unavailable");
        return false;
    }

    float safePan = clampServoAngle(panDeg);
    float safeTilt = clampServoAngle(tiltDeg);
    bool ok = setPwm(SERVO_PAN_CHANNEL, 0, angleToTicks(safePan)) &&
              setPwm(SERVO_TILT_CHANNEL, 0, angleToTicks(safeTilt));
    if (ok) {
        panAngle_ = safePan;
        tiltAngle_ = safeTilt;
        released_ = false;
        setStatus("Tracking tilt");
    } else {
        ready_ = false;
        setStatus("Servo write failed");
        Serial.println("Servo: channel write failed");
    }
    return ok;
}

float ServoController::panAngle() const {
    return panAngle_;
}

float ServoController::tiltAngle() const {
    return tiltAngle_;
}

bool ServoController::probeAddress(uint8_t address) {
    // Probe by writing MODE1=SLEEP. PCA9685 ACKs this register reliably.
    return write8(address, REG_MODE1, MODE1_SLEEP);
}

uint8_t ServoController::findPca9685Address(int& foundCount) {
    foundCount = 0;
    uint8_t firstUsable = 0;
    const uint8_t configuredAddr = static_cast<uint8_t>(PCA9685_I2C_ADDR);

    for (uint8_t address = PCA9685_ADDR_MIN; address <= PCA9685_ADDR_MAX; address++) {
        if (address == configuredAddr) continue;
        if (!M5.Ex_I2C.scanID(address, PCA9685_I2C_FREQ_HZ)) continue;

        foundCount++;
        if (firstUsable == 0 && probeAddress(address)) {
            firstUsable = address;
        }
    }

    return firstUsable;
}

void ServoController::scanBusForDiagnostics() {
    Serial.println("Servo: scanning PortA I2C bus 0x03..0x77 ...");
    Serial.flush();

    int found = 0;
    for (uint8_t address = 0x03; address <= 0x77; address++) {
        if (M5.Ex_I2C.scanID(address, 400000)) {
            Serial.printf("Servo: I2C device found @ 0x%02X\n", address);
            Serial.flush();
            found++;
        }
    }

    Serial.printf("Servo: bus scan done, %d device(s) found.\n", found);
    if (found == 0) {
        Serial.println("Servo: NO I2C device responded on PortA. Check Grove cable orientation, "
                       "PCA9685 VCC logic power, V+ servo power, and common GND.");
    } else {
        Serial.println("Servo: PCA9685 auto-detect scans 0x40..0x7F. "
                       "If your driver is outside that range, update PCA9685_I2C_ADDR.");
    }
    Serial.flush();
}

bool ServoController::write8(uint8_t address, uint8_t reg, uint8_t value) {
    return M5.Ex_I2C.writeRegister8(address, reg, value, PCA9685_I2C_FREQ_HZ);
}

bool ServoController::write8(uint8_t reg, uint8_t value) {
    return write8(i2cAddr_, reg, value);
}

bool ServoController::setPwmFrequency(float frequencyHz) {
    float prescaleValue = PCA9685_OSCILLATOR_HZ / (PWM_STEPS * frequencyHz) - 1.0f;
    uint8_t prescale = static_cast<uint8_t>(prescaleValue + 0.5f);

    if (!write8(REG_MODE1, MODE1_SLEEP)) return false;
    if (!write8(REG_PRESCALE, prescale)) return false;
    if (!write8(REG_MODE1, MODE1_AUTO_INCREMENT)) return false;
    delay(5);
    return write8(REG_MODE1, MODE1_RESTART | MODE1_AUTO_INCREMENT);
}

bool ServoController::setPwm(uint8_t channel, uint16_t onTick, uint16_t offTick) {
    if (channel >= 16) return false;

    uint8_t data[4] = {
        static_cast<uint8_t>(onTick & 0xFF),
        static_cast<uint8_t>((onTick >> 8) & 0x0F),
        static_cast<uint8_t>(offTick & 0xFF),
        static_cast<uint8_t>((offTick >> 8) & 0x0F)
    };
    uint8_t reg = REG_LED0_ON_L + 4 * channel;
    return M5.Ex_I2C.writeRegister(i2cAddr_, reg, data, sizeof(data), PCA9685_I2C_FREQ_HZ);
}

bool ServoController::setChannelOff(uint8_t channel) {
    if (channel >= 16) return false;

    uint8_t data[4] = {0, 0, 0, FULL_OFF_BIT};
    uint8_t reg = REG_LED0_ON_L + 4 * channel;
    return M5.Ex_I2C.writeRegister(i2cAddr_, reg, data, sizeof(data), PCA9685_I2C_FREQ_HZ);
}

float ServoController::clampServoAngle(float angleDeg) const {
    if (angleDeg < SERVO_SAFE_MIN_DEG) return static_cast<float>(SERVO_SAFE_MIN_DEG);
    if (angleDeg > SERVO_SAFE_MAX_DEG) return static_cast<float>(SERVO_SAFE_MAX_DEG);
    return angleDeg;
}

uint16_t ServoController::angleToTicks(float angleDeg) const {
    float clamped = angleDeg;
    if (clamped < 0.0f) clamped = 0.0f;
    if (clamped > 180.0f) clamped = 180.0f;

    float pulseUs = SERVO_MIN_PULSE_US +
                    (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) * (clamped / 180.0f);
    float ticks = pulseUs * static_cast<float>(SERVO_PWM_FREQ_HZ) * PWM_STEPS / 1000000.0f;
    if (ticks < 0.0f) ticks = 0.0f;
    if (ticks > PWM_STEPS - 1.0f) ticks = PWM_STEPS - 1.0f;
    return static_cast<uint16_t>(ticks + 0.5f);
}

void ServoController::setStatus(const char* text) {
    status_ = text ? String(text) : String();
}
