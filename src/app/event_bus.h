#pragma once

#include <functional>
#include <cstdint>

enum class EventType : uint8_t {
    GESTURE_EVENT,
    FACE_DETECTED,
    FACE_LOST,
    AI_STATE_CHANGE,
    POWER_EVENT,
    TIMER_EVENT
};

struct Event {
    EventType type;
    int intParam = 0;      // Generic integer payload
    float floatParam = 0.0f; // Generic float payload
    void* dataPtr = nullptr; // Generic pointer payload
};

using EventListener = std::function<void(const Event&)>;

class EventBus {
public:
    static EventBus& instance();

    void publish(const Event& event);
    void subscribe(EventType type, EventListener listener);

private:
    EventBus() = default;

    static constexpr int MAX_LISTENERS = 12;
    struct ListenerEntry {
        EventType type;
        EventListener listener;
        bool active = false;
    };

    ListenerEntry listeners_[MAX_LISTENERS];
    int listenerCount_ = 0;
};
