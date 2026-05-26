#include "web_server.h"
#include "network/web_ui.h"
#include <ArduinoJson.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

PetWebServer::PetWebServer() {}

static String readRequestBody(httpd_req_t* req) {
    int len = req->content_len;
    if (len <= 0 || len > 512) return String();
    char* buf = (char*)malloc(len + 1);
    if (!buf) return String();
    int ret = httpd_req_recv(req, buf, len);
    if (ret <= 0) { free(buf); return String(); }
    buf[ret] = '\0';
    String body(buf);
    free(buf);
    return body;
}

esp_err_t PetWebServer::handleRoot(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, WEB_UI_HTML, strlen(WEB_UI_HTML));
    return ESP_OK;
}

esp_err_t PetWebServer::handleStatus(httpd_req_t* req) {
    PetWebServer* self = (PetWebServer*)req->user_ctx;
    String json = self->callbacks_.getStatus ? self->callbacks_.getStatus() : "{}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json.c_str(), json.length());
    return ESP_OK;
}

esp_err_t PetWebServer::handlePage(httpd_req_t* req) {
    PetWebServer* self = (PetWebServer*)req->user_ctx;
    String body = readRequestBody(req);
    JsonDocument doc;
    deserializeJson(doc, body);
    String page = doc["page"] | "";
    bool ok = self->callbacks_.navigatePage ? self->callbacks_.navigatePage(page) : false;
    String resp = ok ? "{\"ok\":true}" : "{\"ok\":false}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp.c_str(), resp.length());
    return ESP_OK;
}

