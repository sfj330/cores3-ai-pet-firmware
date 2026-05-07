#include "camera_debug_ui.h"
#include "config/app_config.h"
#include <M5CoreS3.h>

CameraDebugUI::CameraDebugUI() = default;

void CameraDebugUI::begin() {
    // No special init needed
}

void CameraDebugUI::show() {
    visible_ = true;
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

void CameraDebugUI::setDetectionOverlay(bool enabled) {
    showDetection_ = enabled;
}

bool CameraDebugUI::getDetectionOverlay() const {
    return showDetection_;
}

void CameraDebugUI::update() {
    if (!visible_) return;

    // Draw debug overlay on top of camera image
    drawDebugOverlay();
    drawBackButton();
}

void CameraDebugUI::drawDebugOverlay() {
    // FPS counter in top-left
    M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(5, 5);
    M5.Lcd.printf("FPS: %.1f", currentFps_);

    // Detection toggle status
    M5.Lcd.setCursor(5, 20);
    M5.Lcd.setTextColor(showDetection_ ? TFT_GREEN : TFT_RED, TFT_BLACK);
    M5.Lcd.print("Detection: ");
    M5.Lcd.print(showDetection_ ? "ON" : "OFF");

    // Resolution info
    M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Lcd.setCursor(5, 35);
    M5.Lcd.printf("%dx%d", CAMERA_FRAME_WIDTH, CAMERA_FRAME_HEIGHT);

    if (!cameraReady_) {
        M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
        M5.Lcd.setCursor(5, 50);
        M5.Lcd.print("Camera not ready");
    }

    // Hint for detection toggle
    M5.Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5.Lcd.setCursor(5, DISPLAY_HEIGHT - 40);
    M5.Lcd.print("Tap to toggle detection");
}

void CameraDebugUI::drawBackButton() {
    M5.Lcd.fillTriangle(DISPLAY_WIDTH - 5, 10, DISPLAY_WIDTH - 15, 5,
                        DISPLAY_WIDTH - 15, 15, TFT_WHITE);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(DISPLAY_WIDTH - 40, 8);
    M5.Lcd.print("Back");
}
