#include "xiaozhi_client.h"
#include "config/app_config.h"
#include <Arduino.h>

XiaoZhiClient::XiaoZhiClient() = default;

bool XiaoZhiClient::begin() {
    initialized_ = true;
    return true;
}

bool XiaoZhiClient::end() {
    disconnect();
    initialized_ = false;
    return true;
}

bool XiaoZhiClient::connect() {
    if (!initialized_) return false;

    // -------------------------------------------------
    // PLACEHOLDER: WebSocket connection to XiaoZhi server
    //
    // Actual implementation will:
    //   1. Connect to WebSocket at AI_SERVER_URL
    //   2. Perform handshake with device ID
    //   3. Start audio channel
    // -------------------------------------------------

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
    if (!connected_) return false;

    // -------------------------------------------------
    // PLACEHOLDER: Start mic capture & upload
    //
    // Actual implementation will:
    //   1. Start I2S mic capture (16kHz, 16-bit)
    //   2. Stream audio to WebSocket
    //   3. Handle VAD (voice activity detection)
    // -------------------------------------------------

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
    // Placeholder: check for received data, update state
}

void XiaoZhiClient::setState(VoiceState state) {
    if (state_ != state) {
        state_ = state;
        if (callback_) {
            callback_(state);
        }
    }
}
