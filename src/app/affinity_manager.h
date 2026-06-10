#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "config/app_config.h"

class AffinityManager {
public:
    void begin();
    void add(int delta, const char* reason);
    void reset();

    int value() const;
    const char* levelName() const;
    const char* moodName() const;
    const String& recent() const;
    void flush();
    void update();

private:
    void save();

    Preferences prefs_;
    int value_ = AFFINITY_DEFAULT_VALUE;
    String recent_ = "First meet";
    bool dirty_ = false;
    unsigned long lastWriteMs_ = 0;
    static constexpr unsigned long WRITE_INTERVAL_MS = 10000;
};
