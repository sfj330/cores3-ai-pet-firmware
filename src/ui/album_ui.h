#pragma once

#include <Arduino.h>
#include <M5GFX.h>
#include "config/app_config.h"

enum class AlbumViewMode {
    GRID,
    FULLVIEW
};

enum class AlbumHitZone {
    NONE,
    BACK,
    THUMBNAIL,
    DELETE
};

class AlbumUI {
public:
    AlbumUI();
    void begin();
    void show();
    void hide();
    void update();
    void markDirty();

    void scanPhotos();
    int photoCount() const { return fileCount_; }

    AlbumViewMode viewMode() const { return viewMode_; }
    void setViewMode(AlbumViewMode mode);

    void scrollUp();
    void scrollDown();

    void showPhoto(int index);
    void nextPhoto();
    void prevPhoto();
    int currentIndex() const { return currentPhotoIndex_; }

    bool deleteCurrentPhoto();

    AlbumHitZone hitTest(int x, int y) const;
    int thumbnailIndexAt(int x, int y) const;

    void setBackPressed() { backPressedUntil_ = millis() + 150; dirty_ = true; }

private:
    void drawGrid();
    void drawFullView();
    void drawEmptyState();

    M5Canvas canvas_;
    bool spriteReady_ = false;
    bool visible_ = false;
    bool dirty_ = true;
    unsigned long backPressedUntil_ = 0;

    AlbumViewMode viewMode_ = AlbumViewMode::GRID;

    static constexpr int MAX_PHOTOS = 200;
    String fileList_[MAX_PHOTOS];
    int fileCount_ = 0;

    int gridOffset_ = 0;
    int currentPhotoIndex_ = 0;

    static constexpr int GRID_COLS = 4;
    static constexpr int GRID_ROWS = 3;
    static constexpr int GRID_MARGIN = 4;
    static constexpr int GRID_TOP_Y = 36;
    static constexpr int CELL_W = (DISPLAY_WIDTH - GRID_MARGIN * (GRID_COLS + 1)) / GRID_COLS;
    static constexpr int CELL_H = (DISPLAY_HEIGHT - GRID_TOP_Y - 30 - GRID_MARGIN * (GRID_ROWS + 1)) / GRID_ROWS;
    static constexpr int PAGE_SIZE = GRID_COLS * GRID_ROWS;

    int thumbsDecoded_ = 0;

    static constexpr int BACK_X = 5;
    static constexpr int BACK_Y = 5;
    static constexpr int BACK_W = 74;
    static constexpr int BACK_H = 26;

    static constexpr int DEL_X = DISPLAY_WIDTH - 64;
    static constexpr int DEL_Y = DISPLAY_HEIGHT - 28;
    static constexpr int DEL_W = 58;
    static constexpr int DEL_H = 24;
};
