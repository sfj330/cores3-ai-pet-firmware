#pragma once

#include <Arduino.h>

class StorageManager {
public:
    bool begin();
    bool isReady() const;

    bool refresh();
    bool ensureReady();
    bool forceReprobe();

    String nextPhotoPath();
    bool writeFile(const char* path, const uint8_t* data, size_t len);
    bool deleteFile(const char* path);
    String statusText() const;

private:
    bool tryInitAtFreq(uint32_t freq);
    bool attemptInit();

    bool ready_ = false;
    String status_ = "Not initialized";
    bool spiBegun_ = false;
};
