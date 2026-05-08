#include "xiaozhi_client.h"
#include "config/app_config.h"
#include <Arduino.h>

XiaoZhiClient::XiaoZhiClient() = default;

bool XiaoZhiClient::begin() {
    initialized_ = true;
    connected_ = false;
    return true;
}

bool XiaoZhiClient::end() {
    disconnect();
    initialized_ = false;
    return true;
}

bool XiaoZhiClient::connect() {
    if (!initialized_) return false;
    connected_ = true;
    return true;
}

bool XiaoZhiClient::disconnect() {
    connected_ = false;
    setState(VoiceState::IDLE);
    return true;
}

bool XiaoZhiClient::isConnected() const {
    return connected_;
}

bool XiaoZhiClient::startListening() {
    if (!initialized_) return false;

    if (!connected_) {
        setState(VoiceState::LISTENING);
        Serial.println("XiaoZhi: device verification required; real service not configured");
        return true;
    }

    setState(VoiceState::LISTENING);
    return true;
}

bool XiaoZhiClient::stopListening() {
    setState(VoiceState::IDLE);
    return true;
}

void XiaoZhiClient::setStateCallback(StateCallback cb) {
    callback_ = std::move(cb);
}

VoiceState XiaoZhiClient::getState() const {
    return state_;
}

void XiaoZhiClient::process() {
}

void XiaoZhiClient::setState(VoiceState state) {
    if (state_ != state) {
        state_ = state;
        if (callback_) {
            callback_(state);
        }
    }
}
