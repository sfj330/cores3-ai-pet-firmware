#include "ui/memo_ui.h"
#include "ui/ui_theme.h"
#include <M5CoreS3.h>
#include <lgfx/v1/lgfx_fonts.hpp>

namespace {
// Return the byte length of a UTF-8 character from its lead byte.
int utf8CharLen(unsigned char lead) {
    if (lead < 0x80) return 1;
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 1; // invalid lead byte, treat as single byte
}

// Truncate a String at a safe UTF-8 boundary (<= maxBytes).
// Appends "..." if truncation occurs.
String utf8Truncate(const String &str, int maxBytes) {
    if ((int)str.length() <= maxBytes) return str;
    // Walk back from maxBytes to find a safe boundary
    int cut = maxBytes;
    while (cut > 0 && (static_cast<unsigned char>(str.charAt(cut)) & 0xC0) == 0x80) {
        --cut;
    }
    // Make room for "..."
    while (cut > 0 && cut + 3 > maxBytes) {
        --cut;
        while (cut > 0 && (static_cast<unsigned char>(str.charAt(cut)) & 0xC0) == 0x80) {
            --cut;
        }
    }
    return str.substring(0, cut) + "...";
}
} // namespace

MemoUI::MemoUI() : canvas_(&M5.Lcd) {}

void MemoUI::begin()
{
    canvas_.setPsram(true);
    canvas_.setColorDepth(16);
    spriteReady_ = canvas_.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT) != nullptr;
    if (spriteReady_)
    {
        canvas_.fillSprite(UiTheme::BG);
    }
}

void MemoUI::show()
{
    visible_ = true;
    dirty_ = true;
}

void MemoUI::hide()
{
    visible_ = false;
}

void MemoUI::markDirty()
{
    dirty_ = true;
}

void MemoUI::setMemos(const MemoDisplayEntry *memos, int count, uint32_t wallNow, uint32_t uptimeNow)
{
    int n = count > MAX_MEMO_COUNT ? MAX_MEMO_COUNT : count;
    if (n < 0)
        n = 0;
    for (int i = 0; i < n; i++)
    {
        memos_[i] = memos[i];
    }
    memoCount_ = n;
    memoWallNow_ = wallNow;
    memoUptimeNow_ = uptimeNow;
    dirty_ = true;
}

MemoHitZone MemoUI::hitTest(int x, int y) const
{
    if (detailView_)
    {
        if (x >= CLOSE_X && x < CLOSE_X + CLOSE_W && y >= CLOSE_Y && y < CLOSE_Y + CLOSE_H)
        {
            return MemoHitZone::MEMO_HIT_CLOSE;
        }
        if (x >= DELETE_X && x < DELETE_X + DELETE_W && y >= DELETE_Y && y < DELETE_Y + DELETE_H)
        {
            return MemoHitZone::MEMO_HIT_DELETE;
        }
    }
    else
    {
        if (x >= BACK_X && x < BACK_X + BACK_W && y >= BACK_Y && y < BACK_Y + BACK_H)
        {
            return MemoHitZone::MEMO_HIT_BACK;
        }
        for (int i = 0; i < memoCount_ && i < MAX_MEMO_COUNT; i++)
        {
            int ry = ROW_START_Y + i * ROW_H;
            if (x >= 8 && x < 312 && y >= ry && y < ry + ROW_H)
            {
                return static_cast<MemoHitZone>(static_cast<int>(MemoHitZone::MEMO_HIT_ENTRY_0) + i);
            }
        }
    }
    return MemoHitZone::MEMO_HIT_NONE;
}

void MemoUI::update()
{
    if (!visible_ || !spriteReady_)
        return;
    if (millis() < backPressedUntil_)
        dirty_ = true;
    if (!dirty_)
        return;

    canvas_.fillSprite(UiTheme::BG);

    if (detailView_)
    {
        drawDetailView();
    }
    else
    {
        drawList();
    }

    canvas_.pushSprite(0, 0);
    dirty_ = false;
}

void MemoUI::drawList()
{
    UiTheme::drawTitle(canvas_, "Memos", "AI memos & reminders", UiTheme::AMBER);
    if (memoCount_ == 0)
    {
        canvas_.setTextDatum(MC_DATUM);
        canvas_.setTextSize(1);
        canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::BG);
        canvas_.drawString("No memos yet", DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
        canvas_.setTextDatum(TL_DATUM);
    }
    else
    {
        for (int i = 0; i < memoCount_; i++)
        {
            int y = ROW_START_Y + i * ROW_H;
            drawRow(y, i, memos_[i]);
        }
    }
    drawBackButton();
}

void MemoUI::drawRow(int y, int index, const MemoDisplayEntry &entry)
{
    (void)index;
    canvas_.fillRoundRect(8, y, 304, 20, 5, UiTheme::PANEL);

    uint16_t accentColor = entry.remindAt > 0 ? UiTheme::AMBER : UiTheme::TEXT_DIM;
    canvas_.drawFastVLine(16, y + 4, 12, accentColor);

    canvas_.setTextDatum(TL_DATUM);
    canvas_.setTextSize(1);

    // Display stable memo id rather than the row index.
    canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::PANEL);
    canvas_.setCursor(26, y + 5);
    canvas_.printf("#%d", entry.id);

    // Memo text (truncated to fit — efontCN_10 CJK chars are wider)
    canvas_.setFont(&lgfx::v1::fonts::efontCN_10);
    String displayText = utf8Truncate(String(entry.text), 18);
    canvas_.setTextColor(UiTheme::TEXT, UiTheme::PANEL);
    canvas_.setCursor(50, y + 5);
    canvas_.print(displayText);
    canvas_.setFont(&lgfx::v1::fonts::Font0);

    // Reminder time indicator
    if (entry.remindAt > 0)
    {
        char buf[8] = {0};
        formatRemaining(entry, buf, sizeof(buf));
        canvas_.setTextColor(UiTheme::AMBER, UiTheme::PANEL);
        canvas_.setTextDatum(TR_DATUM);
        canvas_.drawString(buf, 304, y + 5);
        canvas_.setTextDatum(TL_DATUM);
    }
}

