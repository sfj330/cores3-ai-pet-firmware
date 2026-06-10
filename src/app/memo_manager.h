#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "config/app_config.h"

struct MemoEntry {
    uint8_t id = 0;
    char text[MAX_MEMO_TEXT_LEN + 1] = {0};
    uint32_t remindAt = 0;  // Unix timestamp, 0 = no reminder
};

class MemoManager {
public:
    void begin();
    bool add(const char* text, uint32_t remindAt = 0);
    bool remove(uint8_t id);
    int count() const;
    const MemoEntry* entry(int index) const;
    void checkDueReminders(uint32_t now, MemoEntry* out, int maxOut, int& outCount);
    bool hasDueReminders(uint32_t now) const;
    void clearReminder(uint8_t id);
    void flush();
    void update();

private:
    void load();
    void save();

    uint8_t nextId_ = 1;
    int count_ = 0;
    MemoEntry entries_[MAX_MEMO_COUNT];
    Preferences prefs_;
    bool dirty_ = false;
    unsigned long lastWriteMs_ = 0;
    static constexpr unsigned long WRITE_INTERVAL_MS = 5000;
};
