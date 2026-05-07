#pragma once

#include <cstdint>
#include <cstddef>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct CameraFrame {
    uint8_t* data = nullptr;
    int width = 0;
    int height = 0;
    size_t size = 0;
    bool valid = false;
    bool locked = false;
};

class CameraManager {
public:
    CameraManager();
    bool begin();
    bool end();

    bool startCapture();
    bool stopCapture();
    bool isInitialized() const;

    // Get a frame for display (full resolution)
    CameraFrame getDisplayFrame();

    // Get a frame for processing (lower resolution)
    CameraFrame getDetectionFrame();
    void releaseFrame(CameraFrame& frame);

    bool captureJpegToFile(const char* path, String& status);

    bool isRunning() const;

    // Scale down an RGB565 frame for detection
    static void scaleDown(const uint8_t* src, uint8_t* dst,
                          int srcW, int srcH, int dstW, int dstH);

private:
    bool ensureMutex();

    bool initialized_ = false;
    bool capturing_ = false;
    SemaphoreHandle_t cameraMutex_ = nullptr;
};
