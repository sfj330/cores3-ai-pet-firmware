#include "camera_debug_ui.h"
#include "config/app_config.h"
#include "ui/ui_theme.h"
#include <M5CoreS3.h>

CameraDebugUI::CameraDebugUI() : canvas_(&M5.Lcd) {}

void CameraDebugUI::begin() {
    canvas_.setPsram(true);
    canvas_.setColorDepth(16);
    spriteReady_ = canvas_.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT) != nullptr;
    if (spriteReady_) {
        canvas_.fillSprite(UiTheme::BG);
    }
}

void CameraDebugUI::show(CameraViewMode mode) {
    visible_ = true;
    mode_ = mode;
    if (spriteReady_) {
        canvas_.fillSprite(UiTheme::BG);
    }
}

void CameraDebugUI::hide() {
    visible_ = false;
}

void CameraDebugUI::setMode(CameraViewMode mode) {
    mode_ = mode;
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

void CameraDebugUI::setVisionStatus(const char* status) {
    visionStatus_ = status ? String(status) : String();
}

void CameraDebugUI::setVisionResult(const char* result) {
    visionResult_ = result ? String(result) : String();
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
    if (mode_ == CameraViewMode::AI_VISION) {
        drawAiVisionOverlay();
    } else {
        drawOverlay();
        drawShotButton();
    }
    drawBackButton();
    canvas_.pushSprite(0, 0);
}

void CameraDebugUI::drawOverlay() {
}

void CameraDebugUI::drawBackButton() {
    UiTheme::drawBackButton(canvas_, BACK_X, BACK_Y, BACK_W, BACK_H);
}

void CameraDebugUI::drawShotButton() {
    uint16_t btnColor = sdReady_ ? UiTheme::GREEN : UiTheme::PANEL_LIGHT;
    canvas_.fillRoundRect(SHOT_X, SHOT_Y, SHOT_W, SHOT_H, 7, btnColor);
    canvas_.drawRoundRect(SHOT_X, SHOT_Y, SHOT_W, SHOT_H, 7, UiTheme::TEXT);
    canvas_.setTextColor(sdReady_ ? UiTheme::BG : UiTheme::TEXT_DIM, btnColor);
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

void CameraDebugUI::drawAiVisionOverlay() {
    canvas_.fillRoundRect(4, 4, DISPLAY_WIDTH - 68, 48, 7, UiTheme::PANEL);
    canvas_.drawRoundRect(4, 4, DISPLAY_WIDTH - 68, 48, 7, UiTheme::CYAN_DIM);
    canvas_.setTextDatum(TL_DATUM);
    canvas_.setTextSize(1);
    canvas_.setTextColor(UiTheme::CYAN, UiTheme::PANEL);
    canvas_.setCursor(12, 12);
    canvas_.print("AI Vision / 小智视觉");
    canvas_.setTextColor(cameraReady_ ? UiTheme::GREEN : UiTheme::AMBER, UiTheme::PANEL);
    canvas_.setCursor(12, 28);
    canvas_.print(cameraReady_ ? "Camera on" : "Opening camera");

    if (visionStatus_.length() > 0) {
        canvas_.fillRoundRect(8, DISPLAY_HEIGHT - 54, DISPLAY_WIDTH - 16, 20, 7, UiTheme::PANEL);
        canvas_.setTextColor(UiTheme::AMBER, UiTheme::PANEL);
        canvas_.setCursor(16, DISPLAY_HEIGHT - 49);
        canvas_.print(visionStatus_);
    }
    if (visionResult_.length() > 0) {
        canvas_.fillRoundRect(8, DISPLAY_HEIGHT - 31, DISPLAY_WIDTH - 16, 23, 7, UiTheme::PANEL);
        canvas_.drawRoundRect(8, DISPLAY_HEIGHT - 31, DISPLAY_WIDTH - 16, 23, 7, UiTheme::CYAN_DIM);
        canvas_.setTextColor(UiTheme::TEXT, UiTheme::PANEL);
        canvas_.setCursor(16, DISPLAY_HEIGHT - 24);
        String shown = visionResult_;
        if (shown.length() > 38) {
            shown = shown.substring(0, 38);
        }
        canvas_.print(shown);
    }
}
