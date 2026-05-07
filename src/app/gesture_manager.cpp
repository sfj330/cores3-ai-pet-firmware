#include "gesture_manager.h"
#include "config/app_config.h"
#include <Arduino.h>

GestureManager::GestureManager() = default;

void GestureManager::setCallback(GestureCallback cb) {
    callback_ = std::move(cb);
}

void GestureManager::reset() {
    phase_ = TouchPhase::IDLE;
    pendingSingleTap_ = false;
    lastTapTime_ = 0;
    pendingTapTime_ = 0;
    pendingTapEvent_ = GestureEvent{};
}

void GestureManager::update(const TouchPoint& tp) {
    unsigned long now = millis();

    if (tp.touched && phase_ == TouchPhase::IDLE) {
        phase_ = TouchPhase::TOUCH_START;
        startPoint_ = tp;
        lastPoint_ = tp;
        touchStartTime_ = now;
    } else if (tp.touched && phase_ == TouchPhase::TOUCH_START) {
        phase_ = TouchPhase::TOUCH_MOVE;
        lastPoint_ = tp;
    } else if (tp.touched && phase_ == TouchPhase::TOUCH_MOVE) {
        lastPoint_ = tp;
    } else if (!tp.touched && (phase_ == TouchPhase::TOUCH_START || phase_ == TouchPhase::TOUCH_MOVE)) {
        phase_ = TouchPhase::TOUCH_END;
        touchEndTime_ = now;
        detectGesture();
        phase_ = TouchPhase::IDLE;
    }

    if (pendingSingleTap_ && (now - pendingTapTime_ > DOUBLE_TAP_WINDOW_MS)) {
        pendingSingleTap_ = false;
        if (callback_) callback_(pendingTapEvent_);
    }
}

void GestureManager::detectGesture() {
    unsigned long touchDuration = touchEndTime_ - touchStartTime_;
    int dx = lastPoint_.x - startPoint_.x;
    int absDx = abs(dx);
    int absDy = abs(lastPoint_.y - startPoint_.y);

    GestureEvent event;
    event.startX = startPoint_.x;
    event.startY = startPoint_.y;
    event.endX = lastPoint_.x;
    event.endY = lastPoint_.y;
    event.durationMs = touchDuration;

    if (touchDuration >= LONG_PRESS_THRESHOLD_MS &&
        absDx < SWIPE_THRESHOLD_PX && absDy < SWIPE_THRESHOLD_PX) {
        event.type = GestureType::LONG_PRESS;
        pendingSingleTap_ = false;
        if (callback_) callback_(event);
        return;
    }

    if (absDx > SWIPE_THRESHOLD_PX && absDx > absDy * 2) {
        event.type = dx > 0 ? GestureType::RIGHT_SWIPE : GestureType::LEFT_SWIPE;
        pendingSingleTap_ = false;
        if (callback_) callback_(event);
        return;
    }

    if (touchDuration < TAP_THRESHOLD_MS &&
        absDx < MOVE_THRESHOLD_PX && absDy < MOVE_THRESHOLD_PX) {
        unsigned long now = touchEndTime_;
        unsigned long gap = now - lastTapTime_;

        if (gap > 0 && gap < DOUBLE_TAP_WINDOW_MS) {
            event.type = GestureType::DOUBLE_TAP;
            pendingSingleTap_ = false;
            lastTapTime_ = 0;
            if (callback_) callback_(event);
        } else {
            event.type = GestureType::SINGLE_TAP;
            pendingSingleTap_ = true;
            pendingTapTime_ = now;
            lastTapTime_ = now;
            pendingTapEvent_ = event;
        }
    }
}
