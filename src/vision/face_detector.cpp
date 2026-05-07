#include "face_detector.h"
#include "config/app_config.h"
#include <Arduino.h>

FaceDetector::FaceDetector() = default;

bool FaceDetector::begin() {
    enabled_ = FACE_DETECTION_ENABLED_ON_BOOT;
    Serial.println("=== FaceDetector BLOCKER REPORT ===");
    Serial.println("ESP-DL/ESP-WHO requires ESP-IDF v5.3+ CMake build system.");
    Serial.println("Current project uses Arduino framework on PlatformIO.");
    Serial.println("ESP-DL human_face_detect model cannot be linked under Arduino framework.");
    Serial.println("Face detection will return 'no face' until ESP-DL is integrated.");
    Serial.println("To resolve: switch to ESP-IDF framework or use Arduino-as-IDF-component.");
    Serial.println("===================================");
    return true;
}

bool FaceDetector::end() {
    enabled_ = false;
    return true;
}

void FaceDetector::setEnabled(bool enabled) {
    enabled_ = enabled;
}

bool FaceDetector::isEnabled() const {
    return enabled_;
}

FaceResult FaceDetector::detect(const uint8_t* frameData, int width, int height) {
    FaceResult result;

    if (!enabled_ || !frameData) {
        return result;
    }

    unsigned long now = millis();
    if (now - lastDetectionTime_ < detectionInterval_) {
        return result;
    }
    lastDetectionTime_ = now;

    result.detected = false;

    return result;
}
