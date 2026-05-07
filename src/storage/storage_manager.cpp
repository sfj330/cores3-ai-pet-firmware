#include "storage_manager.h"
#include "config/app_config.h"
#include <SPI.h>
#include <SD.h>

bool StorageManager::begin() {
    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQ)) {
        ready_ = false;
        status_ = "SD not ready";
        Serial.println("SD init failed");
        return false;
    }

    if (SD.cardType() == CARD_NONE) {
        ready_ = false;
        status_ = "No SD card";
        Serial.println("No SD card attached");
        return false;
    }

    if (!SD.exists(PHOTO_DIR)) {
        SD.mkdir(PHOTO_DIR);
    }

    ready_ = true;
    status_ = "SD ready";
    Serial.printf("SD ready, size=%lluMB\n", SD.cardSize() / (1024 * 1024));
    return true;
}

bool StorageManager::isReady() const {
    return ready_;
}

String StorageManager::nextPhotoPath() {
    if (!ready_) return String();

    char path[40];
    for (int i = 1; i <= 9999; ++i) {
        snprintf(path, sizeof(path), "%s/IMG_%04d.jpg", PHOTO_DIR, i);
        if (!SD.exists(path)) {
            return String(path);
        }
    }
    return String();
}

bool StorageManager::writeFile(const char* path, const uint8_t* data, size_t len) {
    if (!ready_ || path == nullptr || data == nullptr || len == 0) {
        status_ = "Write input invalid";
        return false;
    }

    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        status_ = "Open failed";
        return false;
    }

    size_t written = file.write(data, len);
    file.close();

    if (written != len) {
        status_ = "Write failed";
        return false;
    }

    status_ = String("Saved ") + path;
    return true;
}

String StorageManager::statusText() const {
    return status_;
}
