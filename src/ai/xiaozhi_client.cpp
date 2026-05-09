#include "xiaozhi_client.h"
#include "config/app_config.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <M5CoreS3.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

XiaoZhiClient::XiaoZhiClient() = default;

bool XiaoZhiClient::begin() {
    initialized_ = true;
    connected_ = false;
    activated_ = false;
    hasActivationCode_ = false;
    wsConnected_ = false;
    audioChannelOpen_ = false;
    audioInitialized_ = false;
    serverHelloReceived_ = false;
    opusReady_ = false;
    openingAudioChannel_ = false;
    micActive_ = false;
    speakerActive_ = false;
    listenRequested_ = false;
    listenStarted_ = false;
    mcpReady_ = false;
    pendingStartListening_ = false;
    pendingStopListening_ = false;
    lastHttpCode_ = 0;
    lastError_ = "";
    webSocketUrl_ = "";
    token_ = "";
    sessionId_ = "";
    deviceId_ = getMacAddress();
    deviceId_.toLowerCase();
    String macClean = deviceId_;
    macClean.replace(":", "");
    char uuidBuf[37];
    snprintf(uuidBuf, sizeof(uuidBuf),
             "%.8s-%.4s-4%.3s-a%.3s-%.2s%.6s%.4s",
             macClean.c_str(), macClean.c_str() + 8,
             macClean.c_str() + 1, macClean.c_str() + 5,
             macClean.c_str() + 10, macClean.c_str(),
             macClean.c_str() + 6);
    clientId_ = uuidBuf;

    pcmCaptureBuf_ = (int16_t*)heap_caps_malloc(AUDIO_FRAME_SAMPLES * sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    for (int i = 0; i < PLAYBACK_BUFFER_COUNT; ++i) {
        pcmPlaybackBufs_[i] = (int16_t*)heap_caps_malloc(OPUS_MAX_DECODE_SAMPLES * sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    opusEncodeBuf_ = (uint8_t*)heap_caps_malloc(OPUS_MAX_PACKET_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    opusTxQueue_ = xQueueCreate(4, sizeof(OpusTxPacket));

    bool playbackBuffersOk = true;
    for (int i = 0; i < PLAYBACK_BUFFER_COUNT; ++i) {
        playbackBuffersOk = playbackBuffersOk && pcmPlaybackBufs_[i] != nullptr;
    }
    if (!pcmCaptureBuf_ || !playbackBuffersOk || !opusEncodeBuf_ || !opusTxQueue_) {
        Serial.println("XiaoZhi: audio buffer allocation failed");
        lastError_ = "Audio alloc failed";
    }

    Serial.println("XiaoZhiClient OK");
    return true;
}

bool XiaoZhiClient::end() {
    closeAudioChannel();
    deinitAudio();
    deinitOpusCodec();
    if (pcmCaptureBuf_) { heap_caps_free(pcmCaptureBuf_); pcmCaptureBuf_ = nullptr; }
    for (int i = 0; i < PLAYBACK_BUFFER_COUNT; ++i) {
        if (pcmPlaybackBufs_[i]) {
            heap_caps_free(pcmPlaybackBufs_[i]);
            pcmPlaybackBufs_[i] = nullptr;
        }
    }
    if (opusEncodeBuf_) { heap_caps_free(opusEncodeBuf_); opusEncodeBuf_ = nullptr; }
    if (opusTxQueue_) {
        vQueueDelete(opusTxQueue_);
        opusTxQueue_ = nullptr;
    }
    activated_ = false;
    connected_ = false;
    initialized_ = false;
    return true;
}

String XiaoZhiClient::getMacAddress() {
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

String XiaoZhiClient::getDeviceId() { return deviceId_; }

String XiaoZhiClient::buildUserAgent() const {
    return String(XIAOZHI_BOARD_TYPE) + "/" + XIAOZHI_FIRMWARE_VERSION;
}

String XiaoZhiClient::buildSystemInfoJson() const {
    JsonDocument doc;
    doc["version"] = 2;
    doc["language"] = "zh-CN";
    doc["flash_size"] = ESP.getFlashChipSize();
    doc["minimum_free_heap_size"] = ESP.getMinFreeHeap();
    doc["mac_address"] = deviceId_;
    doc["uuid"] = clientId_;
    doc["chip_model_name"] = XIAOZHI_CHIP_ID;
    esp_chip_info_t chipInfoRaw;
    esp_chip_info(&chipInfoRaw);
    JsonObject chipInfo = doc["chip_info"].to<JsonObject>();
    chipInfo["model"] = static_cast<int>(chipInfoRaw.model);
    chipInfo["cores"] = chipInfoRaw.cores;
    chipInfo["revision"] = chipInfoRaw.revision;
    chipInfo["features"] = chipInfoRaw.features;
    JsonObject app = doc["application"].to<JsonObject>();
    app["name"] = XIAOZHI_APP_NAME;
    app["version"] = XIAOZHI_FIRMWARE_VERSION;
    app["compile_time"] = String(__DATE__) + "T" + String(__TIME__) + "Z";
    app["idf_version"] = ESP.getSdkVersion();
    app["elf_sha256"] = "";
    doc["partition_table"].to<JsonArray>();
    JsonObject ota = doc["ota"].to<JsonObject>();
    ota["label"] = "factory";
    String body;
    serializeJson(doc, body);
    return body;
}

bool XiaoZhiClient::requestActivationCode() {
    if (!XIAOZHI_REAL_ACTIVATION) return false;
    if (!initialized_ || WiFi.status() != WL_CONNECTED) {
        connected_ = false;
        return false;
    }
    unsigned long now = millis();
    if (lastHttpCode_ >= 400 && lastActivationErrorMs_ > 0 &&
        now - lastActivationErrorMs_ < ACTIVATION_ERROR_COOLDOWN_MS) {
        return false;
    }
    lastActivationAttemptMs_ = now;

    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    if (!http.begin(client, OTA_URL)) return false;

    http.setUserAgent(buildUserAgent());
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Activation-Version", "1");
    http.addHeader("Device-Id", deviceId_);
    http.addHeader("Client-Id", clientId_);
    http.addHeader("Accept-Language", "zh-CN");

    String body = buildSystemInfoJson();
    Serial.printf("XiaoZhi: OTA POST identity=%s\n", buildUserAgent().c_str());

    int code = http.POST(body);
    if (code != 200) {
        String errBody = http.getString();
        Serial.printf("XiaoZhi: OTA failed code=%d body=%s\n", code, errBody.c_str());
        lastHttpCode_ = code;
        lastActivationErrorMs_ = millis();
        lastError_ = "HTTP " + String(code);
        http.end();
        return false;
    }

    String response = http.getString();
    http.end();
    lastHttpCode_ = code;
    lastError_ = "";
    connected_ = true;
    Serial.printf("XiaoZhi: OTA response: %s\n", response.c_str());

    JsonDocument resp;
    DeserializationError err = deserializeJson(resp, response);
    if (err) {
        Serial.printf("XiaoZhi: JSON parse failed: %s\n", err.c_str());
        return false;
    }

    if (resp["activation"].is<JsonObject>()) {
        JsonObject act = resp["activation"];
        if (!act["code"].isNull()) {
            activationCode_ = act["code"].as<String>();
            hasActivationCode_ = true;
            Serial.printf("XiaoZhi: activation code: %s\n", activationCode_.c_str());
        }
        if (!act["message"].isNull()) {
            activationMessage_ = act["message"].as<String>();
        }
    }

    if (resp["websocket"].is<JsonObject>()) {
        JsonObject ws = resp["websocket"];
        if (!ws["url"].isNull()) webSocketUrl_ = ws["url"].as<String>();
        if (!ws["token"].isNull()) token_ = ws["token"].as<String>();
        if (webSocketUrl_.length() > 0 && token_.length() > 0) {
            activated_ = true;
            Serial.println("XiaoZhi: device activated, got WS config");
        }
    }

    return hasActivationCode_ || activated_;
}

bool XiaoZhiClient::checkActivation() {
    if (!XIAOZHI_REAL_ACTIVATION) return false;
    if (!initialized_ || WiFi.status() != WL_CONNECTED) return false;
    if (activated_) return true;
    requestActivationCode();
    return activated_;
}

bool XiaoZhiClient::isConnected() const { return connected_; }
bool XiaoZhiClient::isWsConnected() const { return wsConnected_; }
bool XiaoZhiClient::isActivated() const { return activated_; }
bool XiaoZhiClient::hasActivationCode() const { return hasActivationCode_; }
String XiaoZhiClient::getActivationCode() const { return activationCode_; }
String XiaoZhiClient::getActivationMessage() const { return activationMessage_; }
String XiaoZhiClient::getWebSocketUrl() const { return webSocketUrl_; }
String XiaoZhiClient::getToken() const { return token_; }
String XiaoZhiClient::getLastError() const { return lastError_; }
String XiaoZhiClient::getFirmwareIdentity() const { return buildUserAgent(); }

String XiaoZhiClient::getHelloMessage() {
    JsonDocument doc;
    doc["type"] = "hello";
    doc["version"] = protocolVersion_;
    JsonObject features = doc["features"].to<JsonObject>();
    features["mcp"] = true;
    doc["transport"] = "websocket";
    JsonObject audioParams = doc["audio_params"].to<JsonObject>();
    audioParams["format"] = "opus";
    audioParams["sample_rate"] = AUDIO_SAMPLE_RATE;
    audioParams["channels"] = AUDIO_CHANNELS;
    audioParams["frame_duration"] = AUDIO_FRAME_DURATION_MS;
    String msg;
    serializeJson(doc, msg);
    return msg;
}

bool XiaoZhiClient::openAudioChannel() {
    if (openingAudioChannel_) {
        return false;
    }
    unsigned long now = millis();
    if (lastAudioOpenAttemptMs_ > 0 && now - lastAudioOpenAttemptMs_ < AUDIO_OPEN_RETRY_COOLDOWN_MS) {
        return false;
    }
    openingAudioChannel_ = true;
    lastAudioOpenAttemptMs_ = now;

    if (!activated_ || webSocketUrl_.length() == 0) {
        Serial.println("XiaoZhi: cannot open audio channel, not activated");
        openingAudioChannel_ = false;
        return false;
    }

    Serial.printf("XiaoZhi: connecting WS to %s\n", webSocketUrl_.c_str());
    wsConnected_ = false;
    serverHelloReceived_ = false;
    audioChannelOpen_ = false;
    listenStarted_ = false;
    mcpReady_ = false;
    sessionId_ = "";
    if (opusTxQueue_) {
        xQueueReset(opusTxQueue_);
    }

    String wsUrl = webSocketUrl_;
    String wsPath = "/";
    int protoEnd = wsUrl.indexOf("://") + 3;
    int pathIdx = wsUrl.indexOf("/", protoEnd);
    if (pathIdx > 0) {
        wsPath = wsUrl.substring(pathIdx);
        wsUrl = wsUrl.substring(0, pathIdx);
    }

    int port = 443;
    bool useSSL = true;
    if (wsUrl.startsWith("ws://")) {
        wsUrl = wsUrl.substring(5);
        useSSL = false;
        port = 80;
    } else if (wsUrl.startsWith("wss://")) {
        wsUrl = wsUrl.substring(6);
    }

    int colonIdx = wsUrl.indexOf(":");
    if (colonIdx > 0) {
        port = wsUrl.substring(colonIdx + 1).toInt();
        wsUrl = wsUrl.substring(0, colonIdx);
    }

    if (useSSL) {
        webSocket_.beginSSL(wsUrl.c_str(), port, wsPath.c_str(), nullptr, "");
    } else {
        webSocket_.begin(wsUrl.c_str(), port, wsPath.c_str(), "");
    }

    String authVal = "Bearer " + token_;
    webSocket_.setAuthorization(authVal.c_str());
    String headers = "Protocol-Version: " + String(protocolVersion_) + "\r\n"
                     "Device-Id: " + deviceId_ + "\r\n"
                     "Client-Id: " + clientId_;
    webSocket_.setExtraHeaders(headers.c_str());

    webSocket_.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
        this->wsEvent(type, payload, length);
    });
    webSocket_.setReconnectInterval(5000);
    webSocket_.enableHeartbeat(15000, 3000, 2);

    unsigned long startWait = millis();
    while (!wsConnected_ && millis() - startWait < 10000) {
        webSocket_.loop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (!wsConnected_) {
        Serial.println("XiaoZhi: WS connect timeout");
        lastError_ = "WS connect timeout";
        openingAudioChannel_ = false;
        return false;
    }

    String hello = getHelloMessage();
    Serial.printf("XiaoZhi: sending hello: %s\n", hello.c_str());
    webSocket_.sendTXT(hello);

    startWait = millis();
    while (!serverHelloReceived_ && wsConnected_ && millis() - startWait < 10000) {
        webSocket_.loop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (!serverHelloReceived_) {
        Serial.println("XiaoZhi: server hello timeout");
        lastError_ = "Server hello timeout";
        webSocket_.disconnect();
        wsConnected_ = false;
        openingAudioChannel_ = false;
        return false;
    }

    audioChannelOpen_ = true;
    openingAudioChannel_ = false;
    return audioInitialized_ && opusReady_;
}

void XiaoZhiClient::closeAudioChannel() {
    audioCaptureRunning_ = false;
    audioChannelOpen_ = false;
    openingAudioChannel_ = false;
    listenRequested_ = false;
    listenStarted_ = false;
    mcpReady_ = false;
    pendingStartListening_ = false;
    pendingStopListening_ = false;

    if (audioCaptureTaskHandle_) {
        vTaskDelay(pdMS_TO_TICKS(100));
        vTaskDelete(audioCaptureTaskHandle_);
        audioCaptureTaskHandle_ = nullptr;
    }
    if (opusTxQueue_) {
        xQueueReset(opusTxQueue_);
    }

    if (wsConnected_) {
        webSocket_.disconnect();
        wsConnected_ = false;
    }
    deinitAudio();
    deinitOpusCodec();
    serverHelloReceived_ = false;
    listenStarted_ = false;
    mcpReady_ = false;
}

void XiaoZhiClient::wsEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("XiaoZhi: WS disconnected");
            wsConnected_ = false;
            audioChannelOpen_ = false;
            audioCaptureRunning_ = false;
            listenRequested_ = false;
            listenStarted_ = false;
            mcpReady_ = false;
            if (opusTxQueue_) {
                xQueueReset(opusTxQueue_);
            }
            setState(VoiceState::IDLE);
            break;
        case WStype_CONNECTED:
            Serial.println("XiaoZhi: WS connected");
            wsConnected_ = true;
            break;
        case WStype_TEXT:
            handleTextMessage((const char*)payload, length);
            break;
        case WStype_BIN:
            handleBinaryMessage(payload, length);
            break;
        default:
            break;
    }
}

void XiaoZhiClient::handleTextMessage(const char* data, size_t len) {
    Serial.printf("XiaoZhi: WS text: %.*s\n", (int)min((size_t)200, len), data);

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        Serial.printf("XiaoZhi: WS JSON parse failed: %s\n", err.c_str());
        return;
    }

    String type = doc["type"] | "";
    if (type == "hello") {
        parseServerHello(data, len);
    } else if (type == "mcp") {
        if (doc["payload"].is<JsonObject>()) {
            handleMcpMessage(doc["payload"].as<JsonObjectConst>());
        } else {
            Serial.println("XiaoZhi: MCP payload missing");
        }
    } else if (type == "tts") {
        String st = doc["state"] | "";
        if (st == "start") {
            audioCaptureRunning_ = false;
            listenStarted_ = false;
            if (opusTxQueue_) {
                xQueueReset(opusTxQueue_);
            }
            startSpeaker();
            setState(VoiceState::SPEAKING);
        } else if (st == "stop") {
            stopSpeaker();
            tryStartListeningStream();
            if (!listenStarted_) {
                setState(VoiceState::IDLE);
            }
        }
    } else if (type == "stt") {
        String st = doc["state"] | "";
        if (st == "stop") {
            setState(VoiceState::THINKING);
            audioCaptureRunning_ = false;
            listenStarted_ = false;
            if (opusTxQueue_) {
                xQueueReset(opusTxQueue_);
            }
        }
    } else if (type == "response") {
        String action = doc["action"] | "";
        if (action == "listen") {
            setState(VoiceState::LISTENING);
            audioCaptureRunning_ = true;
        }
    }
}

void XiaoZhiClient::handleMcpMessage(JsonObjectConst payload) {
    String method = payload["method"] | "";
    if (method.length() == 0 || method.startsWith("notifications")) {
        return;
    }

    if (payload["id"].isNull()) {
        Serial.printf("XiaoZhi: MCP request without id: %s\n", method.c_str());
        return;
    }

    int id = payload["id"].as<int>();
    Serial.printf("XiaoZhi: MCP method=%s id=%d\n", method.c_str(), id);

    if (method == "initialize") {
        JsonDocument result;
        const char* protocolVersion = "2024-11-05";
        if (payload["params"].is<JsonObject>()) {
            JsonObjectConst params = payload["params"].as<JsonObjectConst>();
            const char* requestedVersion = params["protocolVersion"] | nullptr;
            if (requestedVersion && requestedVersion[0] != '\0') {
                protocolVersion = requestedVersion;
            }
        }

        result["protocolVersion"] = protocolVersion;
        result["capabilities"]["tools"].to<JsonObject>();
        result["serverInfo"]["name"] = "CoreS3 AI Pet";
        result["serverInfo"]["version"] = XIAOZHI_FIRMWARE_VERSION;

        String resultJson;
        serializeJson(result, resultJson);
        sendMcpResult(id, resultJson);
    } else if (method == "tools/list") {
        sendMcpToolsList(id);
        mcpReady_ = true;
        tryStartListeningStream();
    } else if (method == "tools/call") {
        String toolName = "";
        if (payload["params"].is<JsonObject>()) {
            JsonObjectConst params = payload["params"].as<JsonObjectConst>();
            toolName = params["name"] | "";
        }
        if (toolName.length() == 0) {
            toolName = "unknown";
        }
        sendMcpError(id, "No tools are implemented on this CoreS3 build: " + toolName);
    } else {
        sendMcpError(id, "Method not implemented: " + method);
    }
}

void XiaoZhiClient::sendMcpResult(int id, const String& resultJson) {
    if (!wsConnected_ || sessionId_.length() == 0) return;
    String msg = "{\"session_id\":\"" + sessionId_ +
                 "\",\"type\":\"mcp\",\"payload\":{\"jsonrpc\":\"2.0\",\"id\":" +
                 String(id) + ",\"result\":" + resultJson + "}}";
    webSocket_.sendTXT(msg);
    Serial.printf("XiaoZhi: sent MCP result id=%d\n", id);
}

void XiaoZhiClient::sendMcpError(int id, const String& message) {
    if (!wsConnected_ || sessionId_.length() == 0) return;

    JsonDocument payload;
    payload["jsonrpc"] = "2.0";
    payload["id"] = id;
    payload["error"]["code"] = -32601;
    payload["error"]["message"] = message;

    String payloadJson;
    serializeJson(payload, payloadJson);
    String msg = "{\"session_id\":\"" + sessionId_ +
                 "\",\"type\":\"mcp\",\"payload\":" + payloadJson + "}";
    webSocket_.sendTXT(msg);
    Serial.printf("XiaoZhi: sent MCP error id=%d %s\n", id, message.c_str());
}

void XiaoZhiClient::sendMcpToolsList(int id) {
    JsonDocument result;
    result["tools"].to<JsonArray>();

    String resultJson;
    serializeJson(result, resultJson);
    sendMcpResult(id, resultJson);
}

void XiaoZhiClient::handleBinaryMessage(const uint8_t* data, size_t len) {
    if (!opusReady_ || !opusDecoder_ || len == 0) return;
    if (!speakerActive_ && !startSpeaker()) {
        return;
    }

    int16_t* playbackBuf = pcmPlaybackBufs_[playbackBufferIndex_];
    if (!playbackBuf) return;
    playbackBufferIndex_ = (playbackBufferIndex_ + 1) % PLAYBACK_BUFFER_COUNT;

    int decodedSamples = opus_decode(opusDecoder_, data, len, playbackBuf, OPUS_MAX_DECODE_SAMPLES, 0);
    if (decodedSamples <= 0) {
        Serial.printf("XiaoZhi: opus decode failed: %d len=%u\n", decodedSamples, static_cast<unsigned>(len));
        return;
    }

    M5.Speaker.playRaw(playbackBuf, decodedSamples, serverSampleRate_, false, 1, 0, false);
}

void XiaoZhiClient::parseServerHello(const char* data, size_t len) {
    JsonDocument doc;
    deserializeJson(doc, data, len);

    String transport = doc["transport"] | "";
    if (transport != "websocket") {
        Serial.printf("XiaoZhi: unsupported transport: %s\n", transport.c_str());
        return;
    }

    sessionId_ = doc["session_id"] | "";
    Serial.printf("XiaoZhi: session_id=%s\n", sessionId_.c_str());

    if (doc["audio_params"].is<JsonObject>()) {
        JsonObject ap = doc["audio_params"];
        if (!ap["sample_rate"].isNull()) serverSampleRate_ = ap["sample_rate"].as<int>();
        if (!ap["frame_duration"].isNull()) serverFrameDuration_ = ap["frame_duration"].as<int>();
    }

    Serial.printf("XiaoZhi: server audio %dHz %dms\n", serverSampleRate_, serverFrameDuration_);

    serverHelloReceived_ = true;

    if (!audioInitialized_ && !initAudio()) {
        Serial.println("XiaoZhi: audio init failed after server hello");
        return;
    }
    if (!micActive_ && !startMic()) {
        Serial.println("XiaoZhi: mic not available, cannot listen");
        return;
    }

    listenRequested_ = true;
    setState(VoiceState::LISTENING);
    audioCaptureRunning_ = false;
}

bool XiaoZhiClient::initAudio() {
    if (audioInitialized_) return true;
    if (!initOpusCodec()) {
        setState(VoiceState::ERROR);
        return false;
    }

    stopSpeaker();
    if (!startMic()) {
        setState(VoiceState::ERROR);
        return false;
    }

    audioInitialized_ = true;
    Serial.println("XiaoZhi: audio I/O initialized (half-duplex mic)");

    xTaskCreatePinnedToCore(
        [](void* arg) { static_cast<XiaoZhiClient*>(arg)->audioCaptureTask(); },
        "AudioCap", 32768, this, 4, &audioCaptureTaskHandle_, 0);

    return true;
}

void XiaoZhiClient::deinitAudio() {
    if (!audioInitialized_) return;
    stopMic();
    stopSpeaker();
    audioInitialized_ = false;
}

bool XiaoZhiClient::startMic() {
    if (micActive_ && M5.Mic.isRunning()) return true;

    stopSpeaker();
    auto mic_cfg = M5.Mic.config();
    mic_cfg.sample_rate = AUDIO_SAMPLE_RATE;
    mic_cfg.stereo = false;
    M5.Mic.config(mic_cfg);
    micActive_ = M5.Mic.begin();
    if (!micActive_) {
        Serial.println("XiaoZhi: mic begin failed");
        lastError_ = "Mic begin failed";
    }
    return micActive_;
}

void XiaoZhiClient::stopMic() {
    if (!micActive_ && !M5.Mic.isRunning()) return;
    audioCaptureRunning_ = false;
    unsigned long start = millis();
    while (M5.Mic.isRecording() && millis() - start < 500) {
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    M5.Mic.end();
    micActive_ = false;
}

bool XiaoZhiClient::startSpeaker() {
    if (speakerActive_ && M5.Speaker.isRunning()) return true;

    stopMic();
    auto spk_cfg = M5.Speaker.config();
    spk_cfg.sample_rate = 48000;
    spk_cfg.stereo = false;
    spk_cfg.dma_buf_count = 8;
    spk_cfg.dma_buf_len = 256;
    spk_cfg.task_priority = 5;
    M5.Speaker.config(spk_cfg);
    speakerActive_ = M5.Speaker.begin();
    if (speakerActive_) {
        M5.Speaker.setVolume(160);
    }
    if (!speakerActive_) {
        Serial.println("XiaoZhi: speaker begin failed");
        lastError_ = "Speaker begin failed";
    }
    return speakerActive_;
}

void XiaoZhiClient::stopSpeaker() {
    if (!speakerActive_ && !M5.Speaker.isRunning()) return;
    M5.Speaker.end();
    speakerActive_ = false;
}

bool XiaoZhiClient::initOpusCodec() {
    deinitOpusCodec();
    bool playbackBuffersOk = true;
    for (int i = 0; i < PLAYBACK_BUFFER_COUNT; ++i) {
        playbackBuffersOk = playbackBuffersOk && pcmPlaybackBufs_[i] != nullptr;
    }
    if (!opusEncodeBuf_ || !playbackBuffersOk) {
        lastError_ = "No audio buffers";
        return false;
    }

    int err = OPUS_OK;
    opusEncoder_ = opus_encoder_create(AUDIO_SAMPLE_RATE, AUDIO_CHANNELS, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK || !opusEncoder_) {
        Serial.printf("XiaoZhi: opus encoder create failed: %d\n", err);
        lastError_ = "Opus encoder failed";
        return false;
    }
    opus_encoder_ctl(opusEncoder_, OPUS_SET_BITRATE(OPUS_BITRATE));
    opus_encoder_ctl(opusEncoder_, OPUS_SET_COMPLEXITY(OPUS_COMPLEXITY));
    opus_encoder_ctl(opusEncoder_, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));

    err = OPUS_OK;
    opusDecoder_ = opus_decoder_create(serverSampleRate_, AUDIO_CHANNELS, &err);
    if (err != OPUS_OK || !opusDecoder_) {
        Serial.printf("XiaoZhi: opus decoder create failed: %d\n", err);
        lastError_ = "Opus decoder failed";
        deinitOpusCodec();
        return false;
    }

    opusReady_ = true;
    playbackBufferIndex_ = 0;
    Serial.printf("XiaoZhi: opus ready enc=%dHz dec=%dHz frame=%dms\n",
                  AUDIO_SAMPLE_RATE, serverSampleRate_, AUDIO_FRAME_DURATION_MS);
    return true;
}

void XiaoZhiClient::deinitOpusCodec() {
    if (opusEncoder_) {
        opus_encoder_destroy(opusEncoder_);
        opusEncoder_ = nullptr;
    }
    if (opusDecoder_) {
        opus_decoder_destroy(opusDecoder_);
        opusDecoder_ = nullptr;
    }
    opusReady_ = false;
}

void XiaoZhiClient::audioCaptureTask() {
    Serial.println("XiaoZhi: audio capture task started");
    while (true) {
        if (!audioCaptureRunning_ || !wsConnected_) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        if (micActive_ && M5.Mic.isRunning()) {
            size_t readLen = AUDIO_FRAME_SAMPLES;
            if (M5.Mic.record(pcmCaptureBuf_, readLen, AUDIO_SAMPLE_RATE, false)) {
                sendAudioFrame(pcmCaptureBuf_, readLen);
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

bool XiaoZhiClient::sendAudioFrame(const int16_t* pcmData, size_t sampleCount) {
    if (!wsConnected_ || !opusReady_ || !opusEncoder_ || !opusTxQueue_) return false;
    int encoded = opus_encode(opusEncoder_, pcmData, sampleCount, opusEncodeBuf_, OPUS_MAX_PACKET_BYTES);
    if (encoded <= 0) {
        Serial.printf("XiaoZhi: opus encode failed: %d\n", encoded);
        return false;
    }

    OpusTxPacket packet;
    packet.length = static_cast<uint16_t>(encoded);
    memcpy(packet.data, opusEncodeBuf_, encoded);
    if (xQueueSend(opusTxQueue_, &packet, 0) != pdTRUE) {
        return false;
    }
    return true;
}

void XiaoZhiClient::sendListenStart() {
    if (!wsConnected_ || sessionId_.length() == 0) return;
    JsonDocument doc;
    doc["session_id"] = sessionId_;
    doc["type"] = "listen";
    doc["state"] = "start";
    doc["mode"] = "auto";
    String msg;
    serializeJson(doc, msg);
    webSocket_.sendTXT(msg);
    Serial.println("XiaoZhi: sent listen start");
}

void XiaoZhiClient::sendListenStop() {
    if (!wsConnected_ || sessionId_.length() == 0) return;
    JsonDocument doc;
    doc["session_id"] = sessionId_;
    doc["type"] = "listen";
    doc["state"] = "stop";
    String msg;
    serializeJson(doc, msg);
    webSocket_.sendTXT(msg);
    Serial.println("XiaoZhi: sent listen stop");
}

void XiaoZhiClient::sendAbort() {
    if (!wsConnected_ || sessionId_.length() == 0) return;
    JsonDocument doc;
    doc["session_id"] = sessionId_;
    doc["type"] = "abort";
    String msg;
    serializeJson(doc, msg);
    webSocket_.sendTXT(msg);
}

bool XiaoZhiClient::startListening() {
    if (!initialized_) return false;
    pendingStartListening_ = true;
    setState(VoiceState::LISTENING);
    return true;
}

bool XiaoZhiClient::stopListening() {
    pendingStopListening_ = true;
    setState(VoiceState::IDLE);
    return true;
}

void XiaoZhiClient::tryStartListeningStream() {
    if (!listenRequested_ || listenStarted_ || !audioChannelOpen_ || !mcpReady_ || !wsConnected_) {
        return;
    }
    if (!micActive_ && !startMic()) {
        return;
    }

    sendListenStart();
    listenStarted_ = true;
    audioCaptureRunning_ = micActive_;
    setState(VoiceState::LISTENING);
}

void XiaoZhiClient::process() {
    if (pendingStopListening_) {
        pendingStopListening_ = false;
        listenRequested_ = false;
        listenStarted_ = false;
        if (audioChannelOpen_) {
            sendListenStop();
            audioCaptureRunning_ = false;
        }
        if (opusTxQueue_) {
            xQueueReset(opusTxQueue_);
        }
        setState(VoiceState::IDLE);
    }

    if (pendingStartListening_) {
        pendingStartListening_ = false;
        listenRequested_ = true;
        if (activated_ && !audioChannelOpen_) {
            openAudioChannel();
        }
        tryStartListeningStream();
        setState(VoiceState::LISTENING);
    }

    if (wsConnected_) {
        webSocket_.loop();
        flushOutgoingAudio();
    }
}

void XiaoZhiClient::setStateCallback(std::function<void(VoiceState)> cb) {
    callback_ = std::move(cb);
}

VoiceState XiaoZhiClient::getState() const { return state_; }

void XiaoZhiClient::flushOutgoingAudio() {
    if (!wsConnected_ || !opusTxQueue_) return;

    OpusTxPacket packet;
    while (xQueueReceive(opusTxQueue_, &packet, 0) == pdTRUE) {
        if (packet.length > 0) {
            webSocket_.sendBIN(packet.data, packet.length);
        }
    }
}

void XiaoZhiClient::setState(VoiceState state) {
    if (state_ != state) {
        state_ = state;
        if (callback_) callback_(state);
    }
}
