#include "camera_manager.h"
#include "config/app_config.h"
#include <M5CoreS3.h>
#include <esp_camera.h>
#include <img_converters.h>
#include <SD.h>

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
    if (!ensureMutex()) return frame;
    if (xSemaphoreTake(cameraMutex_, pdMS_TO_TICKS(200)) != pdTRUE) return frame;

    if (CoreS3.Camera.get()) {
        frame.data = CoreS3.Camera.fb->buf;
        frame.width = CoreS3.Camera.fb->width;
        frame.height = CoreS3.Camera.fb->height;
        frame.size = CoreS3.Camera.fb->len;
        frame.valid = true;
        frame.locked = true;
        return frame;
    }
    xSemaphoreGive(cameraMutex_);
    return frame;
}

CameraFrame CameraManager::getDetectionFrame() {
    // Returns the same frame — detection resolution is QVGA already
    return getDisplayFrame();
}

void CameraManager::releaseFrame(CameraFrame& frame) {
    if (!frame.locked) {
        frame = CameraFrame{};
        return;
    }

    CoreS3.Camera.free();
    CoreS3.Camera.fb = nullptr;
    frame = CameraFrame{};
    if (cameraMutex_ != nullptr) {
        xSemaphoreGive(cameraMutex_);
    }
}

bool CameraManager::isRunning() const {
    return initialized_ && capturing_;
}

bool CameraManager::isInitialized() const {
    return initialized_;
}

bool CameraManager::ensureMutex() {
    if (cameraMutex_ == nullptr) {
        cameraMutex_ = xSemaphoreCreateMutex();
    }
    return cameraMutex_ != nullptr;
}

bool CameraManager::captureJpegToFile(const char* path, String& status) {
    if (!initialized_) {
        status = "Camera not initialized";
        return false;
    }

    if (!ensureMutex()) {
        status = "Mutex create failed";
        return false;
    }

    if (xSemaphoreTake(cameraMutex_, pdMS_TO_TICKS(2000)) != pdTRUE) {
        status = "Camera busy";
        return false;
    }

    bool ok = false;

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        status = "Capture failed";
        xSemaphoreGive(cameraMutex_);
        return false;
    }

    if (fb->format == PIXFORMAT_JPEG) {
        File file = SD.open(path, FILE_WRITE);
        if (file) {
            size_t written = file.write(fb->buf, fb->len);
            file.close();
            if (written == fb->len) {
                status = String("Saved ") + path;
                ok = true;
            } else {
                status = "Write failed";
            }
        } else {
            status = "SD open failed";
        }
    } else {
        uint8_t* jpgBuf = nullptr;
        size_t jpgLen = 0;
        bool jpegOk = fmt2jpg(fb->buf, fb->len, fb->width, fb->height,
                               fb->format, CAMERA_JPEG_QUALITY, &jpgBuf, &jpgLen);
        if (!jpegOk || jpgBuf == nullptr) {
            status = "JPEG convert failed";
        } else {
            File file = SD.open(path, FILE_WRITE);
            if (file) {
                size_t written = file.write(jpgBuf, jpgLen);
                file.close();
                if (written == jpgLen) {
                    status = String("Saved ") + path;
                    ok = true;
                } else {
                    status = "Write failed";
                }
            } else {
                status = "SD open failed";
            }
            free(jpgBuf);
        }
    }

    esp_camera_fb_return(fb);
    xSemaphoreGive(cameraMutex_);
    return ok;
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
