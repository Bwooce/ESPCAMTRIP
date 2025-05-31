#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "esp_camera.h"

// Configuration namespace for all system settings
namespace Config {
  
  // WiFi Configuration
  struct WiFiConfig {
    const char* ssid = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";
    const uint32_t CONNECTION_TIMEOUT = 10000; // 10 seconds
    const uint32_t RECONNECT_DELAY = 5000;     // 5 seconds
    const uint8_t MAX_RECONNECT_ATTEMPTS = 5;
  };
  
  // AWS S3 Configuration
  struct S3Config {
    const char* access_key = "YOUR_AWS_ACCESS_KEY";
    const char* secret_key = "YOUR_AWS_SECRET_KEY";
    const char* region = "us-east-1";
    const char* bucket = "your-bucket-name";
    const size_t UPLOAD_BUFFER_SIZE = 4096;
    const size_t MAX_UPLOAD_RETRIES = 3;
  };
  
  // NTRIP Configuration
  struct NtripConfig {
    bool enabled = true;
    const char* server = "ntrip.data.gnss.ga.gov.au";
    const int port = 443;  // HTTPS port
    const char* mountpoint = "SYDN00AUS0";
    const char* username = "your_username";
    const char* password = "your_password";
    const char* gga_message = "$GPGGA,235959.000,3347.9167,S,15110.9333,E,1,12,1.0,42.5,M,6.6,M,,*65";
    const bool use_ssl = true;
    const uint32_t RTCM_TIMEOUT = 5000; // 5 seconds
    const uint32_t GGA_INTERVAL = 10000; // 10 seconds
  };
  
  // Pin Configuration
  struct PinConfig {
    const uint8_t CAPTURE_TRIGGER_PIN = 12;
    const uint8_t UPLOAD_TRIGGER_PIN = 13;
    const uint8_t LED_STATUS_PIN = 33;  // 2 for ESP32-C3
    const uint8_t RTCM_UART_TX = 3;
    const uint8_t RTCM_UART_RX = -1;  // Not used but defined
  };
  
  // Camera Pin Configuration
  struct CameraPins {
    const int8_t PWDN_GPIO_NUM = -1;
    const int8_t RESET_GPIO_NUM = -1;
    const uint8_t XCLK_GPIO_NUM = 10;
    const uint8_t SIOD_GPIO_NUM = 40;
    const uint8_t SIOC_GPIO_NUM = 39;
    const uint8_t Y9_GPIO_NUM = 48;
    const uint8_t Y8_GPIO_NUM = 11;
    const uint8_t Y7_GPIO_NUM = 12;
    const uint8_t Y6_GPIO_NUM = 14;
    const uint8_t Y5_GPIO_NUM = 16;
    const uint8_t Y4_GPIO_NUM = 18;
    const uint8_t Y3_GPIO_NUM = 17;
    const uint8_t Y2_GPIO_NUM = 15;
    const uint8_t VSYNC_GPIO_NUM = 38;
    const uint8_t HREF_GPIO_NUM = 47;
    const uint8_t PCLK_GPIO_NUM = 13;
  };
  
  // Timing Configuration
  struct TimingConfig {
    const uint32_t CAPTURE_INTERVAL = 500;    // 500ms between captures
    const uint32_t DEBOUNCE_DELAY = 50;       // 50ms button debounce
    const uint32_t WATCHDOG_TIMEOUT = 15000;  // 15 seconds
  };
  
  // Storage Configuration
  struct StorageConfig {
    const uint32_t MIN_FREE_SPACE_MB = 1024;  // 1GB minimum
    const uint32_t DIRECTORY_RETENTION_DAYS = 7;
    const char* UPLOAD_TRACKING_FILE = "/upload_status.txt";
    const char* CONFIG_FILE = "/config.json";
    const char* ERROR_LOG_FILE = "/error_log.txt";
  };
  
  // Power Management Configuration
  struct PowerConfig {
    const bool ENABLE_OPTIMIZATION = true;
    const bool SLEEP_BETWEEN_CAPTURES = true;
    const bool CAMERA_POWER_MANAGEMENT = true;
    const uint32_t IDLE_TIMEOUT_MS = 30000;      // 30 seconds
    const uint32_t DEEP_SLEEP_TIMEOUT_MS = 300000; // 5 minutes
    const uint32_t CPU_FREQ_CAPTURE = 240;       // MHz during capture
    const uint32_t CPU_FREQ_NORMAL = 160;        // MHz normal operation
    const uint32_t CPU_FREQ_IDLE = 80;           // MHz when idle
  };
  
  // Upload Configuration
  struct UploadConfig {
    const bool AUTO_UPLOAD = false;  // Auto upload every hour
    const bool DELETE_AFTER_UPLOAD = false; // Delete immediately after upload
  };
  
  // Camera Configuration
  struct CameraConfig {
    const uint8_t JPEG_QUALITY = 10;  // 0-63, lower is better quality
    const framesize_t FRAME_SIZE = FRAMESIZE_UXGA; // 1600x1200
    const uint32_t XCLK_FREQ = 20000000; // 20MHz
  };
  
  // Create instances
  extern WiFiConfig wifi;
  extern S3Config s3;
  extern NtripConfig ntrip;
  extern PinConfig pins;
  extern CameraPins cameraPins;
  extern TimingConfig timing;
  extern StorageConfig storage;
  extern PowerConfig power;
  extern UploadConfig upload;
  extern CameraConfig camera;
  
  // Configuration file functions
  bool loadFromFile();
  bool saveToFile();
  void printConfig();
}

#endif // CONFIG_H