#include "web_server.h"
#include "network/web_ui.h"
#include <ArduinoJson.h>

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

bool PetWebServer::begin(uint16_t port) {
    if (server_) return true;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.max_uri_handlers = 12;
    config.stack_size = 8192;
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
