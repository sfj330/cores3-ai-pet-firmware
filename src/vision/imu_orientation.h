#pragma once

#include <cstdint>

enum class PomoOrientation {
    TOP,
    RIGHT,
    BOTTOM,
    LEFT,
    FLAT,
    UNKNOWN
};

class ImuOrientation {
public:
    ImuOrientation();

    bool begin();
    void update();

    PomoOrientation getCurrent() const;
    PomoOrientation getStable() const;
    bool isShaking() const;

    static const char* orientationName(PomoOrientation o);

private:
    PomoOrientation current_ = PomoOrientation::UNKNOWN;
    PomoOrientation stable_ = PomoOrientation::UNKNOWN;
    unsigned long stableStart_ = 0;
    PomoOrientation candidate_ = PomoOrientation::UNKNOWN;

    static constexpr unsigned long STABLE_THRESHOLD_MS = 700;
    static constexpr float TILT_THRESHOLD = 0.6f;

    static constexpr int SHAKE_SAMPLE_COUNT = 10;
    static constexpr float SHAKE_VARIANCE_THRESHOLD = 0.15f;
    static constexpr unsigned long SHAKE_COOLDOWN_MS = 2000;

    float accelHistory_[SHAKE_SAMPLE_COUNT][3] = {};
    int shakeSampleIndex_ = 0;
    int shakeSamplesFilled_ = 0;
    bool shaking_ = false;
    unsigned long lastShakeTime_ = 0;

    void updateShakeDetection(float ax, float ay, float az);
};
