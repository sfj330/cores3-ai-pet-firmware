#pragma once

#include <Arduino.h>
#include <esp_http_server.h>
#include <functional>

struct WebControlCallbacks {
    std::function<String()> getStatus;
    std::function<bool(const String& page)> navigatePage;
    std::function<void(const String& level)> setBrightness;
    std::function<void(const String& level)> setVolume;
    std::function<void(const String& action)> sleepControl;
    std::function<bool(const String& action)> servoControl;
    std::function<String()> listPhotos;
    std::function<bool(const String& name, uint8_t** data, size_t* len)> getPhoto;
    std::function<bool(const String& name)> deletePhoto;
    std::function<bool(uint8_t** data, size_t* len)> captureJpeg;
    std::function<void(uint8_t* data)> releaseJpeg;
    std::function<void()> onOtaStart;
    std::function<void()> onOtaEnd;
    std::function<String()> getFaceState;
    std::function<bool(const String& emotion)> setEmotion;
    std::function<bool(const String& action)> doAction;
};

class PetWebServer {
public:
    PetWebServer();
    bool begin(uint16_t port = 80);
    void stop();
    bool isRunning() const { return server_ != nullptr; }

    void setCallbacks(const WebControlCallbacks& cb) { callbacks_ = cb; }

private:
    static esp_err_t handleRoot(httpd_req_t* req);
    static esp_err_t handleStatus(httpd_req_t* req);
    static esp_err_t handlePage(httpd_req_t* req);
    static esp_err_t handleBrightness(httpd_req_t* req);
    static esp_err_t handleVolume(httpd_req_t* req);
    static esp_err_t handleSleep(httpd_req_t* req);
    static esp_err_t handleServo(httpd_req_t* req);
    static esp_err_t handlePhotoList(httpd_req_t* req);
    static esp_err_t handlePhotoGet(httpd_req_t* req);
    static esp_err_t handlePhotoDelete(httpd_req_t* req);
    static esp_err_t handleStream(httpd_req_t* req);
    static esp_err_t handleOta(httpd_req_t* req);
    static esp_err_t handleFaceState(httpd_req_t* req);
    static esp_err_t handleEmotion(httpd_req_t* req);
    static esp_err_t handleAction(httpd_req_t* req);

    httpd_handle_t server_ = nullptr;
    WebControlCallbacks callbacks_;
};
