#pragma once

#include <cstdint>
#include <functional>
#include "voice_state.h"

class XiaoZhiClient {
public:
    using StateCallback = std::function<void(VoiceState)>;

    XiaoZhiClient();
    bool begin();
    bool end();

    bool connect();
    bool disconnect();
    bool isConnected() const;

    // Start/stop listening (mic capture + upload)
    bool startListening();
    bool stopListening();

    void setStateCallback(StateCallback cb);
    VoiceState getState() const;

    // Process any received data (call from AI task)
    void process();

private:
    void setState(VoiceState state);

    bool initialized_ = false;
    bool connected_ = false;
    VoiceState state_ = VoiceState::IDLE;
    StateCallback callback_ = nullptr;
};