void MemoUI::drawBackButton()
{
    UiTheme::drawBackButton(canvas_, BACK_X, BACK_Y, BACK_W, BACK_H, millis() < backPressedUntil_);
}

void MemoUI::formatRemaining(const MemoDisplayEntry &entry, char *buf, int bufLen)
{
    uint32_t now = entry.timeBase == 2 ? memoUptimeNow_ : memoWallNow_;
    if (now == 0 || (entry.timeBase == 1 && now < 1700000000UL))
    {
        snprintf(buf, bufLen, "?");
        return;
    }
    int32_t diff = (int32_t)(entry.remindAt - now);
    if (diff < 0)
        diff = 0;
    if (diff < 3600)
    {
        snprintf(buf, bufLen, "%dm", diff / 60);
    }
    else if (diff < 86400)
    {
        snprintf(buf, bufLen, "%dh", diff / 3600);
    }
    else
    {
        snprintf(buf, bufLen, "%dd", diff / 86400);
    }
}

void MemoUI::showDetail(int index)
{
    if (index >= 0 && index < memoCount_)
    {
        detailView_ = true;
        detailIndex_ = index;
        dirty_ = true;
    }
}

void MemoUI::showList()
{
    detailView_ = false;
    detailIndex_ = -1;
    dirty_ = true;
}

uint8_t MemoUI::takePendingDeleteId()
{
    uint8_t id = pendingDeleteId_;
    pendingDeleteId_ = 0;
    return id;
}

uint8_t MemoUI::detailEntryId() const
{
    if (detailView_ && detailIndex_ >= 0 && detailIndex_ < memoCount_)
    {
        return memos_[detailIndex_].id;
    }
    return 0;
}

void MemoUI::drawDetailView()
{
    if (detailIndex_ < 0 || detailIndex_ >= memoCount_)
        return;
    const auto &entry = memos_[detailIndex_];

    // Close button (top-left, same style as back)
    UiTheme::drawBackButton(canvas_, CLOSE_X, CLOSE_Y, CLOSE_W, CLOSE_H, millis() < backPressedUntil_);

    // Title
    canvas_.setTextDatum(TL_DATUM);
    canvas_.setTextSize(1);
    canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::BG);
    canvas_.setCursor(90, 10);
    canvas_.printf("Memo #%d", entry.id);

    // Full text in a panel
    int panelX = 10, panelY = 42, panelW = 300, panelH = 120;
    canvas_.fillRoundRect(panelX, panelY, panelW, panelH, 8, UiTheme::PANEL);
    canvas_.drawRoundRect(panelX, panelY, panelW, panelH, 8, UiTheme::PANEL_LIGHT);

    canvas_.setTextColor(UiTheme::TEXT, UiTheme::PANEL);
    canvas_.setTextSize(1);
    canvas_.setFont(&lgfx::v1::fonts::efontCN_14);
    // Word-wrap the text within the panel (UTF-8 aware iteration)
    int cursorX = panelX + 8;
    int cursorY = panelY + 8;
    int maxX = panelX + panelW - 8;
    int maxY = panelY + panelH - 8;
    for (int i = 0; entry.text[i] != '\0' && cursorY < maxY; )
    {
        int clen = utf8CharLen(static_cast<unsigned char>(entry.text[i]));
        // Build the full UTF-8 character string
        String ch;
        for (int j = 0; j < clen && entry.text[i + j] != '\0'; ++j) {
            ch += entry.text[i + j];
        }
        int charW = canvas_.textWidth(ch);
        if (cursorX + charW > maxX && ch.charAt(0) != ' ')
        {
            cursorX = panelX + 8;
            cursorY += 16;
        }
        if (cursorY < maxY)
        {
            canvas_.setCursor(cursorX, cursorY);
            canvas_.print(ch);
            cursorX += charW;
        }
        i += clen;
    }
    canvas_.setFont(&lgfx::v1::fonts::Font0);

    // Reminder info
    int infoY = panelY + panelH + 12;
    canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::BG);
    canvas_.setCursor(10, infoY);
    if (entry.remindAt > 0)
    {
        char buf[16] = {0};
        formatRemaining(entry, buf, sizeof(buf));
        canvas_.printf("Reminder: %s", buf);
    }
    else
    {
        canvas_.print("No reminder");
    }

    // Delete button (red accent)
    canvas_.fillRoundRect(DELETE_X, DELETE_Y, DELETE_W, DELETE_H, 6, UiTheme::RED);
    canvas_.drawRoundRect(DELETE_X, DELETE_Y, DELETE_W, DELETE_H, 6, 0xF9A0); // darker red border
    canvas_.setTextDatum(MC_DATUM);
    canvas_.setTextSize(1);
    canvas_.setTextColor(UiTheme::BG, UiTheme::RED);
    canvas_.drawString("Delete", DELETE_X + DELETE_W / 2, DELETE_Y + DELETE_H / 2 + 1);
    canvas_.setTextDatum(TL_DATUM);
}
