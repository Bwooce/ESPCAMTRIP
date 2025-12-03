#include "../NTRIP_Atlas.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>

// ESP32 platform callbacks for NTRIP Atlas
static Preferences preferences;

// HTTP client callback
static int esp32_http_stream(const char* host, int port, const char* path, const char* headers,
                           void* response_buffer, size_t buffer_size, void* platform_data) {
    HTTPClient http;
    WiFiClient client;

    String url = "http://" + String(host) + ":" + String(port) + String(path);
    http.begin(client, url);

    // Add any custom headers
    if (headers && strlen(headers) > 0) {
        http.addHeader("Authorization", headers);
    }

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        size_t payload_len = payload.length();
        if (payload_len > buffer_size - 1) {
            payload_len = buffer_size - 1;
        }
        memcpy(response_buffer, payload.c_str(), payload_len);
        ((char*)response_buffer)[payload_len] = '\0';
        http.end();
        return payload_len;
    }

    http.end();
    return -1;
}

// Preferences callback for failure tracking
static bool esp32_store_failure_data(const char* service_id, const void* data, size_t size, void* platform_data) {
    preferences.begin("ntrip_atlas", false);
    String key = "fail_" + String(service_id);
    bool result = preferences.putBytes(key.c_str(), data, size) == size;
    preferences.end();
    return result;
}

static bool esp32_load_failure_data(const char* service_id, void* data, size_t max_size, void* platform_data) {
    preferences.begin("ntrip_atlas", true);
    String key = "fail_" + String(service_id);
    size_t bytes_read = preferences.getBytesLength(key.c_str());
    if (bytes_read > 0 && bytes_read <= max_size) {
        preferences.getBytes(key.c_str(), data, bytes_read);
        preferences.end();
        return true;
    }
    preferences.end();
    return false;
}

static unsigned long esp32_get_time(void* platform_data) {
    return millis() / 1000; // Convert to seconds
}

bool ntrip_atlas_init_esp32() {
    // Initialize platform interface
    ntrip_platform_t platform = {
        .interface_version = 2,
        .http_stream = esp32_http_stream,
        .store_failure_data = esp32_store_failure_data,
        .load_failure_data = esp32_load_failure_data,
        .get_time = esp32_get_time,
        .platform_data = nullptr
    };

    return ntrip_atlas_init(&platform) == NTRIP_ATLAS_SUCCESS;
}