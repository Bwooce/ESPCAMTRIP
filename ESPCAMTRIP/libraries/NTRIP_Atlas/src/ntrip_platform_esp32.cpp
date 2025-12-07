/**
 * ESP32/Arduino Platform Implementation for NTRIP Atlas
 *
 * Provides WiFi-based HTTP streaming and secure credential storage using
 * WiFiClient/WiFiClientSecure and Preferences library.
 *
 * Copyright (c) 2024 NTRIP Atlas Contributors
 * Licensed under MIT License
 */

#include "ntrip_atlas.h"
#include "ntrip_atlas_config.h"

#ifdef ESP32

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>

// Preferences namespace for NTRIP Atlas data
#define PREFS_NAMESPACE "ntripatlas"
#define PREFS_CRED_NS   "ntripcreds"
#define PREFS_FAIL_NS   "ntripfail"

static Preferences prefs;

/**
 * HTTP streaming implementation for ESP32
 *
 * Uses WiFiClient to stream data in chunks, calling the callback
 * function for each received chunk without buffering the entire response.
 */
static int esp32_http_stream(
    const char* host,
    uint16_t port,
    uint8_t ssl,
    const char* path,
    ntrip_stream_callback_t on_data,
    void* user_context,
    uint32_t timeout_ms
) {
    WiFiClient* client;
    WiFiClientSecure* secure_client = NULL;

    // Create appropriate client based on SSL requirement
    if (ssl) {
        secure_client = new WiFiClientSecure();
        secure_client->setInsecure(); // Skip certificate validation for now
        client = secure_client;
    } else {
        client = new WiFiClient();
    }

    // Set timeout
    client->setTimeout(timeout_ms / 1000); // Convert ms to seconds

    // Connect to server
    if (!client->connect(host, port)) {
        delete client;
        return NTRIP_ATLAS_ERROR_NO_NETWORK;
    }

    // Send HTTP GET request
    client->printf("GET %s HTTP/1.1\r\n", path);
    client->printf("Host: %s\r\n", host);
    client->printf("User-Agent: NTRIP-Atlas/1.0\r\n");
    client->printf("Accept: */*\r\n");
    client->printf("Connection: close\r\n");
    client->printf("\r\n");

    // Wait for response headers
    uint32_t start_ms = millis();
    while (!client->available() && (millis() - start_ms) < timeout_ms) {
        delay(10);
    }

    if (!client->available()) {
        client->stop();
        delete client;
        return NTRIP_ATLAS_ERROR_TIMEOUT;
    }

    // Skip HTTP headers (read until \r\n\r\n)
    int header_state = 0;
    while (client->available() && header_state < 4) {
        char c = client->read();
        if (c == '\r' && header_state == 0) header_state = 1;
        else if (c == '\n' && header_state == 1) header_state = 2;
        else if (c == '\r' && header_state == 2) header_state = 3;
        else if (c == '\n' && header_state == 3) header_state = 4;
        else if (c != '\r' && c != '\n') header_state = 0;
    }

    // Stream response body in chunks
    char chunk_buffer[NTRIP_TCP_CHUNK_SIZE];
    int callback_result = 0;

    while (client->connected() || client->available()) {
        if (client->available()) {
            size_t bytes_read = client->readBytes(chunk_buffer, NTRIP_TCP_CHUNK_SIZE);

            if (bytes_read > 0) {
                // Call user callback with chunk
                callback_result = on_data(chunk_buffer, bytes_read, user_context);

                // If callback returns non-zero, stop streaming
                if (callback_result != 0) {
                    break;
                }
            }
        } else {
            delay(10); // Brief delay to allow more data to arrive
        }

        // Check timeout
        if ((millis() - start_ms) > timeout_ms) {
            client->stop();
            delete client;
            return NTRIP_ATLAS_ERROR_TIMEOUT;
        }
    }

    // Clean up
    client->stop();
    delete client;

    return NTRIP_ATLAS_SUCCESS;
}

/**
 * Send NMEA sentence to NTRIP connection
 *
 * For VRS networks that require position updates.
 * Connection pointer should be a WiFiClient*.
 */
