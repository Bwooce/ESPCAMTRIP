#include "config.h"
#include "storage_manager.h"
#include <ArduinoJson.h>

namespace Config {
  // Create instances
  WiFiConfig wifi = {
    "YOUR_WIFI_SSID",
    "YOUR_WIFI_PASSWORD",
    10000, // CONNECTION_TIMEOUT
    5000,  // RECONNECT_DELAY
    5      // MAX_RECONNECT_ATTEMPTS
  };
  S3Config s3 = {
    "YOUR_AWS_ACCESS_KEY", // Now a String
    "YOUR_AWS_SECRET_KEY", // Now a String
    "us-east-1",
    "your-bucket-name",
    4096, // UPLOAD_BUFFER_SIZE
    3     // MAX_UPLOAD_RETRIES
  };
  NtripConfig ntrip = {
    true, // enabled
    "ntrip.data.gnss.ga.gov.au", // server
    443,   // port
    "SYDN00AUS0", // mountpoint
    "your_username", // username
    "your_password", // password
    "$GPGGA,235959.000,3347.9167,S,15110.9333,E,1,12,1.0,42.5,M,6.6,M,,*65", // gga_message
    true,  // use_ssl
    5000,  // RTCM_TIMEOUT
    10000  // GGA_INTERVAL
  };
  GPSConfig gps;
  MAVLinkConfig mavlink;
  PinConfig pins;
  CameraPins cameraPins;
  TimingConfig timing;
  StorageConfig storage;
  PowerConfig power;
  UploadConfig upload;
  CameraConfig camera;
  CameraModeConfig cameraMode;
  AprilTagConfig apriltag;

  bool loadFromFile() {
    File configFile = StorageManager::openFile(storage.CONFIG_FILE, "r");
    if (!configFile) {
      Serial.println("No config file found, using defaults");
      return false;
    }

    // Read the file
    String jsonStr = "";
    while (configFile.available()) {
      jsonStr += (char)configFile.read();
    }
    StorageManager::closeFile(configFile);

    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonStr);

    if (error) {
      Serial.print("Config parsing failed: ");
      Serial.println(error.c_str());
      return false;
    }

    // Load WiFi settings
    if (!doc["wifi"].isNull()) {
      JsonObject wifiObj = doc["wifi"].as<JsonObject>();
      if (!wifiObj["ssid"].isNull()) {
        wifi.ssid = wifiObj["ssid"].as<String>();
      }
      if (!wifiObj["password"].isNull()) {
        wifi.password = wifiObj["password"].as<String>();
      }
    }

    // Load S3 settings
    if (!doc["s3"].isNull()) {
      JsonObject s3Obj = doc["s3"].as<JsonObject>();
      if (!s3Obj["access_key"].isNull()) {
        s3.access_key = s3Obj["access_key"].as<String>();
      }
      if (!s3Obj["secret_key"].isNull()) {
        s3.secret_key = s3Obj["secret_key"].as<String>();
      }
      if (!s3Obj["region"].isNull()) {
        s3.region = s3Obj["region"].as<String>();
      }
      if (!s3Obj["bucket"].isNull()) {
        s3.bucket = s3Obj["bucket"].as<String>();
      }
    }

    // Load NTRIP settings
    if (!doc["ntrip"].isNull()) {
      JsonObject ntripObj = doc["ntrip"].as<JsonObject>();
      if (!ntripObj["enabled"].isNull()) {
        ntrip.enabled = ntripObj["enabled"].as<bool>();
      }
      if (!ntripObj["server"].isNull()) {
        ntrip.server = ntripObj["server"].as<String>();
      }
      if (!ntripObj["port"].isNull()) {
        ntrip.port = ntripObj["port"].as<int>();
      }
      if (!ntripObj["mountpoint"].isNull()) {
        ntrip.mountpoint = ntripObj["mountpoint"].as<String>();
      }
      if (!ntripObj["username"].isNull()) {
        ntrip.username = ntripObj["username"].as<String>();
      }
      if (!ntripObj["password"].isNull()) {
        ntrip.password = ntripObj["password"].as<String>();
      }
      if (!ntripObj["gga_message"].isNull()) {
        ntrip.gga_message = ntripObj["gga_message"].as<String>();
      }
      if (!ntripObj["use_ssl"].isNull()) {
        ntrip.use_ssl = ntripObj["use_ssl"].as<bool>();
      }
    }

