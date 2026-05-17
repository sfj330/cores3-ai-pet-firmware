#pragma once

#include <cstdint>
#include <functional>

enum class GestureType {
    NONE,
    SINGLE_TAP,
    DOUBLE_TAP,
    RIGHT_SWIPE,
    LEFT_SWIPE,
    UP_SWIPE,
    DOWN_SWIPE,
    LONG_PRESS
};

struct TouchPoint {
    int x = 0;
    int y = 0;
    bool touched = false;
};

struct GestureEvent {
    GestureType type = GestureType::NONE;
    int startX = 0;
    int startY = 0;
    int endX = 0;
    int endY = 0;
    unsigned long durationMs = 0;
};

class GestureManager {
public:
    using GestureCallback = std::function<void(const GestureEvent&)>;

    GestureManager();

    void update(const TouchPoint& tp);
    void setCallback(GestureCallback cb);
    void reset();

private:
    void detectGesture();

    // Touch lifecycle state
    enum class TouchPhase {
        IDLE,
        TOUCH_START,
        TOUCH_MOVE,
        TOUCH_END
    };

    TouchPhase phase_ = TouchPhase::IDLE;
    TouchPoint startPoint_;
    TouchPoint lastPoint_;
    unsigned long touchStartTime_ = 0;
    unsigned long touchEndTime_ = 0;

    // Double-tap tracking
    unsigned long lastTapTime_ = 0;
    bool pendingSingleTap_ = false;
    unsigned long pendingTapTime_ = 0;
    GestureEvent pendingTapEvent_;

    GestureCallback callback_ = nullptr;
};
