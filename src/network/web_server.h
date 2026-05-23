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

    httpd_handle_t server_ = nullptr;
    WebControlCallbacks callbacks_;
};
