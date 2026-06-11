#include "app/affinity_manager.h"

static constexpr const char* NVS_NAMESPACE = "affinity";
static constexpr const char* NVS_KEY_VALUE = "val";

void AffinityManager::begin() {
    prefs_.begin(NVS_NAMESPACE, false);
    value_ = prefs_.getInt(NVS_KEY_VALUE, AFFINITY_DEFAULT_VALUE);
    if (value_ < AFFINITY_MIN_VALUE) value_ = AFFINITY_MIN_VALUE;
    if (value_ > AFFINITY_MAX_VALUE) value_ = AFFINITY_MAX_VALUE;
    recent_ = "Ready";
    dirty_ = false;
    lastWriteMs_ = millis();
    recentInteractionCount_ = 0;
    lastInteractionMs_ = 0;
    lastDecayMs_ = millis();
    comboCount_ = 0;
    comboStartMs_ = 0;
    Serial.printf("Affinity loaded: %d\n", value_);
}

void AffinityManager::add(int delta, const char* reason) {
    int prev = value_;
    value_ += delta;
    if (value_ < AFFINITY_MIN_VALUE) value_ = AFFINITY_MIN_VALUE;
    if (value_ > AFFINITY_MAX_VALUE) value_ = AFFINITY_MAX_VALUE;
    if (reason != nullptr && reason[0] != '\0') {
        recent_ = reason;
    }
    if (value_ != prev) {
        dirty_ = true;
    }

    // Update interaction tracking
    unsigned long now = millis();
    recentInteractionCount_++;
    lastInteractionMs_ = now;

    // Combo tracking
    if (now - comboStartMs_ < COMBO_WINDOW_MS) {
        comboCount_++;
    } else {
        comboCount_ = 1;
        comboStartMs_ = now;
    }
}

void AffinityManager::reset() {
    value_ = AFFINITY_DEFAULT_VALUE;
    recent_ = "Reset";
    dirty_ = true;
}

void AffinityManager::save() {
    prefs_.putInt(NVS_KEY_VALUE, value_);
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
    if (recentInteractionCount_ >= MOOD_THRESHOLD_LIVELY) return "Lively";
    if (recentInteractionCount_ >= MOOD_THRESHOLD_HAPPY) return "Happy";
    if (recentInteractionCount_ >= MOOD_THRESHOLD_WARM) return "Warm";
    if (recentInteractionCount_ >= MOOD_THRESHOLD_QUIET) return "Quiet";
    return "Lonely";
}

const String& AffinityManager::recent() const {
    return recent_;
}

void AffinityManager::flush() {
    if (dirty_) {
        save();
        dirty_ = false;
        lastWriteMs_ = millis();
    }
}

void AffinityManager::update() {
    unsigned long now = millis();

    // Decay interaction count when outside the 30-minute window
    if (recentInteractionCount_ > 0 && lastInteractionMs_ > 0 &&
        now - lastInteractionMs_ >= MOOD_WINDOW_MS) {
        recentInteractionCount_ = 0;
        comboCount_ = 0;
    }

    // Natural affinity decay: -1 per hour of no interaction, down to default
    if (lastInteractionMs_ > 0 && value_ > AFFINITY_DEFAULT_VALUE &&
        now - lastInteractionMs_ >= AFFINITY_DECAY_INTERVAL_MS) {
        if (now - lastDecayMs_ >= AFFINITY_DECAY_INTERVAL_MS) {
            value_--;
            if (value_ < AFFINITY_DEFAULT_VALUE) value_ = AFFINITY_DEFAULT_VALUE;
            dirty_ = true;
            lastDecayMs_ = now;
        }
    } else {
        lastDecayMs_ = now;  // Reset decay timer on any interaction
    }

    // Existing deferred write logic
    if (dirty_ && millis() - lastWriteMs_ >= WRITE_INTERVAL_MS) {
        flush();
    }
}
