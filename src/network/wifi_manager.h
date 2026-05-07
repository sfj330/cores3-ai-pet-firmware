#pragma once

#include <Arduino.h>

class WifiManager {
public:
    bool begin();
    void update();

    bool isConnected() const;
    bool isConfigured() const;
    String ipString() const;
    String statusText() const;
    int rssi() const;

private:
    void startConnect();

    bool configured_ = false;
    unsigned long lastConnectAttempt_ = 0;
};