esp_err_t PetWebServer::handleBrightness(httpd_req_t* req) {
    PetWebServer* self = (PetWebServer*)req->user_ctx;
    String body = readRequestBody(req);
    JsonDocument doc;
    deserializeJson(doc, body);
    String level = doc["level"] | "";
    if (self->callbacks_.setBrightness) self->callbacks_.setBrightness(level);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

esp_err_t PetWebServer::handleVolume(httpd_req_t* req) {
    PetWebServer* self = (PetWebServer*)req->user_ctx;
    String body = readRequestBody(req);
    JsonDocument doc;
    deserializeJson(doc, body);
    String level = doc["level"] | "";
    if (self->callbacks_.setVolume) self->callbacks_.setVolume(level);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

esp_err_t PetWebServer::handleSleep(httpd_req_t* req) {
    PetWebServer* self = (PetWebServer*)req->user_ctx;
    String body = readRequestBody(req);
    JsonDocument doc;
    deserializeJson(doc, body);
    String action = doc["action"] | "";
    if (self->callbacks_.sleepControl) self->callbacks_.sleepControl(action);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

esp_err_t PetWebServer::handleServo(httpd_req_t* req) {
    PetWebServer* self = (PetWebServer*)req->user_ctx;
    String body = readRequestBody(req);
    JsonDocument doc;
    deserializeJson(doc, body);
    String action = doc["action"] | "";
    bool ok = self->callbacks_.servoControl ? self->callbacks_.servoControl(action) : false;
    String resp = ok ? "{\"ok\":true}" : "{\"ok\":false}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp.c_str(), resp.length());
    return ESP_OK;
}

esp_err_t PetWebServer::handlePhotoList(httpd_req_t* req) {
    PetWebServer* self = (PetWebServer*)req->user_ctx;
    String json = self->callbacks_.listPhotos ? self->callbacks_.listPhotos() : "[]";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json.c_str(), json.length());
    return ESP_OK;
}

esp_err_t PetWebServer::handlePhotoGet(httpd_req_t* req) {
    PetWebServer* self = (PetWebServer*)req->user_ctx;
    const char* uri = req->uri;
    // Extract filename from /api/photos/IMG_XXXX.jpg
    const char* prefix = "/api/photos/";
    if (strncmp(uri, prefix, strlen(prefix)) != 0) {
        httpd_resp_send_404(req);
        return ESP_OK;
    }
    String name = String(uri + strlen(prefix));
    if (name.length() == 0) {
        httpd_resp_send_404(req);
        return ESP_OK;
    }

    uint8_t* data = nullptr;
    size_t len = 0;
    bool ok = self->callbacks_.getPhoto ? self->callbacks_.getPhoto(name, &data, &len) : false;
    if (!ok || !data) {
        httpd_resp_send_404(req);
        return ESP_OK;
    }

    httpd_resp_set_type(req, "image/jpeg");
    // Send in chunks to avoid large stack usage
    size_t sent = 0;
    while (sent < len) {
        size_t chunk = len - sent;
        if (chunk > 4096) chunk = 4096;
        httpd_resp_send_chunk(req, (const char*)(data + sent), chunk);
        sent += chunk;
    }
    httpd_resp_send_chunk(req, nullptr, 0);
    free(data);
    return ESP_OK;
}

esp_err_t PetWebServer::handlePhotoDelete(httpd_req_t* req) {
    PetWebServer* self = (PetWebServer*)req->user_ctx;
    String body = readRequestBody(req);
    JsonDocument doc;
    deserializeJson(doc, body);
    String name = doc["name"] | "";
    bool ok = self->callbacks_.deletePhoto ? self->callbacks_.deletePhoto(name) : false;
    String resp = ok ? "{\"ok\":true}" : "{\"ok\":false}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp.c_str(), resp.length());
    return ESP_OK;
}

esp_err_t PetWebServer::handleStream(httpd_req_t* req) {
    PetWebServer* self = (PetWebServer*)req->user_ctx;
    if (!self->callbacks_.captureJpeg || !self->callbacks_.releaseJpeg) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Camera not available");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    static const char* boundary = "\r\n--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
    char hdr[128];

    while (true) {
        uint8_t* data = nullptr;
        size_t len = 0;
        if (!self->callbacks_.captureJpeg(&data, &len) || !data) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        int hdrLen = snprintf(hdr, sizeof(hdr), boundary, (unsigned)len);
        esp_err_t err = httpd_resp_send_chunk(req, hdr, hdrLen);
        if (err != ESP_OK) {
            self->callbacks_.releaseJpeg(data);
            break;
        }

        size_t sent = 0;
        while (sent < len) {
            size_t chunk = len - sent;
            if (chunk > 4096) chunk = 4096;
            err = httpd_resp_send_chunk(req, (const char*)(data + sent), chunk);
            if (err != ESP_OK) break;
            sent += chunk;
        }
        self->callbacks_.releaseJpeg(data);
        if (err != ESP_OK) break;

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    return ESP_OK;
}

esp_err_t PetWebServer::handleOta(httpd_req_t* req) {
    PetWebServer* self = (PetWebServer*)req->user_ctx;
    int contentLen = req->content_len;
    if (contentLen <= 0) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"ok\":false,\"msg\":\"No data\"}");
        return ESP_OK;
    }

    if (self->callbacks_.onOtaStart) self->callbacks_.onOtaStart();

    const esp_partition_t* updatePartition = esp_ota_get_next_update_partition(nullptr);
    if (!updatePartition) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"ok\":false,\"msg\":\"No OTA partition\"}");
        if (self->callbacks_.onOtaEnd) self->callbacks_.onOtaEnd();
        return ESP_OK;
    }

    esp_ota_handle_t otaHandle = 0;
    esp_err_t err = esp_ota_begin(updatePartition, OTA_WITH_SEQUENTIAL_WRITES, &otaHandle);
    if (err != ESP_OK) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"ok\":false,\"msg\":\"OTA begin failed\"}");
        if (self->callbacks_.onOtaEnd) self->callbacks_.onOtaEnd();
        return ESP_OK;
    }

    char* buf = (char*)malloc(4096);
    if (!buf) {
        esp_ota_abort(otaHandle);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"ok\":false,\"msg\":\"No memory\"}");
        if (self->callbacks_.onOtaEnd) self->callbacks_.onOtaEnd();
        return ESP_OK;
    }

    int remaining = contentLen;
    bool success = true;
    while (remaining > 0) {
        int toRead = remaining > 4096 ? 4096 : remaining;
        int received = httpd_req_recv(req, buf, toRead);
        if (received <= 0) {
            success = false;
            break;
        }
        err = esp_ota_write(otaHandle, buf, received);
        if (err != ESP_OK) {
            success = false;
            break;
        }
        remaining -= received;
    }
    free(buf);

    if (!success) {
        esp_ota_abort(otaHandle);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"ok\":false,\"msg\":\"Write failed\"}");
        if (self->callbacks_.onOtaEnd) self->callbacks_.onOtaEnd();
        return ESP_OK;
    }

    err = esp_ota_end(otaHandle);
    if (err != ESP_OK) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"ok\":false,\"msg\":\"OTA end failed\"}");
        if (self->callbacks_.onOtaEnd) self->callbacks_.onOtaEnd();
        return ESP_OK;
    }

    err = esp_ota_set_boot_partition(updatePartition);
    if (err != ESP_OK) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"ok\":false,\"msg\":\"Set boot partition failed\"}");
        if (self->callbacks_.onOtaEnd) self->callbacks_.onOtaEnd();
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true,\"msg\":\"Rebooting...\"}");

    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    return ESP_OK;
}