static int esp32_send_nmea(
    void* connection,
    const char* nmea_sentence
) {
    if (!connection || !nmea_sentence) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    WiFiClient* client = (WiFiClient*)connection;

    if (!client->connected()) {
        return NTRIP_ATLAS_ERROR_NO_NETWORK;
    }

    size_t len = strlen(nmea_sentence);
    size_t written = client->write((const uint8_t*)nmea_sentence, len);

    if (written != len) {
        return NTRIP_ATLAS_ERROR_PLATFORM;
    }

    return NTRIP_ATLAS_SUCCESS;
}

/**
 * Store credential securely using ESP32 Preferences
 */
static int esp32_store_credential(const char* key, const char* value) {
    if (!key || !value) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    prefs.begin(PREFS_CRED_NS, false);
    prefs.putString(key, value);
    prefs.end();

    return NTRIP_ATLAS_SUCCESS;
}

/**
 * Load credential from ESP32 Preferences
 */
static int esp32_load_credential(const char* key, char* value, size_t max_len) {
    if (!key || !value || max_len == 0) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    prefs.begin(PREFS_CRED_NS, true); // Read-only
    String stored = prefs.getString(key, "");
    prefs.end();

    if (stored.length() == 0) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    strncpy(value, stored.c_str(), max_len - 1);
    value[max_len - 1] = '\0';

    return NTRIP_ATLAS_SUCCESS;
}

/**
 * Store service failure data
 */
static int esp32_store_failure_data(
    const char* service_id,
    const ntrip_service_failure_t* failure
) {
    if (!service_id || !failure) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    prefs.begin(PREFS_FAIL_NS, false);

    // Store as binary blob
    String key = String(service_id) + "_fail";
    prefs.putBytes(key.c_str(), failure, sizeof(ntrip_service_failure_t));

    prefs.end();

    return NTRIP_ATLAS_SUCCESS;
}

/**
 * Load service failure data
 */
static int esp32_load_failure_data(
    const char* service_id,
    ntrip_service_failure_t* failure
) {
    if (!service_id || !failure) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    prefs.begin(PREFS_FAIL_NS, true);

    String key = String(service_id) + "_fail";
    size_t len = prefs.getBytesLength(key.c_str());

    if (len != sizeof(ntrip_service_failure_t)) {
        prefs.end();
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    prefs.getBytes(key.c_str(), failure, sizeof(ntrip_service_failure_t));
    prefs.end();

    return NTRIP_ATLAS_SUCCESS;
}

/**
 * Clear service failure data
 */
static int esp32_clear_failure_data(const char* service_id) {
    if (!service_id) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    prefs.begin(PREFS_FAIL_NS, false);
    String key = String(service_id) + "_fail";
    prefs.remove(key.c_str());
    prefs.end();

    return NTRIP_ATLAS_SUCCESS;
}

/**
 * Log message using Arduino Serial
 */
static void esp32_log_message(int level, const char* message) {
    const char* level_str;
    switch (level) {
        case 0: level_str = "ERROR"; break;
        case 1: level_str = "WARN"; break;
        case 2: level_str = "INFO"; break;
        case 3: level_str = "DEBUG"; break;
        default: level_str = "UNKNOWN"; break;
    }

    Serial.printf("[NTRIP-%s] %s\n", level_str, message);
}

/**
 * Get milliseconds since boot
 */
static uint32_t esp32_get_time_ms(void) {
    return millis();
}

/**
 * Get seconds since epoch (requires NTP sync)
 */
static uint32_t esp32_get_time_seconds(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

/**
 * ESP32 Platform Implementation
 */
const ntrip_platform_t ntrip_platform_esp32 = {
    .interface_version = 2,
    .http_stream = esp32_http_stream,
    .send_nmea = esp32_send_nmea,
    .store_credential = esp32_store_credential,
    .load_credential = esp32_load_credential,
    .store_failure_data = esp32_store_failure_data,
    .load_failure_data = esp32_load_failure_data,
    .clear_failure_data = esp32_clear_failure_data,
    .log_message = esp32_log_message,
    .get_time_ms = esp32_get_time_ms,
    .get_time_seconds = esp32_get_time_seconds
};

#endif // ESP32
