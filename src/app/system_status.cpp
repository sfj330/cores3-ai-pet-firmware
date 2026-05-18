#include "app/system_status.h"

namespace {
String trimStatusText(const String& text, size_t maxLen) {
    if (text.length() <= maxLen) {
        return text;
    }
    if (maxLen < 4) {
        return text.substring(0, maxLen);
    }
    return text.substring(0, maxLen - 3) + "...";
}

String musicSentencePart(const SystemStatusViewModel& status) {
    switch (status.music.playbackState) {
        case SystemMusicState::PLAYING:
            if (status.music.currentTitle.length() > 0) {
                return String("音乐正在播放《") + trimStatusText(status.music.currentTitle, 12) + "》";
            }
            return "音乐正在播放";
        case SystemMusicState::PAUSED:
            return "音乐已经暂停";
        case SystemMusicState::ERROR:
            return "音乐暂时不可用";
        case SystemMusicState::STOPPED:
        default:
            return "音乐现在空闲";
    }
}

String aiSentencePart(const SystemStatusViewModel& status) {
    String summary = systemStatusXiaoZhiSummary(status);
    if (summary == "Listening") return "小智正在聆听";
    if (summary == "Speaking") return "小智正在说话";
    if (summary == "Connected") return "小智已经连上了";
    if (summary == "Error") return "小智当前有点忙乱";
    return "小智目前离线";
}
}

bool systemStatusEquals(const SystemStatusViewModel& a, const SystemStatusViewModel& b) {
    return a.wifi.connected == b.wifi.connected &&
           a.wifi.configured == b.wifi.configured &&
           a.wifi.rssi == b.wifi.rssi &&
           a.wifi.ip == b.wifi.ip &&
           a.sd.ready == b.sd.ready &&
           a.sd.statusText == b.sd.statusText &&
           a.music.playbackState == b.music.playbackState &&
           a.music.currentTitle == b.music.currentTitle &&
           a.music.statusText == b.music.statusText &&
           a.ai.activated == b.ai.activated &&
           a.ai.wsConnected == b.ai.wsConnected &&
           a.ai.audioChannelOpen == b.ai.audioChannelOpen &&
           a.ai.voiceState == b.ai.voiceState &&
           a.ai.visionReady == b.ai.visionReady &&
           a.memory.heapKb == b.memory.heapKb &&
           a.memory.psramKb == b.memory.psramKb &&
           a.control.brightnessLevel == b.control.brightnessLevel &&
           a.control.volumeLevel == b.control.volumeLevel;
}

String systemBrightnessLabel(SystemBrightnessLevel level) {
    switch (level) {
        case SystemBrightnessLevel::DIM: return "Dim";
        case SystemBrightnessLevel::NORMAL: return "Normal";
        case SystemBrightnessLevel::BRIGHT:
        default:
            return "Bright";
    }
}

String systemVolumeLabel(SystemVolumeLevel level) {
    switch (level) {
        case SystemVolumeLevel::QUIET: return "Quiet";
        case SystemVolumeLevel::NORMAL: return "Normal";
        case SystemVolumeLevel::LOUD:
        default:
            return "Loud";
    }
}

String systemStatusXiaoZhiSummary(const SystemStatusViewModel& status) {
    if (status.ai.voiceState == SystemVoiceState::ERROR) {
        return "Error";
    }
    if (status.ai.audioChannelOpen && status.ai.voiceState == SystemVoiceState::SPEAKING) {
        return "Speaking";
    }
    if (status.ai.audioChannelOpen && status.ai.voiceState == SystemVoiceState::LISTENING) {
        return "Listening";
    }
    if (status.ai.wsConnected || status.ai.audioChannelOpen || status.ai.activated) {
        return "Connected";
    }
    return "Offline";
}

String systemStatusVisionSummary(const SystemStatusViewModel& status) {
    return status.ai.visionReady ? "Ready" : "No endpoint";
}

String systemStatusWifiLine(const SystemStatusViewModel& status) {
    if (!status.wifi.connected) {
        return "Offline";
    }
    return String("Online ") + status.wifi.rssi + " dBm";
}

String systemStatusSdLine(const SystemStatusViewModel& status) {
    if (status.sd.ready) {
        return "Ready";
    }
    if (status.sd.statusText.length() == 0) {
        return "Unavailable";
    }
    return trimStatusText(status.sd.statusText, 22);
}

String systemStatusAudioLine(const SystemStatusViewModel& status) {
    switch (status.music.playbackState) {
        case SystemMusicState::PLAYING:
            return "Music playing";
        case SystemMusicState::PAUSED:
            return "Music paused";
        case SystemMusicState::ERROR:
            return "Music error";
        case SystemMusicState::STOPPED:
        default:
            break;
    }

    if (status.ai.audioChannelOpen && status.ai.voiceState == SystemVoiceState::SPEAKING) {
        return "AI speaking";
    }
    if (status.ai.audioChannelOpen && status.ai.voiceState == SystemVoiceState::LISTENING) {
        return "AI listening";
    }
    if (status.ai.audioChannelOpen && status.ai.voiceState == SystemVoiceState::THINKING) {
        return "AI thinking";
    }
    return "Idle";
}

String systemStatusControlLine(const SystemStatusViewModel& status) {
    return systemBrightnessLabel(status.control.brightnessLevel) + " / " +
           systemVolumeLabel(status.control.volumeLevel);
}

String systemStatusMemoryLine(const SystemStatusViewModel& status) {
    return String("Heap ") + status.memory.heapKb + " KB / PSRAM " +
           status.memory.psramKb + " KB";
}

String systemStatusBriefSentence1(const SystemStatusViewModel& status) {
    String sentence;
    if (status.wifi.connected) {
        sentence = String("Wi-Fi 已连接，信号 ") + status.wifi.rssi + " dBm";
    } else if (status.wifi.configured) {
        sentence = "Wi-Fi 当前离线";
    } else {
        sentence = "Wi-Fi 还没有配置";
    }

    if (status.sd.ready) {
        sentence += "，SD 卡已就绪。";
    } else if (status.sd.statusText.length() > 0) {
        sentence += "，SD 卡暂不可用（" + trimStatusText(status.sd.statusText, 14) + "）。";
    } else {
        sentence += "，SD 卡暂不可用。";
    }
    return sentence;
}

String systemStatusBriefSentence2(const SystemStatusViewModel& status) {
    String sentence = musicSentencePart(status);
    sentence += "，";
    sentence += aiSentencePart(status);
    sentence += status.ai.visionReady ? "，视觉接口已就绪。" : "，视觉接口还没配置。";
    return sentence;
}

String systemStatusMemorySentence(const SystemStatusViewModel& status) {
    return String("当前堆内存 ") + status.memory.heapKb + " KB，PSRAM " +
           status.memory.psramKb + " KB。";
}
