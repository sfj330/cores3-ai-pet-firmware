#pragma once

#include <cstdint>
#include <M5GFX.h>
#include "config/app_config.h"

struct MemoDisplayEntry {
    char text[61] = {0};
    uint32_t remindAt = 0;
};

enum class MemoHitZone {
    MEMO_HIT_NONE,
    MEMO_HIT_BACK
};

class MemoUI {
public:
    MemoUI();
    void begin();
    void show();
    void hide();
    void update();
    void markDirty();
    void setMemos(const MemoDisplayEntry* memos, int count, uint32_t now);
    MemoHitZone hitTest(int x, int y) const;

private:
    void drawRow(int y, int index, const MemoDisplayEntry& entry);
    void drawBackButton();
    void formatRemaining(uint32_t remindAt, uint32_t now, char* buf, int bufLen);

    M5Canvas canvas_;
    bool spriteReady_ = false;
    bool visible_ = false;
    bool dirty_ = true;
    MemoDisplayEntry memos_[MAX_MEMO_COUNT];
    int memoCount_ = 0;
    uint32_t memoNow_ = 0;

    static constexpr int BACK_X = 8;
    static constexpr int BACK_Y = DISPLAY_HEIGHT - 32;
    static constexpr int BACK_W = 76;
    static constexpr int BACK_H = 24;
    static constexpr int ROW_START_Y = 50;
    static constexpr int ROW_H = 22;
};
