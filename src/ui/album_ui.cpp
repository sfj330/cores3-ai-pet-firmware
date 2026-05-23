#include "album_ui.h"
#include "ui/ui_theme.h"
#include "storage/storage_manager.h"
#include <M5CoreS3.h>
#include <SD.h>

extern StorageManager gStorageManager;

static bool drawJpgFromSd(M5Canvas& canvas, const char* path,
                           int x, int y, int maxW, int maxH) {
    File f = SD.open(path, FILE_READ);
    if (!f) return false;
    size_t size = f.size();
    if (size == 0 || size > 200000) { f.close(); return false; }
    uint8_t* buf = (uint8_t*)ps_malloc(size);
    if (!buf) { f.close(); return false; }
    f.read(buf, size);
    f.close();
    canvas.drawJpg(buf, size, x, y, maxW, maxH, 0, 0);
    free(buf);
    return true;
}

AlbumUI::AlbumUI() : canvas_(&M5.Lcd) {}

void AlbumUI::begin() {
    canvas_.setPsram(true);
    canvas_.setColorDepth(16);
    spriteReady_ = canvas_.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT) != nullptr;
    if (spriteReady_) {
        canvas_.fillSprite(UiTheme::BG);
    }
}

void AlbumUI::show() {
    visible_ = true;
    dirty_ = true;
    viewMode_ = AlbumViewMode::GRID;
    gridOffset_ = 0;
    thumbsDecoded_ = 0;
}

void AlbumUI::hide() {
    visible_ = false;
}

void AlbumUI::markDirty() {
    dirty_ = true;
}

void AlbumUI::setViewMode(AlbumViewMode mode) {
    viewMode_ = mode;
    dirty_ = true;
    if (mode == AlbumViewMode::GRID) {
        thumbsDecoded_ = 0;
    }
}

void AlbumUI::scanPhotos() {
    fileCount_ = 0;
    File dir = SD.open(PHOTO_DIR);
    if (!dir || !dir.isDirectory()) return;

    File entry;
    while ((entry = dir.openNextFile()) && fileCount_ < MAX_PHOTOS) {
        String name = entry.name();
        if (name.endsWith(".jpg") || name.endsWith(".JPG")) {
            fileList_[fileCount_] = String(PHOTO_DIR) + "/" + name;
            fileCount_++;
        }
        entry.close();
    }
    dir.close();

    // Sort descending (newest first) by filename
    for (int i = 1; i < fileCount_; i++) {
        String key = fileList_[i];
        int j = i - 1;
        while (j >= 0 && fileList_[j] < key) {
            fileList_[j + 1] = fileList_[j];
            j--;
        }
        fileList_[j + 1] = key;
    }

    gridOffset_ = 0;
    currentPhotoIndex_ = 0;
    thumbsDecoded_ = 0;
}

void AlbumUI::scrollUp() {
    if (gridOffset_ > 0) {
        gridOffset_ -= PAGE_SIZE;
        if (gridOffset_ < 0) gridOffset_ = 0;
        thumbsDecoded_ = 0;
        dirty_ = true;
    }
}

void AlbumUI::scrollDown() {
    if (gridOffset_ + PAGE_SIZE < fileCount_) {
        gridOffset_ += PAGE_SIZE;
        thumbsDecoded_ = 0;
        dirty_ = true;
    }
}

void AlbumUI::showPhoto(int index) {
    if (index < 0 || index >= fileCount_) return;
    currentPhotoIndex_ = index;
    viewMode_ = AlbumViewMode::FULLVIEW;
    dirty_ = true;
}

void AlbumUI::nextPhoto() {
    if (currentPhotoIndex_ < fileCount_ - 1) {
        currentPhotoIndex_++;
        dirty_ = true;
    }
}

void AlbumUI::prevPhoto() {
    if (currentPhotoIndex_ > 0) {
        currentPhotoIndex_--;
        dirty_ = true;
    }
}

bool AlbumUI::deleteCurrentPhoto() {
    if (fileCount_ == 0 || currentPhotoIndex_ >= fileCount_) return false;

    bool ok = gStorageManager.deleteFile(fileList_[currentPhotoIndex_].c_str());
    if (!ok) return false;

    for (int i = currentPhotoIndex_; i < fileCount_ - 1; i++) {
        fileList_[i] = fileList_[i + 1];
    }
    fileCount_--;

    if (fileCount_ == 0) {
        viewMode_ = AlbumViewMode::GRID;
    } else if (currentPhotoIndex_ >= fileCount_) {
        currentPhotoIndex_ = fileCount_ - 1;
    }

    thumbsDecoded_ = 0;
    dirty_ = true;
    return true;
}

AlbumHitZone AlbumUI::hitTest(int x, int y) const {
    if (x >= BACK_X && x < BACK_X + BACK_W && y >= BACK_Y && y < BACK_Y + BACK_H) {
        return AlbumHitZone::BACK;
    }
    if (viewMode_ == AlbumViewMode::FULLVIEW) {
        if (x >= DEL_X && x < DEL_X + DEL_W && y >= DEL_Y && y < DEL_Y + DEL_H) {
            return AlbumHitZone::DELETE;
        }
    }
    if (viewMode_ == AlbumViewMode::GRID && thumbnailIndexAt(x, y) >= 0) {
        return AlbumHitZone::THUMBNAIL;
    }
    return AlbumHitZone::NONE;
}

int AlbumUI::thumbnailIndexAt(int x, int y) const {
    if (y < GRID_TOP_Y || fileCount_ == 0) return -1;

    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            int cellX = GRID_MARGIN + col * (CELL_W + GRID_MARGIN);
            int cellY = GRID_TOP_Y + GRID_MARGIN + row * (CELL_H + GRID_MARGIN);
            if (x >= cellX && x < cellX + CELL_W && y >= cellY && y < cellY + CELL_H) {
                int fileIdx = gridOffset_ + row * GRID_COLS + col;
                if (fileIdx < fileCount_) return fileIdx;
                return -1;
            }
        }
    }
    return -1;
}

