#include "face_detector.h"
#include "config/app_config.h"
#include <Arduino.h>

FaceDetector::FaceDetector() = default;

bool FaceDetector::begin() {
    enabled_ = FACE_DETECTION_ENABLED_ON_BOOT;
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

    // -------------------------------------------------
    // PLACEHOLDER: Replace with real ESP-WHO detection
    //
    // For now, this returns a "face not detected" result.
    // When integrating ESP-WHO:
    //   1. Include <esp_who.hpp> / human_face_detect_msr01.hpp
    //   2. Create detector in begin()
    //   3. Call detector->run(frameData, width, height)
    //   4. Return first face's bounding box
    //
    // The frame data is RGB565 format from the camera.
    // You may need to convert to grayscale first.
    // -------------------------------------------------

    // result.detected remains false by default = no face
    result.detected = false;

    return result;
}
