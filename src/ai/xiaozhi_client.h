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
    String getActivationCode() const;
    String getActivationMessage() const;
    String getWebSocketUrl() const;
    String getToken() const;

    bool startListening();
    bool stopListening();

    void setStateCallback(std::function<void(VoiceState)> cb);
    VoiceState getState() const;
    void process();

private:
    void setState(VoiceState state);
    String getMacAddress();
    String getDeviceId();

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
    String deviceId_;
    String clientId_;

    static constexpr const char* OTA_URL = "https://api.xiaozhi.me/v1/ota";
};
