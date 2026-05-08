#include "xiaozhi_client.h"
#include "config/app_config.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>

XiaoZhiClient::XiaoZhiClient() = default;

bool XiaoZhiClient::begin() {
    initialized_ = true;
    connected_ = false;
    activated_ = false;
    hasActivationCode_ = false;
    deviceId_ = getMacAddress();
    clientId_ = "cores3-" + deviceId_;
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

bool XiaoZhiClient::requestActivationCode() {
    if (!initialized_ || WiFi.status() != WL_CONNECTED) {
        Serial.println("XiaoZhi: no WiFi, cannot request activation");
        return false;
    }

    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();

    if (!http.begin(client, OTA_URL)) {
        Serial.println("XiaoZhi: HTTP begin failed");
        return false;
    }

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Device-Id", deviceId_);
    http.addHeader("Client-Id", clientId_);
    http.addHeader("Accept-Language", "zh-CN");

    StaticJsonDocument<256> doc;
    doc["device_id"] = deviceId_;
    doc["client_id"] = clientId_;
    String body;
    serializeJson(doc, body);

    int code = http.POST(body);
    if (code != 200) {
        Serial.printf("XiaoZhi: OTA POST failed, code=%d\n", code);
        http.end();
        return false;
    }

    String response = http.getString();
    http.end();

    StaticJsonDocument<1024> resp;
    DeserializationError err = deserializeJson(resp, response);
    if (err) {
        Serial.printf("XiaoZhi: JSON parse failed: %s\n", err.c_str());
        return false;
    }

    if (resp.containsKey("activation")) {
        JsonObject act = resp["activation"];
        if (act.containsKey("code")) {
            activationCode_ = act["code"].as<String>();
            hasActivationCode_ = true;
            Serial.printf("XiaoZhi: activation code: %s\n", activationCode_.c_str());
        }
        if (act.containsKey("message")) {
            activationMessage_ = act["message"].as<String>();
        }
    }

    if (resp.containsKey("websocket")) {
        JsonObject ws = resp["websocket"];
        if (ws.containsKey("url")) {
            webSocketUrl_ = ws["url"].as<String>();
        }
        if (ws.containsKey("token")) {
            token_ = ws["token"].as<String>();
        }
        if (webSocketUrl_.length() > 0 && token_.length() > 0) {
            activated_ = true;
            Serial.println("XiaoZhi: device already activated, got WS config");
        }
    }

    if (resp.containsKey("mqtt")) {
        JsonObject mqtt = resp["mqtt"];
    }

    return hasActivationCode_ || activated_;
}

bool XiaoZhiClient::checkActivation() {
    if (!initialized_ || WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();

    String url = String(OTA_URL) + "/activate";
    if (!http.begin(client, url)) return false;

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Device-Id", deviceId_);
    http.addHeader("Client-Id", clientId_);

    StaticJsonDocument<128> doc;
    doc["activation_code"] = activationCode_;
    String body;
    serializeJson(doc, body);

    int code = http.POST(body);
    http.end();

    if (code == 200) {
        activated_ = true;
        Serial.println("XiaoZhi: activation confirmed!");
        return requestActivationCode();
    }

    return false;
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

bool XiaoZhiClient::startListening() {
    if (!initialized_) return false;
    setState(VoiceState::LISTENING);
    if (!activated_) {
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
