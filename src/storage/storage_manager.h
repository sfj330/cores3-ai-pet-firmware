#pragma once

#include <Arduino.h>

class StorageManager {
public:
    bool begin();
    bool isReady() const;

    String nextPhotoPath();
    bool writeFile(const char* path, const uint8_t* data, size_t len);
    String statusText() const;

private:
    bool ready_ = false;
    String status_ = "Not initialized";
};