void AlbumUI::update() {
    if (!visible_ || !spriteReady_) return;

    bool needRedraw = dirty_;
    bool progressiveLoad = false;

    if (viewMode_ == AlbumViewMode::GRID && fileCount_ > 0) {
        int visibleCount = fileCount_ - gridOffset_;
        if (visibleCount > PAGE_SIZE) visibleCount = PAGE_SIZE;
        if (thumbsDecoded_ < visibleCount) {
            progressiveLoad = true;
            needRedraw = true;
        }
    }

    if (!needRedraw) return;

    canvas_.fillSprite(UiTheme::BG);

    if (fileCount_ == 0) {
        drawEmptyState();
    } else if (viewMode_ == AlbumViewMode::GRID) {
        drawGrid();
    } else {
        drawFullView();
    }

    canvas_.pushSprite(0, 0);
    dirty_ = false;
}

void AlbumUI::drawEmptyState() {
    UiTheme::drawTitle(canvas_, "Album", "No photos", UiTheme::PINK);
    UiTheme::drawBackButton(canvas_, BACK_X, BACK_Y, BACK_W, BACK_H);

    canvas_.setTextDatum(MC_DATUM);
    canvas_.setTextSize(2);
    canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::BG);
    canvas_.drawString("Empty", DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(TL_DATUM);
}

void AlbumUI::drawGrid() {
    UiTheme::drawTitle(canvas_, "Album", nullptr, UiTheme::PINK);
    UiTheme::drawBackButton(canvas_, BACK_X, BACK_Y, BACK_W, BACK_H);

    int visibleCount = fileCount_ - gridOffset_;
    if (visibleCount > PAGE_SIZE) visibleCount = PAGE_SIZE;

    int decoded = 0;
    for (int i = 0; i < visibleCount; i++) {
        int row = i / GRID_COLS;
        int col = i % GRID_COLS;
        int cellX = GRID_MARGIN + col * (CELL_W + GRID_MARGIN);
        int cellY = GRID_TOP_Y + GRID_MARGIN + row * (CELL_H + GRID_MARGIN);

        if (decoded < thumbsDecoded_) {
            drawJpgFromSd(canvas_, fileList_[gridOffset_ + i].c_str(),
                          cellX, cellY, CELL_W, CELL_H);
            decoded++;
        } else if (thumbsDecoded_ + 2 > decoded && decoded >= thumbsDecoded_) {
            drawJpgFromSd(canvas_, fileList_[gridOffset_ + i].c_str(),
                          cellX, cellY, CELL_W, CELL_H);
            thumbsDecoded_++;
            decoded++;
        } else {
            canvas_.fillRect(cellX, cellY, CELL_W, CELL_H, UiTheme::PANEL);
            canvas_.setTextDatum(MC_DATUM);
            canvas_.setTextSize(1);
            canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::PANEL);
            canvas_.drawNumber(gridOffset_ + i + 1, cellX + CELL_W / 2, cellY + CELL_H / 2);
            canvas_.setTextDatum(TL_DATUM);
        }
    }

    // Page indicator
    int totalPages = (fileCount_ + PAGE_SIZE - 1) / PAGE_SIZE;
    int currentPage = gridOffset_ / PAGE_SIZE + 1;
    canvas_.setTextDatum(BC_DATUM);
    canvas_.setTextSize(1);
    canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::BG);
    char pageStr[16];
    snprintf(pageStr, sizeof(pageStr), "%d/%d", currentPage, totalPages);
    canvas_.drawString(pageStr, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 4);
    canvas_.setTextDatum(TL_DATUM);
}

void AlbumUI::drawFullView() {
    drawJpgFromSd(canvas_, fileList_[currentPhotoIndex_].c_str(),
                  0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // Back button overlay
    canvas_.fillRoundRect(BACK_X, BACK_Y, BACK_W, BACK_H, 6, UiTheme::PANEL);
    canvas_.drawRoundRect(BACK_X, BACK_Y, BACK_W, BACK_H, 6, UiTheme::PANEL_LIGHT);
    canvas_.setTextDatum(MC_DATUM);
    canvas_.setTextSize(1);
    canvas_.setTextColor(UiTheme::TEXT_DIM, UiTheme::PANEL);
    canvas_.drawString("< Back", BACK_X + BACK_W / 2, BACK_Y + BACK_H / 2 + 1);

    // Delete button
    canvas_.fillRoundRect(DEL_X, DEL_Y, DEL_W, DEL_H, 6, UiTheme::PANEL);
    canvas_.drawRoundRect(DEL_X, DEL_Y, DEL_W, DEL_H, 6, UiTheme::PINK);
    canvas_.setTextColor(UiTheme::PINK, UiTheme::PANEL);
    canvas_.drawString("Delete", DEL_X + DEL_W / 2, DEL_Y + DEL_H / 2 + 1);

    // Photo index
    char indexStr[16];
    snprintf(indexStr, sizeof(indexStr), "%d/%d", currentPhotoIndex_ + 1, fileCount_);
    int labelW = canvas_.textWidth(indexStr) + 12;
    int labelX = DISPLAY_WIDTH - labelW - 5;
    canvas_.fillRoundRect(labelX, BACK_Y, labelW, BACK_H, 6, UiTheme::PANEL);
    canvas_.setTextColor(UiTheme::TEXT, UiTheme::PANEL);
    canvas_.drawString(indexStr, labelX + labelW / 2, BACK_Y + BACK_H / 2 + 1);

    canvas_.setTextDatum(TL_DATUM);
}
