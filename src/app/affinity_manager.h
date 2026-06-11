#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "config/app_config.h"

enum class AffinityEventType : uint8_t {
    GENERIC = 0,
    TOUCH,
    AI_LISTENING,
    PHOTO,
    MEMO,
    POMODORO,
    DANCE,
    SHAKE,
    LOW_BATTERY,
    IDLE
};

class AffinityManager {
public:
    void begin();
    void add(int delta, const char* reason);
    void applyInteraction(AffinityEventType type, int bondDelta, int moodDelta, const char* reason);
    void reset();

    int value() const;
    int bondValue() const;
    int moodValue() const;
    const char* levelName() const;
    const char* moodName() const;
    const String& recent() const;
    int recentInteractionCount() const { return recentInteractionCount_; }
    int comboCount() const { return comboCount_; }
    void flush();
    void update();

private:
    void save();
    int clampValue(int value) const;
    bool shouldApplyMood(AffinityEventType type, unsigned long now);
    int eventIndex(AffinityEventType type) const;

    Preferences prefs_;
    int value_ = AFFINITY_DEFAULT_VALUE;
    int moodValue_ = MOOD_DEFAULT_VALUE;
    String recent_ = "First meet";
    bool dirty_ = false;
    unsigned long lastWriteMs_ = 0;
    static constexpr unsigned long WRITE_INTERVAL_MS = 10000;
    static constexpr uint8_t EVENT_COUNT = 10;
    int recentInteractionCount_ = 0;
    unsigned long lastInteractionMs_ = 0;
    unsigned long lastDecayMs_ = 0;
    unsigned long lastMoodDecayMs_ = 0;
    unsigned long lastMoodEventMs_[EVENT_COUNT] = {};
    int comboCount_ = 0;
    unsigned long comboStartMs_ = 0;
};
