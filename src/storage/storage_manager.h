#pragma once

#include <Arduino.h>
#include <SPI.h>

struct StorageDiagnostic {
    bool beginOk = false;
    bool cardPresent = false;
    bool rootOpen = false;
    bool photoDirOk = false;
    bool probeWriteOk = false;
    uint8_t cardType = 0;
    uint32_t freqHz = 0;
    uint64_t cardSizeMb = 0;
    String detail = "Not initialized";
};

class StorageManager {
public:
    bool begin();
    bool isReady() const;

    bool refresh();
    bool ensureReady();
    bool forceReprobe();

    String nextPhotoPath();
    bool writeFile(const char* path, const uint8_t* data, size_t len);
    String statusText() const;
    const StorageDiagnostic& diagnostic() const;

private:
    bool tryInitAtFreq(uint32_t freq);
    bool attemptInit();
    bool validateMountedCard(uint32_t freq);
    void resetCardBus();
    const char* cardTypeName(uint8_t type) const;

    bool ready_ = false;
    String status_ = "Not initialized";
    uint32_t freqLevels_[4] = {25000000, 10000000, 4000000, 1000000};
    bool spiBegun_ = false;
    StorageDiagnostic diagnostic_;
    SPIClass* sdSPI_ = nullptr;
};
