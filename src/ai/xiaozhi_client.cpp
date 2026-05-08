#include "xiaozhi_client.h"
#include "config/app_config.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>
#include <esp_system.h>

XiaoZhiClient::XiaoZhiClient() = default;

bool XiaoZhiClient::begin() {
    initialized_ = true;
    connected_ = false;
    activated_ = false;
    hasActivationCode_ = false;
    lastHttpCode_ = 0;
    lastError_ = "";
    webSocketUrl_ = "";
    token_ = "";
    mqttBroker_ = "";
    mqttPort_ = 0;
    deviceId_ = getMacAddress();
    deviceId_.toLowerCase();
    String macClean = deviceId_;
    macClean.replace(":", "");
    // Generate a UUID-formatted client ID from MAC (server requires UUID format)
    char uuidBuf[37];
    snprintf(uuidBuf, sizeof(uuidBuf),
             "%.8s-%.4s-4%.3s-a%.3s-%.2s%.6s%.4s",
             macClean.c_str(),
             macClean.c_str() + 8,
             macClean.c_str() + 1,
             macClean.c_str() + 5,
             macClean.c_str() + 10,
             macClean.c_str(),
             macClean.c_str() + 6);
    clientId_ = uuidBuf;
    return true;
}

bool XiaoZhiClient::end() {
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

String XiaoZhiClient::getDeviceId() {
    return deviceId_;
}

String XiaoZhiClient::buildUserAgent() const {
    String userAgent = XIAOZHI_BOARD_TYPE;
    userAgent += "/";
    userAgent += XIAOZHI_FIRMWARE_VERSION;
    return userAgent;
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
    String compileTime = String(__DATE__) + "T" + String(__TIME__) + "Z";
    app["compile_time"] = compileTime;
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
    if (!XIAOZHI_REAL_ACTIVATION) {
        Serial.println("XiaoZhi: placeholder mode, skipping OTA request");
        return false;
    }

    if (!initialized_ || WiFi.status() != WL_CONNECTED) {
        connected_ = false;
        Serial.println("XiaoZhi: no WiFi, cannot request activation");
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

    if (!http.begin(client, OTA_URL)) {
        Serial.println("XiaoZhi: HTTP begin failed");
        return false;
    }

    String userAgent = buildUserAgent();
    http.setUserAgent(userAgent);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Activation-Version", "1");
    http.addHeader("Device-Id", deviceId_);
    http.addHeader("Client-Id", clientId_);
    http.addHeader("Accept-Language", "zh-CN");

    String body = buildSystemInfoJson();
    Serial.printf("XiaoZhi: OTA identity: %s\n", userAgent.c_str());
    Serial.printf("XiaoZhi: OTA POST body: %s\n", body.c_str());

    int code = http.POST(body);
    if (code != 200) {
        String errBody = http.getString();
        Serial.printf("XiaoZhi: OTA POST failed, code=%d, body=%s\n", code, errBody.c_str());
        lastHttpCode_ = code;
        lastActivationErrorMs_ = millis();
        lastError_ = "HTTP " + String(code) + ": " + errBody;
        connected_ = false;
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
        lastError_ = String("JSON parse failed: ") + err.c_str();
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
        if (!ws["url"].isNull()) {
            webSocketUrl_ = ws["url"].as<String>();
        }
        if (!ws["token"].isNull()) {
            token_ = ws["token"].as<String>();
        }
        if (webSocketUrl_.length() > 0 && token_.length() > 0) {
            activated_ = true;
            Serial.println("XiaoZhi: device activated, got WS config");
        }
    }

    if (resp["mqtt"].is<JsonObject>()) {
        JsonObject mqtt = resp["mqtt"];
        if (!mqtt["endpoint"].isNull()) {
            mqttBroker_ = mqtt["endpoint"].as<String>();
        } else if (!mqtt["broker"].isNull()) {
            mqttBroker_ = mqtt["broker"].as<String>();
        }
        if (!mqtt["port"].isNull()) {
            mqttPort_ = mqtt["port"].as<int>();
        }
        if (mqttBroker_.length() > 0) {
            Serial.printf("XiaoZhi: MQTT broker %s:%d\n", mqttBroker_.c_str(), mqttPort_);
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

bool XiaoZhiClient::isConnected() const {
    return connected_;
}

bool XiaoZhiClient::isActivated() const {
    return activated_;
}

String XiaoZhiClient::getActivationCode() const {
    return activationCode_;
}

String XiaoZhiClient::getActivationMessage() const {
    return activationMessage_;
}

String XiaoZhiClient::getWebSocketUrl() const {
    return webSocketUrl_;
}

String XiaoZhiClient::getToken() const {
    return token_;
}

bool XiaoZhiClient::hasActivationCode() const {
    return hasActivationCode_;
}

String XiaoZhiClient::getMqttBroker() const {
    return mqttBroker_;
}

int XiaoZhiClient::getMqttPort() const {
    return mqttPort_;
}

String XiaoZhiClient::getLastError() const {
    return lastError_;
}

String XiaoZhiClient::getFirmwareIdentity() const {
    return buildUserAgent();
}

bool XiaoZhiClient::startListening() {
    if (!initialized_) return false;
    setState(VoiceState::LISTENING);
    if (!XIAOZHI_REAL_ACTIVATION) {
        Serial.println("XiaoZhi: placeholder mode, listening simulated");
        return true;
    }
    if (!activated_ && !hasActivationCode_) {
        Serial.println("XiaoZhi: not activated, requesting code...");
        requestActivationCode();
    }
    return true;
}

bool XiaoZhiClient::stopListening() {
    setState(VoiceState::IDLE);
    return true;
}

void XiaoZhiClient::setStateCallback(std::function<void(VoiceState)> cb) {
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
