#include "camera_debug_ui.h"
#include "config/app_config.h"
#include <M5CoreS3.h>

CameraDebugUI::CameraDebugUI() : canvas_(&M5.Lcd) {}

void CameraDebugUI::begin() {
    canvas_.setPsram(true);
    canvas_.setColorDepth(16);
    spriteReady_ = canvas_.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT) != nullptr;
    if (spriteReady_) {
        canvas_.fillSprite(TFT_BLACK);
    }
}

void CameraDebugUI::show() {
    visible_ = true;
    if (spriteReady_) {
        canvas_.fillSprite(TFT_BLACK);
    }
}

void CameraDebugUI::hide() {
    visible_ = false;
}

void CameraDebugUI::setFps(float fps) {
    currentFps_ = fps;
}

void CameraDebugUI::setCameraReady(bool ready) {
    cameraReady_ = ready;
}

void CameraDebugUI::setLastPhotoPath(const char* path) {
    lastPhotoPath_ = path ? String(path) : String();
}

void CameraDebugUI::setSdReady(bool ready) {
    sdReady_ = ready;
}

void CameraDebugUI::setCaptureStatus(const char* status) {
    captureStatus_ = status ? String(status) : String();
}

CameraHitZone CameraDebugUI::hitTest(int x, int y) const {
    if (x >= BACK_X && x < BACK_X + BACK_W && y >= BACK_Y && y < BACK_Y + BACK_H) {
        return CameraHitZone::HIT_BACK;
    }
    if (x >= SHOT_X && x < SHOT_X + SHOT_W && y >= SHOT_Y && y < SHOT_Y + SHOT_H) {
        return CameraHitZone::HIT_SHOT;
    }
    return CameraHitZone::HIT_NONE;
}

void CameraDebugUI::pushCameraFrame(const uint16_t* data, int w, int h) {
    if (!spriteReady_ || !visible_) return;
    canvas_.pushImage(0, 0, w, h, data);
}

void CameraDebugUI::update() {
    if (!visible_ || !spriteReady_) return;
    drawOverlay();
    drawBackButton();
    drawShotButton();
    canvas_.pushSprite(0, 0);
}

void CameraDebugUI::drawOverlay() {
    canvas_.setTextSize(1);
    canvas_.setTextDatum(TL_DATUM);

    canvas_.setTextColor(TFT_GREEN, TFT_BLACK);
    canvas_.setCursor(5, 5);
    canvas_.printf("FPS: %.1f", currentFps_);

    canvas_.setCursor(5, 20);
    canvas_.printf("%dx%d", CAMERA_FRAME_WIDTH, CAMERA_FRAME_HEIGHT);

    if (!cameraReady_) {
        canvas_.setTextColor(TFT_RED, TFT_BLACK);
        canvas_.setCursor(5, 35);
        canvas_.print("Camera not ready");
    }

    if (!sdReady_) {
        canvas_.setTextColor(TFT_YELLOW, TFT_BLACK);
        canvas_.setCursor(5, 50);
        canvas_.print("No SD card");
    }

    if (lastPhotoPath_.length() > 0) {
        canvas_.setTextColor(TFT_CYAN, TFT_BLACK);
        canvas_.setCursor(5, 65);
        canvas_.print(lastPhotoPath_);
    }

    if (captureStatus_.length() > 0) {
        canvas_.setTextColor(TFT_WHITE, TFT_BLACK);
        canvas_.setCursor(5, 80);
        canvas_.print(captureStatus_);
    }
}

void CameraDebugUI::drawBackButton() {
    canvas_.fillRoundRect(BACK_X, BACK_Y, BACK_W, BACK_H, 4, TFT_DARKGREY);
    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(1);
    canvas_.setTextDatum(MC_DATUM);
    canvas_.setCursor(BACK_X + BACK_W / 2 - 12, BACK_Y + BACK_H / 2 - 4);
    canvas_.print("Back");
    canvas_.setTextDatum(TL_DATUM);
}

void CameraDebugUI::drawShotButton() {
    uint16_t btnColor = sdReady_ ? TFT_GREEN : TFT_DARKGREY;
    canvas_.fillRoundRect(SHOT_X, SHOT_Y, SHOT_W, SHOT_H, 6, btnColor);
    canvas_.setTextColor(TFT_WHITE);
    canvas_.setTextSize(2);
    canvas_.setTextDatum(MC_DATUM);
    int textX = SHOT_X + SHOT_W / 2;
    int textStartY = SHOT_Y + SHOT_H / 2 - 24;
    const char letters[] = "SHOT";
    for (int i = 0; i < 4; ++i) {
        canvas_.setCursor(textX - 6, textStartY + i * 16);
        canvas_.print(letters[i]);
    }
    canvas_.setTextDatum(TL_DATUM);
}
