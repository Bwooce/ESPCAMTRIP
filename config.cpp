#include "config.h"
#include "storage_manager.h"
#include <ArduinoJson.h>

namespace Config {
  // Create instances
  WiFiConfig wifi;
  S3Config s3;
  NtripConfig ntrip;
  PinConfig pins;
  CameraPins cameraPins;
  TimingConfig timing;
  StorageConfig storage;
  PowerConfig power;
  UploadConfig upload;
  CameraConfig camera;
  
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
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
    
    if (error) {
      Serial.print("Config parsing failed: ");
      Serial.println(error.c_str());
      return false;
    }
    
    // Load WiFi settings
    if (doc.containsKey("wifi")) {
      JsonObject wifiObj = doc["wifi"];
      // Note: Can't modify const members, would need non-const versions
      // This is just an example of how it would work
    }
    
    // Load S3 settings
    if (doc.containsKey("s3")) {
      JsonObject s3Obj = doc["s3"];
      // Load S3 configuration
    }
    
    // Load NTRIP settings
    if (doc.containsKey("ntrip")) {
      JsonObject ntripObj = doc["ntrip"];
      if (ntripObj.containsKey("enabled")) {
        ntrip.enabled = ntripObj["enabled"];
      }
    }
    
    Serial.println("Configuration loaded from file");
    return true;
  }
  
  bool saveToFile() {
    StaticJsonDocument<2048> doc;
    
    // Save WiFi settings
    JsonObject wifiObj = doc.createNestedObject("wifi");
    wifiObj["ssid"] = wifi.ssid;
    // Don't save password for security
    
    // Save S3 settings (without credentials)
    JsonObject s3Obj = doc.createNestedObject("s3");
    s3Obj["region"] = s3.region;
    s3Obj["bucket"] = s3.bucket;
    
    // Save NTRIP settings
    JsonObject ntripObj = doc.createNestedObject("ntrip");
    ntripObj["enabled"] = ntrip.enabled;
    ntripObj["server"] = ntrip.server;
    ntripObj["port"] = ntrip.port;
    ntripObj["mountpoint"] = ntrip.mountpoint;
    
    // Save upload settings
    JsonObject uploadObj = doc.createNestedObject("upload");
    uploadObj["auto_upload"] = upload.AUTO_UPLOAD;
    uploadObj["delete_after_upload"] = upload.DELETE_AFTER_UPLOAD;
    
    // Save power settings
    JsonObject powerObj = doc.createNestedObject("power");
    powerObj["enable_optimization"] = power.ENABLE_OPTIMIZATION;
    powerObj["camera_power_management"] = power.CAMERA_POWER_MANAGEMENT;
    
    // Serialize to file
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
    Serial.printf("SSID: %s\n", wifi.ssid);
    Serial.printf("Connection Timeout: %d ms\n", wifi.CONNECTION_TIMEOUT);
    
    Serial.println("\n[S3]");
    Serial.printf("Region: %s\n", s3.region);
    Serial.printf("Bucket: %s\n", s3.bucket);
    Serial.printf("Max Upload Retries: %d\n", s3.MAX_UPLOAD_RETRIES);
    
    Serial.println("\n[NTRIP]");
    Serial.printf("Enabled: %s\n", ntrip.enabled ? "Yes" : "No");
    Serial.printf("Server: %s:%d\n", ntrip.server, ntrip.port);
    Serial.printf("Mountpoint: %s\n", ntrip.mountpoint);
    Serial.printf("SSL: %s\n", ntrip.use_ssl ? "Yes" : "No");
    
    Serial.println("\n[Storage]");
    Serial.printf("Min Free Space: %d MB\n", storage.MIN_FREE_SPACE_MB);
    Serial.printf("Retention Days: %d\n", storage.DIRECTORY_RETENTION_DAYS);
    
    Serial.println("\n[Power]");
    Serial.printf("Optimization: %s\n", power.ENABLE_OPTIMIZATION ? "Enabled" : "Disabled");
    Serial.printf("Camera Power Mgmt: %s\n", power.CAMERA_POWER_MANAGEMENT ? "Enabled" : "Disabled");
    Serial.printf("CPU Frequencies: %d/%d/%d MHz\n", 
                  power.CPU_FREQ_CAPTURE, power.CPU_FREQ_NORMAL, power.CPU_FREQ_IDLE);
    
    Serial.println("\n[Camera]");
    Serial.printf("JPEG Quality: %d\n", camera.JPEG_QUALITY);
    Serial.printf("Frame Size: %s\n", 
                  camera.FRAME_SIZE == FRAMESIZE_UXGA ? "UXGA (1600x1200)" : "Other");
    
    Serial.println("=============================\n");
  }
}