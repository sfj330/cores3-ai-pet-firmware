#include "event_bus.h"

EventBus& EventBus::instance() {
    static EventBus inst;
    return inst;
}

void EventBus::publish(const Event& event) {
    for (int i = 0; i < listenerCount_; ++i) {
        if (listeners_[i].active && listeners_[i].type == event.type) {
            listeners_[i].listener(event);
        }
    }
}

void EventBus::subscribe(EventType type, EventListener listener) {
    if (listenerCount_ < MAX_LISTENERS) {
        listeners_[listenerCount_].type = type;
        listeners_[listenerCount_].listener = std::move(listener);
        listeners_[listenerCount_].active = true;
        ++listenerCount_;
    }
}
