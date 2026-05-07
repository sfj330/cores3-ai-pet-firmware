#pragma once

#include <cstdint>
#include "face_detector.h"

class FaceTracker {
public:
    FaceTracker();

    // Update with latest face detection result
    void update(const FaceResult& face);

    // Get normalized gaze offset (-1.0 to 1.0)
    float getGazeX() const;
    float getGazeY() const;

    // Whether a face is currently being tracked
    bool hasFace() const;

    // Reset tracking state
    void reset();

private:
    // EMA smoothed face position (normalized -1..1)
    float smoothX_ = 0.0f;
    float smoothY_ = 0.0f;

    bool facePresent_ = false;
    unsigned long lastFaceTime_ = 0;
};
