#include "camera_manager.h"
#include "config/app_config.h"
#include <M5CoreS3.h>
#include <esp_camera.h>

CameraManager::CameraManager() = default;

bool CameraManager::begin() {
    if (initialized_) return true;

    if (ESP.getPsramSize() == 0) {
        initialized_ = false;
        return false;
    }

    if (!CoreS3.Camera.begin()) {
        initialized_ = false;
        return false;
    }
    initialized_ = true;
    capturing_ = false;
    return true;
}

bool CameraManager::end() {
    stopCapture();
    if (initialized_) {
        esp_camera_deinit();
        initialized_ = false;
    }
    return true;
}

bool CameraManager::startCapture() {
    if (!initialized_) return false;
    capturing_ = true;
    return true;
}

bool CameraManager::stopCapture() {
    capturing_ = false;
    return true;
}

CameraFrame CameraManager::getDisplayFrame() {
    CameraFrame frame;
    if (!initialized_ || !capturing_) return frame;

    if (CoreS3.Camera.get()) {
        frame.data = CoreS3.Camera.fb->buf;
        frame.width = CoreS3.Camera.fb->width;
        frame.height = CoreS3.Camera.fb->height;
        frame.size = CoreS3.Camera.fb->len;
        frame.valid = true;
        CoreS3.Camera.free();
    }
    return frame;
}

CameraFrame CameraManager::getDetectionFrame() {
    // Returns the same frame — detection resolution is QVGA already
    return getDisplayFrame();
}

bool CameraManager::isRunning() const {
    return initialized_ && capturing_;
}

bool CameraManager::isInitialized() const {
    return initialized_;
}

void CameraManager::scaleDown(const uint8_t* src, uint8_t* dst,
                               int srcW, int srcH, int dstW, int dstH) {
    float scaleX = (float)srcW / dstW;
    float scaleY = (float)srcH / dstH;

    for (int y = 0; y < dstH; ++y) {
        for (int x = 0; x < dstW; ++x) {
            int srcX = (int)(x * scaleX);
            int srcY = (int)(y * scaleY);
            int srcIdx = (srcY * srcW + srcX) * 2;
            int dstIdx = (y * dstW + x) * 2;
            dst[dstIdx] = src[srcIdx];
            dst[dstIdx + 1] = src[srcIdx + 1];
        }
    }
}
