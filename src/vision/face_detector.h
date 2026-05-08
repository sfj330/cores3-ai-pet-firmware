#pragma once

#include <cstdint>
#include "config/app_config.h"

struct FaceResult {
    bool detected = false;
    int centerX = 0;
    int centerY = 0;
    int width = 0;
    int height = 0;
    float confidence = 0.0f;
};

class FaceDetector {
public:
    FaceDetector();
    bool begin();
    bool end();

    FaceResult detect(const uint8_t* frameData, int width, int height);

    void setEnabled(bool enabled);
    bool isEnabled() const;
    bool backendAvailable() const;
    const char* statusText() const;

private:
    bool enabled_ = false;
    bool backendAvailable_ = false;
    bool statusPrinted_ = false;
};
