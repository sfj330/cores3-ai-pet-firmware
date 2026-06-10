#include "app/memo_manager.h"

static constexpr const char* NVS_NAMESPACE = "memo";
static constexpr const char* NVS_KEY_COUNT = "cnt";
static constexpr const char* NVS_KEY_NEXT_ID = "nid";

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
        if (len != sizeof(MemoEntry)) {
            memset(&entries_[i], 0, sizeof(MemoEntry));
        }
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

bool MemoManager::add(const char* text, uint32_t remindAt) {
    if (count_ >= MAX_MEMO_COUNT) return false;

    MemoEntry& e = entries_[count_];
    memset(&e, 0, sizeof(MemoEntry));
    e.id = nextId_++;
    strncpy(e.text, text, MAX_MEMO_TEXT_LEN);
    e.text[MAX_MEMO_TEXT_LEN] = '\0';
    e.remindAt = remindAt;
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

int MemoManager::count() const {
    return count_;
}

const MemoEntry* MemoManager::entry(int index) const {
    if (index < 0 || index >= count_) return nullptr;
    return &entries_[index];
}

void MemoManager::checkDueReminders(uint32_t now, MemoEntry* out, int maxOut, int& outCount) {
    outCount = 0;
    for (int i = 0; i < count_ && outCount < maxOut; ++i) {
        if (entries_[i].remindAt > 0 && now >= entries_[i].remindAt) {
            out[outCount++] = entries_[i];
        }
    }
}

bool MemoManager::hasDueReminders(uint32_t now) const {
    for (int i = 0; i < count_; ++i) {
        if (entries_[i].remindAt > 0 && now >= entries_[i].remindAt) {
            return true;
        }
    }
    return false;
}

void MemoManager::clearReminder(uint8_t id) {
    for (int i = 0; i < count_; ++i) {
        if (entries_[i].id == id) {
            entries_[i].remindAt = 0;
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
