#pragma once

#include <Arduino.h>
#include <functional>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "ai/voice_state.h"

class XiaoZhiClient {
public:
    XiaoZhiClient();
    bool begin();
    bool end();

    bool requestActivationCode();
    bool checkActivation();

    bool isConnected() const;
    bool isActivated() const;
    bool hasActivationCode() const;
    String getActivationCode() const;
    String getActivationMessage() const;
    String getWebSocketUrl() const;
    String getToken() const;
    String getMqttBroker() const;
    int getMqttPort() const;
    String getLastError() const;
    String getFirmwareIdentity() const;

    bool startListening();
    bool stopListening();

    void setStateCallback(std::function<void(VoiceState)> cb);
    VoiceState getState() const;
    void process();

private:
    void setState(VoiceState state);
    String getMacAddress();
    String getDeviceId();
    String buildUserAgent() const;
    String buildSystemInfoJson() const;

    bool initialized_ = false;
    bool connected_ = false;
    bool activated_ = false;
    bool hasActivationCode_ = false;
    VoiceState state_ = VoiceState::IDLE;
    std::function<void(VoiceState)> callback_;

    String activationCode_;
    String activationMessage_;
    String webSocketUrl_;
    String token_;
    String mqttBroker_;
    int mqttPort_ = 0;
    int lastHttpCode_ = 0;
    String lastError_;
    String deviceId_;
    String clientId_;
    unsigned long lastActivationAttemptMs_ = 0;
    unsigned long lastActivationErrorMs_ = 0;

    static constexpr const char* OTA_URL = "https://api.tenclass.net/xiaozhi/ota/";
    static constexpr unsigned long ACTIVATION_ERROR_COOLDOWN_MS = 30000;
};