    // Load TimingConfig settings
    if (!doc["timing"].isNull()) {
        JsonObject timingObj = doc["timing"].as<JsonObject>();
        if (!timingObj["CAPTURE_INTERVAL"].isNull()) {
            timing.CAPTURE_INTERVAL = timingObj["CAPTURE_INTERVAL"].as<uint32_t>();
        }
    }

    // Load StorageConfig settings
    if (!doc["storage"].isNull()) {
        JsonObject storageObj = doc["storage"].as<JsonObject>();
        if (!storageObj["DIRECTORY_RETENTION_DAYS"].isNull()) {
            storage.DIRECTORY_RETENTION_DAYS = storageObj["DIRECTORY_RETENTION_DAYS"].as<uint32_t>();
        }
    }

    // Load PowerConfig settings
    if (!doc["power"].isNull()) {
        JsonObject powerObj = doc["power"].as<JsonObject>();
        if (!powerObj["ENABLE_OPTIMIZATION"].isNull()) {
            power.ENABLE_OPTIMIZATION = powerObj["ENABLE_OPTIMIZATION"].as<bool>();
        }
        if (!powerObj["SLEEP_BETWEEN_CAPTURES"].isNull()) {
            power.SLEEP_BETWEEN_CAPTURES = powerObj["SLEEP_BETWEEN_CAPTURES"].as<bool>();
        }
        if (!powerObj["CAMERA_POWER_MANAGEMENT"].isNull()) {
            power.CAMERA_POWER_MANAGEMENT = powerObj["CAMERA_POWER_MANAGEMENT"].as<bool>();
        }
        if (!powerObj["IDLE_TIMEOUT_MS"].isNull()) {
            power.IDLE_TIMEOUT_MS = powerObj["IDLE_TIMEOUT_MS"].as<uint32_t>();
        }
        if (!powerObj["DEEP_SLEEP_TIMEOUT_MS"].isNull()) {
            power.DEEP_SLEEP_TIMEOUT_MS = powerObj["DEEP_SLEEP_TIMEOUT_MS"].as<uint32_t>();
        }
        if (!powerObj["CPU_FREQ_CAPTURE"].isNull()) {
            power.CPU_FREQ_CAPTURE = powerObj["CPU_FREQ_CAPTURE"].as<uint32_t>();
        }
        if (!powerObj["CPU_FREQ_NORMAL"].isNull()) {
            power.CPU_FREQ_NORMAL = powerObj["CPU_FREQ_NORMAL"].as<uint32_t>();
        }
        if (!powerObj["CPU_FREQ_IDLE"].isNull()) {
            power.CPU_FREQ_IDLE = powerObj["CPU_FREQ_IDLE"].as<uint32_t>();
        }
    }

    // Load UploadConfig settings
    if (!doc["upload"].isNull()) {
        JsonObject uploadObj = doc["upload"].as<JsonObject>();
        if (!uploadObj["AUTO_UPLOAD"].isNull()) {
            upload.AUTO_UPLOAD = uploadObj["AUTO_UPLOAD"].as<bool>();
        }
        if (!uploadObj["DELETE_AFTER_UPLOAD"].isNull()) {
            upload.DELETE_AFTER_UPLOAD = uploadObj["DELETE_AFTER_UPLOAD"].as<bool>();
        }
    }

    // Load CameraConfig settings
    if (!doc["camera"].isNull()) {
        JsonObject cameraObj = doc["camera"].as<JsonObject>();
        if (!cameraObj["JPEG_QUALITY"].isNull()) {
            camera.JPEG_QUALITY = cameraObj["JPEG_QUALITY"].as<uint8_t>();
        }
        if (!cameraObj["FRAME_SIZE"].isNull()) {
            camera.FRAME_SIZE = (framesize_t)cameraObj["FRAME_SIZE"].as<int>();
        }
    }

