#include "face_tracker.h"
#include "config/app_config.h"
#include <Arduino.h>

FaceTracker::FaceTracker() = default;

void FaceTracker::update(const FaceResult& face) {
    unsigned long now = millis();

    if (face.detected) {
        // Convert image coordinates to normalized -1..1
        float nx = (face.centerX / (float)DETECTION_FRAME_WIDTH) * 2.0f - 1.0f;
        float ny = (face.centerY / (float)DETECTION_FRAME_HEIGHT) * 2.0f - 1.0f;

        // Apply EMA smoothing
        if (!facePresent_) {
            // First detection — snap immediately
            smoothX_ = nx;
            smoothY_ = ny;
            facePresent_ = true;
        } else {
            smoothX_ = smoothX_ * 0.8f + nx * 0.2f;
            smoothY_ = smoothY_ * 0.8f + ny * 0.2f;
        }

        lastFaceTime_ = now;
    } else if (facePresent_) {
        // Face lost — check timeout
        if (now - lastFaceTime_ > FACE_TRACKING_TIMEOUT_MS) {
            facePresent_ = false;
            smoothX_ = 0.0f;
            smoothY_ = 0.0f;
        }
        // Keep last position until timeout
    }
}

float FaceTracker::getGazeX() const {
    return smoothX_;
}

float FaceTracker::getGazeY() const {
    return smoothY_;
}

bool FaceTracker::hasFace() const {
    return facePresent_;
}

void FaceTracker::reset() {
    facePresent_ = false;
    smoothX_ = 0.0f;
    smoothY_ = 0.0f;
}
