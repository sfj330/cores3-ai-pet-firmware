#include "storage_manager.h"
#include "config/app_config.h"
#include <SD.h>

void StorageManager::resetCardBus() {
    SD.end();
    delay(10);

    if (sdSPI_ == nullptr) {
        sdSPI_ = new SPIClass(FSPI);
    }

    sdSPI_->end();
    delay(10);
    sdSPI_->begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    spiBegun_ = true;
    delay(50);
}

const char* StorageManager::cardTypeName(uint8_t type) const {
    switch (type) {
        case CARD_MMC: return "MMC";
        case CARD_SD: return "SDSC";
        case CARD_SDHC: return "SDHC";
        default: return "UNKNOWN";
    }
}

bool StorageManager::tryInitAtFreq(uint32_t freq) {
    resetCardBus();
    diagnostic_ = StorageDiagnostic{};
    diagnostic_.freqHz = freq;

    Serial.printf("SD try %lu MHz (FSPI)...\n", static_cast<unsigned long>(freq / 1000000));

    if (!SD.begin(SD_SPI_CS_PIN, *sdSPI_, freq)) {
        diagnostic_.detail = String("SD.begin failed @") + String(freq / 1000000) + "MHz";
        status_ = diagnostic_.detail;
        return false;
    }

    diagnostic_.beginOk = true;
    return validateMountedCard(freq);
}

bool StorageManager::validateMountedCard(uint32_t freq) {
    uint8_t type = SD.cardType();
    diagnostic_.cardType = type;
    diagnostic_.cardPresent = type != CARD_NONE;

    if (!diagnostic_.cardPresent) {
        diagnostic_.detail = String("CARD_NONE @") + String(freq / 1000000) + "MHz";
        status_ = diagnostic_.detail;
        SD.end();
        return false;
    }

    diagnostic_.cardSizeMb = SD.cardSize() / (1024 * 1024);
    if (diagnostic_.cardSizeMb == 0) {
        diagnostic_.detail = "Card size is 0MB";
        status_ = diagnostic_.detail;
        SD.end();
        return false;
    }

    File root = SD.open("/");
    diagnostic_.rootOpen = root && root.isDirectory();
    if (root) root.close();
    if (!diagnostic_.rootOpen) {
        diagnostic_.detail = "Root open failed";
        status_ = diagnostic_.detail;
        SD.end();
        return false;
    }

    diagnostic_.photoDirOk = SD.exists(PHOTO_DIR) || SD.mkdir(PHOTO_DIR);
    if (!diagnostic_.photoDirOk) {
        diagnostic_.detail = "Create /photos failed";
        status_ = diagnostic_.detail;
        SD.end();
        return false;
    }

    String probePath = String(PHOTO_DIR) + "/.probe";
    File probe = SD.open(probePath.c_str(), FILE_WRITE);
    if (probe) {
        diagnostic_.probeWriteOk = probe.write(static_cast<uint8_t>(0xA5)) == 1;
        probe.close();
        SD.remove(probePath.c_str());
    }
    if (!diagnostic_.probeWriteOk) {
        diagnostic_.detail = "Probe write failed";
        status_ = diagnostic_.detail;
        SD.end();
        return false;
    }

    diagnostic_.detail = String(cardTypeName(type)) + " " + String(diagnostic_.cardSizeMb) +
                         "MB @" + String(freq / 1000000) + "MHz";
    status_ = String("SD ") + diagnostic_.detail;
    return true;
}

bool StorageManager::attemptInit() {
    ready_ = false;
    status_ = "SD probing";

    for (uint32_t freq : freqLevels_) {
        if (tryInitAtFreq(freq)) {
            ready_ = true;
            Serial.printf("SD ready: %s\n", diagnostic_.detail.c_str());
            return true;
        }
        Serial.printf("SD failed: %s\n", diagnostic_.detail.c_str());
    }

    ready_ = false;
    if (status_.length() == 0 || status_ == "SD probing") {
        status_ = "SD not found";
    }
    return false;
}

bool StorageManager::begin() {
    return attemptInit();
}

bool StorageManager::refresh() {
    if (ready_) return true;
    return attemptInit();
}

bool StorageManager::ensureReady() {
    if (ready_) return true;
    return refresh();
}

bool StorageManager::forceReprobe() {
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
    status_ = "Photo index full";
    return String();
}

bool StorageManager::writeFile(const char* path, const uint8_t* data, size_t len) {
    if (!ready_ || path == nullptr || data == nullptr || len == 0) {
        status_ = "Write input invalid";
        return false;
    }

    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        status_ = String("Open failed: ") + path;
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

const StorageDiagnostic& StorageManager::diagnostic() const {
    return diagnostic_;
}
