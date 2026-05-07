#include "wifi_manager.h"
#include "config/app_config.h"
#include <WiFi.h>

#if __has_include("config/wifi_secrets.h")
#include "config/wifi_secrets.h"
#else
constexpr const char* WIFI_SSID = "";
constexpr const char* WIFI_PASSWORD = "";
#endif

bool WifiManager::begin() {
    configured_ = WIFI_SSID[0] != '\0';
    if (!configured_) {
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    startConnect();
    return true;
}

void WifiManager::update() {
    if (!configured_ || isConnected()) {
        return;
    }

    unsigned long now = millis();
    if (now - lastConnectAttempt_ >= WIFI_RECONNECT_INTERVAL_MS) {
        startConnect();
    }
}

bool WifiManager::isConnected() const {
    return configured_ && WiFi.status() == WL_CONNECTED;
}

bool WifiManager::isConfigured() const {
    return configured_;
}

String WifiManager::ipString() const {
    return isConnected() ? WiFi.localIP().toString() : String("--.--.--.--");
}

String WifiManager::statusText() const {
    if (!configured_) return "Not configured";
    if (isConnected()) return "Connected";

    switch (WiFi.status()) {
        case WL_IDLE_STATUS: return "Idle";
        case WL_NO_SSID_AVAIL: return "SSID missing";
        case WL_CONNECT_FAILED: return "Connect failed";
        case WL_CONNECTION_LOST: return "Lost";
        case WL_DISCONNECTED: return "Disconnected";
        default: return "Connecting";
    }
}

int WifiManager::rssi() const {
    return isConnected() ? WiFi.RSSI() : 0;
}

void WifiManager::startConnect() {
    lastConnectAttempt_ = millis();
    WiFi.disconnect(false, false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("WiFi connecting to %s\n", WIFI_SSID);
}
