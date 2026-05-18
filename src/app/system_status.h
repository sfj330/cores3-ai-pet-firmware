#pragma once

#include <Arduino.h>
#include <cstdint>

enum class SystemBrightnessLevel : uint8_t {
    DIM,
    NORMAL,
    BRIGHT
};

enum class SystemVolumeLevel : uint8_t {
    QUIET,
    NORMAL,
    LOUD
};

enum class SystemMusicState : uint8_t {
    STOPPED,
    PLAYING,
    PAUSED,
    ERROR
};

enum class SystemVoiceState : uint8_t {
    IDLE,
    LISTENING,
    THINKING,
    SPEAKING,
    ERROR
};

struct SystemStatusViewModel {
    struct WifiStatus {
        bool connected = false;
        bool configured = false;
        int rssi = 0;
        String ip = "--.--.--.--";
    } wifi;

    struct SdStatus {
        bool ready = false;
        String statusText = "Not initialized";
    } sd;

    struct MusicStatus {
        SystemMusicState playbackState = SystemMusicState::STOPPED;
        String currentTitle;
        String statusText = "No music";
    } music;

    struct AiStatus {
        bool activated = false;
        bool wsConnected = false;
        bool audioChannelOpen = false;
        SystemVoiceState voiceState = SystemVoiceState::IDLE;
        bool visionReady = false;
    } ai;

    struct MemoryStatus {
        uint32_t heapKb = 0;
        uint32_t psramKb = 0;
    } memory;

    struct ControlStatus {
        SystemBrightnessLevel brightnessLevel = SystemBrightnessLevel::BRIGHT;
        SystemVolumeLevel volumeLevel = SystemVolumeLevel::NORMAL;
    } control;
};

bool systemStatusEquals(const SystemStatusViewModel& a, const SystemStatusViewModel& b);

String systemBrightnessLabel(SystemBrightnessLevel level);
String systemVolumeLabel(SystemVolumeLevel level);

String systemStatusXiaoZhiSummary(const SystemStatusViewModel& status);
String systemStatusVisionSummary(const SystemStatusViewModel& status);
String systemStatusWifiLine(const SystemStatusViewModel& status);
String systemStatusSdLine(const SystemStatusViewModel& status);
String systemStatusAudioLine(const SystemStatusViewModel& status);
String systemStatusControlLine(const SystemStatusViewModel& status);
String systemStatusMemoryLine(const SystemStatusViewModel& status);

String systemStatusBriefSentence1(const SystemStatusViewModel& status);
String systemStatusBriefSentence2(const SystemStatusViewModel& status);
String systemStatusMemorySentence(const SystemStatusViewModel& status);
