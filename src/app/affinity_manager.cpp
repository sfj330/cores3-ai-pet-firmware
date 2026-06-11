#include "app/affinity_manager.h"

static constexpr const char* NVS_NAMESPACE = "affinity";
static constexpr const char* NVS_KEY_VALUE = "val";
static constexpr const char* NVS_KEY_MOOD = "mood";

void AffinityManager::begin() {
    prefs_.begin(NVS_NAMESPACE, false);
    value_ = prefs_.getInt(NVS_KEY_VALUE, AFFINITY_DEFAULT_VALUE);
    value_ = clampValue(value_);
    moodValue_ = prefs_.getInt(NVS_KEY_MOOD, MOOD_DEFAULT_VALUE);
    moodValue_ = clampValue(moodValue_);
    recent_ = "Ready";
    dirty_ = false;
    lastWriteMs_ = millis();
    recentInteractionCount_ = 0;
    lastInteractionMs_ = 0;
    lastDecayMs_ = millis();
    lastMoodDecayMs_ = millis();
    memset(lastMoodEventMs_, 0, sizeof(lastMoodEventMs_));
    comboCount_ = 0;
    comboStartMs_ = 0;
    Serial.printf("Affinity loaded: bond=%d mood=%d\n", value_, moodValue_);
}

void AffinityManager::add(int delta, const char* reason) {
    applyInteraction(AffinityEventType::GENERIC, delta, delta, reason);
}

void AffinityManager::applyInteraction(AffinityEventType type, int bondDelta, int moodDelta, const char* reason) {
    int prevBond = value_;
    int prevMood = moodValue_;
    value_ = clampValue(value_ + bondDelta);

    unsigned long now = millis();
    if (shouldApplyMood(type, now)) {
        moodValue_ = clampValue(moodValue_ + moodDelta);
    }

    if (reason != nullptr && reason[0] != '\0') {
        recent_ = reason;
    }
    if (value_ != prevBond || moodValue_ != prevMood) {
        dirty_ = true;
    }

    recentInteractionCount_++;
    lastInteractionMs_ = now;

    if (now - comboStartMs_ < COMBO_WINDOW_MS) {
        comboCount_++;
    } else {
        comboCount_ = 1;
        comboStartMs_ = now;
    }
}

void AffinityManager::reset() {
    value_ = AFFINITY_DEFAULT_VALUE;
    moodValue_ = MOOD_DEFAULT_VALUE;
    recent_ = "Reset";
    dirty_ = true;
}

void AffinityManager::save() {
    prefs_.putInt(NVS_KEY_VALUE, value_);
    prefs_.putInt(NVS_KEY_MOOD, moodValue_);
}

int AffinityManager::value() const {
    return value_;
}

int AffinityManager::bondValue() const {
    return value_;
}

int AffinityManager::moodValue() const {
    return moodValue_;
}

const char* AffinityManager::levelName() const {
    if (value_ < 25) return "Shy";
    if (value_ < 50) return "Familiar";
    if (value_ < 75) return "Close";
    return "Best Friend";
}

const char* AffinityManager::moodName() const {
    if (moodValue_ >= 85) return "Lively";
    if (moodValue_ >= 70) return "Happy";
    if (moodValue_ >= 50) return "Warm";
    if (moodValue_ >= 30) return "Quiet";
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

    if (now - lastMoodDecayMs_ >= MOOD_DECAY_INTERVAL_MS) {
        int prevMood = moodValue_;
        if (lastInteractionMs_ == 0 || now - lastInteractionMs_ >= MOOD_DECAY_INTERVAL_MS) {
            if (moodValue_ > MOOD_DEFAULT_VALUE) {
                moodValue_--;
            } else if (moodValue_ > 20 && now - lastInteractionMs_ >= MOOD_WINDOW_MS) {
                moodValue_--;
            } else if (moodValue_ < MOOD_DEFAULT_VALUE && now - lastInteractionMs_ < MOOD_WINDOW_MS) {
                moodValue_++;
            }
        }
        if (moodValue_ != prevMood) {
            dirty_ = true;
        }
        lastMoodDecayMs_ = now;
    }

    // Existing deferred write logic
    if (dirty_ && millis() - lastWriteMs_ >= WRITE_INTERVAL_MS) {
        flush();
    }
}

int AffinityManager::clampValue(int value) const {
    if (value < AFFINITY_MIN_VALUE) return AFFINITY_MIN_VALUE;
    if (value > AFFINITY_MAX_VALUE) return AFFINITY_MAX_VALUE;
    return value;
}

int AffinityManager::eventIndex(AffinityEventType type) const {
    int idx = static_cast<int>(type);
    if (idx < 0 || idx >= EVENT_COUNT) return 0;
    return idx;
}

bool AffinityManager::shouldApplyMood(AffinityEventType type, unsigned long now) {
    int idx = eventIndex(type);
    bool negativeOrIdle = (type == AffinityEventType::SHAKE ||
                           type == AffinityEventType::LOW_BATTERY ||
                           type == AffinityEventType::IDLE);
    if (!negativeOrIdle && lastMoodEventMs_[idx] > 0 &&
        now - lastMoodEventMs_[idx] < MOOD_EVENT_COOLDOWN_MS) {
        return false;
    }
    lastMoodEventMs_[idx] = now;
    return true;
}