esp_err_t PetWebServer::handleFaceState(httpd_req_t* req) {
    PetWebServer* self = (PetWebServer*)req->user_ctx;
    String json = self->callbacks_.getFaceState ? self->callbacks_.getFaceState() : "{}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json.c_str(), json.length());
    return ESP_OK;
}

esp_err_t PetWebServer::handleEmotion(httpd_req_t* req) {
    PetWebServer* self = (PetWebServer*)req->user_ctx;
    String body = readRequestBody(req);
    JsonDocument doc;
    deserializeJson(doc, body);
    String emotion = doc["emotion"] | "";
    bool ok = self->callbacks_.setEmotion ? self->callbacks_.setEmotion(emotion) : false;
    String resp = ok ? "{\"ok\":true}" : "{\"ok\":false}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp.c_str(), resp.length());
    return ESP_OK;
}

esp_err_t PetWebServer::handleAction(httpd_req_t* req) {
    PetWebServer* self = (PetWebServer*)req->user_ctx;
    String body = readRequestBody(req);
    JsonDocument doc;
    deserializeJson(doc, body);
    String action = doc["action"] | "";
    bool ok = self->callbacks_.doAction ? self->callbacks_.doAction(action) : false;
    String resp = ok ? "{\"ok\":true}" : "{\"ok\":false}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp.c_str(), resp.length());
    return ESP_OK;
}

bool PetWebServer::begin(uint16_t port) {
    if (server_) return true;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.max_uri_handlers = 16;
    config.stack_size = 16384;
    config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&server_, &config) != ESP_OK) {
        Serial.println("WebServer: start failed");
        return false;
    }

    httpd_uri_t uris[] = {
        {"/", HTTP_GET, handleRoot, this},
        {"/api/status", HTTP_GET, handleStatus, this},
        {"/api/page", HTTP_POST, handlePage, this},
        {"/api/brightness", HTTP_POST, handleBrightness, this},
        {"/api/volume", HTTP_POST, handleVolume, this},
        {"/api/sleep", HTTP_POST, handleSleep, this},
        {"/api/servo", HTTP_POST, handleServo, this},
        {"/api/photos", HTTP_GET, handlePhotoList, this},
        {"/api/photos/*", HTTP_GET, handlePhotoGet, this},
        {"/api/photos", HTTP_DELETE, handlePhotoDelete, this},
        {"/api/stream", HTTP_GET, handleStream, this},
        {"/api/ota", HTTP_POST, handleOta, this},
        {"/api/face", HTTP_GET, handleFaceState, this},
        {"/api/emotion", HTTP_POST, handleEmotion, this},
        {"/api/action", HTTP_POST, handleAction, this},
    };

    for (auto& uri : uris) {
        httpd_register_uri_handler(server_, &uri);
    }

    Serial.printf("WebServer: started on port %d\n", port);
    return true;
}

void PetWebServer::stop() {
    if (server_) {
        httpd_stop(server_);
        server_ = nullptr;
        Serial.println("WebServer: stopped");
    }
}
