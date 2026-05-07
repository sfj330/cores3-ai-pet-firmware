#pragma once

#include <cstdint>
#include "config/app_config.h"

struct FaceResult {
    bool detected = false;
    int centerX = 0;        // Center X in image coordinates (0-160 for detection)
    int centerY = 0;        // Center Y in image coordinates
    int width = 0;          // Face width
    int height = 0;         // Face height
    float confidence = 0.0f;
};

class FaceDetector {
public:
    FaceDetector();
    bool begin();
    bool end();

    // Process a detection frame and return face result
    // For now, this is a placeholder that returns mock results.
    // Replace with ESP-WHO integration for real face detection.
    FaceResult detect(const uint8_t* frameData, int width, int height);

    void setEnabled(bool enabled);
    bool isEnabled() const;

private:
    bool enabled_ = false;
    unsigned long lastDetectionTime_ = 0;
    int detectionInterval_ = 1000 / DETECTION_FPS;
};
