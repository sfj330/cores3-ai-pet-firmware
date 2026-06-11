#pragma once

#include <cstdint>
#include <M5GFX.h>
#include "config/app_config.h"

struct MemoDisplayEntry {
    char text[61] = {0};
    uint32_t remindAt = 0;
    uint8_t timeBase = 0;
    uint8_t id = 0;
};

enum class MemoHitZone {
    MEMO_HIT_NONE,
    MEMO_HIT_BACK,
    MEMO_HIT_ENTRY_0,
    MEMO_HIT_ENTRY_1,
    MEMO_HIT_ENTRY_2,
    MEMO_HIT_ENTRY_3,
    MEMO_HIT_ENTRY_4,
    MEMO_HIT_ENTRY_5,
    MEMO_HIT_ENTRY_6,
    MEMO_HIT_ENTRY_7,
    MEMO_HIT_CLOSE,
    MEMO_HIT_DELETE
};

class MemoUI {
public:
    MemoUI();
    void begin();
    void show();
    void hide();
    void update();
    void markDirty();
    void setMemos(const MemoDisplayEntry* memos, int count, uint32_t wallNow, uint32_t uptimeNow);
    MemoHitZone hitTest(int x, int y) const;

    void setBackPressed() { backPressedUntil_ = millis() + 150; dirty_ = true; }

    void showDetail(int index);
    void showList();
    uint8_t takePendingDeleteId();
    bool isDetailView() const { return detailView_; }
    uint8_t detailEntryId() const;

private:
    void drawRow(int y, int index, const MemoDisplayEntry& entry);
    void drawBackButton();
    void drawDetailView();
    void drawList();
    void formatRemaining(const MemoDisplayEntry& entry, char* buf, int bufLen);

    M5Canvas canvas_;
    bool spriteReady_ = false;
    bool visible_ = false;
    bool dirty_ = true;
    unsigned long backPressedUntil_ = 0;
    MemoDisplayEntry memos_[MAX_MEMO_COUNT];
    int memoCount_ = 0;
    uint32_t memoWallNow_ = 0;
    uint32_t memoUptimeNow_ = 0;
    bool detailView_ = false;
    int detailIndex_ = -1;
    uint8_t pendingDeleteId_ = 0;

    static constexpr int BACK_X = 5;
    static constexpr int BACK_Y = 5;
    static constexpr int BACK_W = 74;
    static constexpr int BACK_H = 26;
    static constexpr int ROW_START_Y = 50;
    static constexpr int ROW_H = 22;
    static constexpr int CLOSE_X = 5;
    static constexpr int CLOSE_Y = 5;
    static constexpr int CLOSE_W = 74;
    static constexpr int CLOSE_H = 26;
    static constexpr int DELETE_X = 90;
    static constexpr int DELETE_Y = 200;
    static constexpr int DELETE_W = 140;
    static constexpr int DELETE_H = 28;
};
