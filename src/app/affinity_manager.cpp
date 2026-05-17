#include "app/affinity_manager.h"

void AffinityManager::begin() {
    value_ = AFFINITY_DEFAULT_VALUE;
    recent_ = "Ready";
}

void AffinityManager::add(int delta, const char* reason) {
    value_ += delta;
    if (value_ < AFFINITY_MIN_VALUE) value_ = AFFINITY_MIN_VALUE;
    if (value_ > AFFINITY_MAX_VALUE) value_ = AFFINITY_MAX_VALUE;
    if (reason != nullptr && reason[0] != '\0') {
        recent_ = reason;
    }
}

void AffinityManager::reset() {
    value_ = AFFINITY_DEFAULT_VALUE;
    recent_ = "Reset";
}

int AffinityManager::value() const {
    return value_;
}

const char* AffinityManager::levelName() const {
    if (value_ < 25) return "Shy";
    if (value_ < 50) return "Familiar";
    if (value_ < 75) return "Close";
    return "Best Friend";
}

const char* AffinityManager::moodName() const {
    if (value_ < 25) return "Quiet";
    if (value_ < 50) return "Warm";
    if (value_ < 75) return "Happy";
    return "Lively";
}

const String& AffinityManager::recent() const {
    return recent_;
}