    Serial.println("Configuration loaded from file");
    return true;
  }

  bool saveToFile() {
    JsonDocument doc;

    JsonObject wifiObj = doc["wifi"].to<JsonObject>();
    wifiObj["ssid"] = wifi.ssid;

    JsonObject s3Obj = doc["s3"].to<JsonObject>();
    s3Obj["region"] = s3.region;
    s3Obj["bucket"] = s3.bucket;

    JsonObject ntripObj = doc["ntrip"].to<JsonObject>();
    ntripObj["enabled"] = ntrip.enabled;
    ntripObj["server"] = ntrip.server;
    ntripObj["port"] = ntrip.port;
    ntripObj["mountpoint"] = ntrip.mountpoint;

    JsonObject uploadObj = doc["upload"].to<JsonObject>();
    uploadObj["auto_upload"] = upload.AUTO_UPLOAD;
    uploadObj["delete_after_upload"] = upload.DELETE_AFTER_UPLOAD;

    JsonObject powerObj = doc["power"].to<JsonObject>();
    powerObj["enable_optimization"] = power.ENABLE_OPTIMIZATION;
    powerObj["camera_power_management"] = power.CAMERA_POWER_MANAGEMENT;

    File configFile = StorageManager::openFile(storage.CONFIG_FILE, "w");
    if (!configFile) {
      Serial.println("Failed to create config file");
      return false;
    }

    serializeJsonPretty(doc, configFile);
    StorageManager::closeFile(configFile);

    Serial.println("Configuration saved to file");
    return true;
  }

  void printConfig() {
    Serial.println("\n=== Current Configuration ===");

    Serial.println("\n[WiFi]");
    Serial.printf("SSID: %s\n", wifi.ssid.c_str());
    Serial.printf("Connection Timeout: %lu ms\n", wifi.CONNECTION_TIMEOUT);

    Serial.println("\n[S3]");
    // Note: Printing keys directly is a security risk in production.
    // For debugging, this is fine. Consider printing "[set]" or partial keys.
    Serial.printf("Access Key: %s\n", s3.access_key.c_str());
    Serial.printf("Secret Key: %s\n", s3.secret_key.c_str()); // Mask this in production
    Serial.printf("Region: %s\n", s3.region.c_str());
    Serial.printf("Bucket: %s\n", s3.bucket.c_str());
    Serial.printf("Max Upload Retries: %zu\n", s3.MAX_UPLOAD_RETRIES);

    Serial.println("\n[NTRIP]");
    Serial.printf("Enabled: %s\n", ntrip.enabled ? "Yes" : "No");
    Serial.printf("Server: %s:%d\n", ntrip.server.c_str(), ntrip.port);
    Serial.printf("Mountpoint: %s\n", ntrip.mountpoint.c_str());
    Serial.printf("SSL: %s\n", ntrip.use_ssl ? "Yes" : "No");

    Serial.println("\n[Storage]");
    Serial.printf("Min Free Space: %u MB\n", storage.MIN_FREE_SPACE_MB); // uint32_t
    Serial.printf("Retention Days: %u\n", storage.DIRECTORY_RETENTION_DAYS); // uint32_t

    Serial.println("\n[Power]");
    Serial.printf("Optimization: %s\n", power.ENABLE_OPTIMIZATION ? "Enabled" : "Disabled");
    Serial.printf("Camera Power Mgmt: %s\n", power.CAMERA_POWER_MANAGEMENT ? "Enabled" : "Disabled");
    Serial.printf("CPU Frequencies: %u/%u/%u MHz\n", // uint32_t
                  power.CPU_FREQ_CAPTURE, power.CPU_FREQ_NORMAL, power.CPU_FREQ_IDLE);

    Serial.println("\n[Camera]");
    Serial.printf("JPEG Quality: %u\n", camera.JPEG_QUALITY); // uint8_t, %u fine
    Serial.printf("Frame Size: %s\n",
                  camera.FRAME_SIZE == FRAMESIZE_UXGA ? "UXGA (1600x1200)" : "Other");

    Serial.println("=============================\n");
  }
}