#include "storage_manager.h"
#include "config/app_config.h"
#include <SPI.h>
#include <SD.h>

bool StorageManager::tryInitAtFreq(uint32_t freq) {
    if (!spiBegun_) {
        SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
        spiBegun_ = true;
        delay(50);
    }

    Serial.printf("SD try %lu MHz...\n", static_cast<unsigned long>(freq / 1000000));

    if (!SD.begin(SD_SPI_CS_PIN, SPI, freq)) {
        Serial.printf("SD failed @%luMHz\n", static_cast<unsigned long>(freq / 1000000));
        return false;
    }

    if (SD.cardType() == CARD_NONE) {
        SD.end();
        Serial.println("SD: CARD_NONE");
        return false;
    }

    if (!SD.exists(PHOTO_DIR)) {
        SD.mkdir(PHOTO_DIR);
    }

    Serial.printf("SD OK @%luMHz, type=%d, size=%lluMB\n",
                  static_cast<unsigned long>(freq / 1000000),
                  SD.cardType(),
                  SD.cardSize() / (1024 * 1024));
    return true;
}

bool StorageManager::attemptInit() {
    uint32_t freqs[] = {25000000, 10000000, 4000000, 1000000};
    for (uint32_t freq : freqs) {
        if (tryInitAtFreq(freq)) {
            ready_ = true;
            status_ = String("SD @") + String(freq / 1000000) + "MHz";
            return true;
        }
        SD.end();
        delay(50);
    }
    ready_ = false;
    status_ = "SD not found";
    return false;
}

bool StorageManager::begin() {
    return attemptInit();
}

bool StorageManager::refresh() {
    if (ready_) return true;
    SD.end();
    spiBegun_ = false;
    delay(100);
    return attemptInit();
}

bool StorageManager::ensureReady() {
    if (ready_) return true;
    return refresh();
}

bool StorageManager::forceReprobe() {
    SD.end();
    spiBegun_ = false;
    ready_ = false;
    delay(100);
    return attemptInit();
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
    if (!ready_ || !path || !data || len == 0) return false;
    File file = SD.open(path, FILE_WRITE);
    if (!file) return false;
    size_t written = file.write(data, len);
    file.close();
    return written == len;
}

String StorageManager::statusText() const {
    return status_;
}
