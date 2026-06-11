#include "app/memo_manager.h"

static constexpr const char* NVS_NAMESPACE = "memo";
static constexpr const char* NVS_KEY_COUNT = "cnt";
static constexpr const char* NVS_KEY_NEXT_ID = "nid";

namespace {
struct LegacyMemoEntry {
    uint8_t id = 0;
    char text[MAX_MEMO_TEXT_LEN + 1] = {0};
    uint32_t remindAt = 0;
};

bool isDue(const MemoEntry& entry, uint32_t wallNow, uint32_t uptimeNow) {
    if (entry.remindAt == 0) return false;
    if (entry.timeBase == 2) return uptimeNow >= entry.remindAt;
    return wallNow > 1700000000UL && wallNow >= entry.remindAt;
}
}

void MemoManager::begin() {
    prefs_.begin(NVS_NAMESPACE, false);
    load();
}

void MemoManager::load() {
    count_ = prefs_.getInt(NVS_KEY_COUNT, 0);
    nextId_ = prefs_.getInt(NVS_KEY_NEXT_ID, 1);
    if (count_ < 0) count_ = 0;
    if (count_ > MAX_MEMO_COUNT) count_ = MAX_MEMO_COUNT;

    for (int i = 0; i < count_; ++i) {
        char key[4];
        snprintf(key, sizeof(key), "e%d", i);
        size_t len = prefs_.getBytes(key, &entries_[i], sizeof(MemoEntry));
        if (len == sizeof(MemoEntry)) {
            if (entries_[i].remindAt == 0) {
                entries_[i].timeBase = 0;
            } else if (entries_[i].timeBase == 0) {
                entries_[i].timeBase = 1;
            } else if (entries_[i].timeBase == 2) {
                entries_[i].remindAt = 0;
                entries_[i].timeBase = 0;
                dirty_ = true;
            }
        } else if (len == sizeof(LegacyMemoEntry)) {
            LegacyMemoEntry legacy;
            prefs_.getBytes(key, &legacy, sizeof(LegacyMemoEntry));
            memset(&entries_[i], 0, sizeof(MemoEntry));
            entries_[i].id = legacy.id;
            strncpy(entries_[i].text, legacy.text, MAX_MEMO_TEXT_LEN);
            entries_[i].text[MAX_MEMO_TEXT_LEN] = '\0';
            entries_[i].remindAt = legacy.remindAt;
            entries_[i].timeBase = legacy.remindAt > 0 ? 1 : 0;
            dirty_ = true;
        } else {
            memset(&entries_[i], 0, sizeof(MemoEntry));
        }
    }
    if (dirty_) {
        save();
    }
    dirty_ = false;
    lastWriteMs_ = millis();
}

void MemoManager::save() {
    prefs_.putInt(NVS_KEY_COUNT, count_);
    prefs_.putInt(NVS_KEY_NEXT_ID, nextId_);
    for (int i = 0; i < count_; ++i) {
        char key[4];
        snprintf(key, sizeof(key), "e%d", i);
        prefs_.putBytes(key, &entries_[i], sizeof(MemoEntry));
    }
}

bool MemoManager::add(const char* text, uint32_t remindAt, uint8_t timeBase) {
    if (count_ >= MAX_MEMO_COUNT) return false;

    MemoEntry& e = entries_[count_];
    memset(&e, 0, sizeof(MemoEntry));
    e.id = nextId_++;
    strncpy(e.text, text, MAX_MEMO_TEXT_LEN);
    e.text[MAX_MEMO_TEXT_LEN] = '\0';
    // Walk back to avoid splitting a UTF-8 character at the boundary
    for (int i = MAX_MEMO_TEXT_LEN - 1; i >= 0; --i) {
        unsigned char c = static_cast<unsigned char>(e.text[i]);
        if (c == 0) continue;
        if ((c & 0xC0) == 0x80) {
            // Continuation byte — part of a truncated multi-byte char, zero it
            e.text[i] = '\0';
        } else {
            break; // Found a lead byte or ASCII char — safe boundary
        }
    }
    e.remindAt = remindAt;
    e.timeBase = remindAt > 0 ? timeBase : 0;
    ++count_;
    dirty_ = true;
    return true;
}

bool MemoManager::remove(uint8_t id) {
    for (int i = 0; i < count_; ++i) {
        if (entries_[i].id == id) {
            // Shift remaining entries down
            for (int j = i; j < count_ - 1; ++j) {
                entries_[j] = entries_[j + 1];
            }
            --count_;
            memset(&entries_[count_], 0, sizeof(MemoEntry));
            dirty_ = true;
            return true;
        }
    }
    return false;
}

bool MemoManager::removeAtIndex(int index) {
    const MemoEntry* e = entry(index);
    if (!e) return false;
    return remove(e->id);
}

int MemoManager::count() const {
    return count_;
}

const MemoEntry* MemoManager::entry(int index) const {
    if (index < 0 || index >= count_) return nullptr;
    return &entries_[index];
}

void MemoManager::checkDueReminders(uint32_t wallNow, uint32_t uptimeNow, MemoEntry* out, int maxOut, int& outCount) {
    outCount = 0;
    for (int i = 0; i < count_ && outCount < maxOut; ++i) {
        if (isDue(entries_[i], wallNow, uptimeNow)) {
            out[outCount++] = entries_[i];
        }
    }
}

bool MemoManager::hasDueReminders(uint32_t wallNow, uint32_t uptimeNow) const {
    for (int i = 0; i < count_; ++i) {
        if (isDue(entries_[i], wallNow, uptimeNow)) {
            return true;
        }
    }
    return false;
}

void MemoManager::clearReminder(uint8_t id) {
    for (int i = 0; i < count_; ++i) {
        if (entries_[i].id == id) {
            entries_[i].remindAt = 0;
            entries_[i].timeBase = 0;
            dirty_ = true;
            return;
        }
    }
}

void MemoManager::flush() {
    if (dirty_) {
        save();
        dirty_ = false;
        lastWriteMs_ = millis();
    }
}

void MemoManager::update() {
    if (dirty_ && millis() - lastWriteMs_ >= WRITE_INTERVAL_MS) {
        flush();
    }
}
