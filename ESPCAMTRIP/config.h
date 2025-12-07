#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "esp_camera.h"

// Configuration namespace for all system settings
namespace Config {
  
  // WiFi Configuration
  struct WiFiConfig {
    String ssid;
    String password;
    const uint32_t CONNECTION_TIMEOUT = 10000; // 10 seconds
    const uint32_t RECONNECT_DELAY = 5000;     // 5 seconds
    const uint8_t MAX_RECONNECT_ATTEMPTS = 5;
  };
  
  // AWS S3 Configuration
  struct S3Config {
    String access_key; // Changed from const char*
    String secret_key; // Changed from const char*
    String region;
    String bucket;
    const size_t UPLOAD_BUFFER_SIZE = 4096;
    const size_t MAX_UPLOAD_RETRIES = 3;
  };
  
  // NTRIP Configuration
  struct NtripConfig {
    bool enabled = true;
    String server;
    int port = 443;  // HTTPS port
    String mountpoint;
    String username;
    String password;
    String gga_message;
    bool use_ssl = true;
    const uint32_t RTCM_TIMEOUT = 5000; // 5 seconds
    const uint32_t GGA_INTERVAL = 10000; // 10 seconds
  };
  
  // Pin Configuration (board-specific auto-detection)
  struct PinConfig {
    #if defined(ARDUINO_XIAO_ESP32S3)
      // XIAO ESP32S3 pin configuration
      const uint8_t CAPTURE_TRIGGER_PIN = 2; // GPIO2 (XIAO D0/IO2) - safe pin
      const uint8_t UPLOAD_TRIGGER_PIN = 4;  // GPIO4 (XIAO D2/IO4) - safe pin
      const uint8_t LED_STATUS_PIN = 21;     // GPIO21 (built-in LED)
      const uint8_t RTCM_UART_TX = 6;       // GPIO6 (XIAO D4/IO6) - safe pin
      const uint8_t RTCM_UART_RX = 7;       // GPIO7 (XIAO D5/IO7) - safe pin
    #elif defined(ARDUINO_ESP32S3_CAM_LCD)
      // ESP32-S3-CAM-LCD pin configuration
      const uint8_t CAPTURE_TRIGGER_PIN = 1; // GPIO1 - safe pin on ESP32-S3-CAM
      const uint8_t UPLOAD_TRIGGER_PIN = 2;  // GPIO2 - safe pin on ESP32-S3-CAM
      const uint8_t LED_STATUS_PIN = 33;     // GPIO33 - LED pin on ESP32-S3-CAM
      const uint8_t RTCM_UART_TX = 6;       // GPIO6 - UART TX
      const uint8_t RTCM_UART_RX = 7;       // GPIO7 - UART RX
    #else
      // Generic ESP32-S3 pin configuration (fallback)
      const uint8_t CAPTURE_TRIGGER_PIN = 1; // GPIO1 - safe pin
      const uint8_t UPLOAD_TRIGGER_PIN = 2;  // GPIO2 - safe pin
      const uint8_t LED_STATUS_PIN = 8;      // GPIO8 - generic LED pin
      const uint8_t RTCM_UART_TX = 6;       // GPIO6 - UART TX
      const uint8_t RTCM_UART_RX = 7;       // GPIO7 - UART RX
    #endif
  };
  
  // Camera Pin Configuration (board-specific auto-detection)
  struct CameraPins {
    #if defined(ARDUINO_XIAO_ESP32S3)
      // XIAO ESP32S3 Sense camera pins (OV2640)
      const int8_t PWDN_GPIO_NUM = -1;  // Not connected
      const int8_t RESET_GPIO_NUM = -1; // Not connected
      const uint8_t XCLK_GPIO_NUM = 10;  // Camera clock
      const uint8_t SIOD_GPIO_NUM = 40;  // I2C SDA (SCCB Data)
      const uint8_t SIOC_GPIO_NUM = 39;  // I2C SCL (SCCB Clock)
      const uint8_t Y9_GPIO_NUM = 48;    // Data line Y9
      const uint8_t Y8_GPIO_NUM = 11;    // Data line Y8
      const uint8_t Y7_GPIO_NUM = 12;    // Data line Y7
      const uint8_t Y6_GPIO_NUM = 14;    // Data line Y6
      const uint8_t Y5_GPIO_NUM = 16;    // Data line Y5
      const uint8_t Y4_GPIO_NUM = 18;    // Data line Y4
      const uint8_t Y3_GPIO_NUM = 17;    // Data line Y3
      const uint8_t Y2_GPIO_NUM = 15;    // Data line Y2
      const uint8_t VSYNC_GPIO_NUM = 38; // Vertical sync
      const uint8_t HREF_GPIO_NUM = 47;  // Horizontal reference
      const uint8_t PCLK_GPIO_NUM = 13;  // Pixel clock
    #elif defined(ARDUINO_ESP32S3_CAM_LCD)
      // ESP32-S3-CAM-LCD camera pins (typically OV2640/OV5640)
      const int8_t PWDN_GPIO_NUM = 32;   // Power down
      const int8_t RESET_GPIO_NUM = -1;  // Reset (may vary)
      const uint8_t XCLK_GPIO_NUM = 0;   // Camera clock
      const uint8_t SIOD_GPIO_NUM = 26;  // I2C SDA (SCCB Data)
      const uint8_t SIOC_GPIO_NUM = 27;  // I2C SCL (SCCB Clock)
      const uint8_t Y9_GPIO_NUM = 35;    // Data line Y9
      const uint8_t Y8_GPIO_NUM = 34;    // Data line Y8
      const uint8_t Y7_GPIO_NUM = 39;    // Data line Y7
      const uint8_t Y6_GPIO_NUM = 36;    // Data line Y6
      const uint8_t Y5_GPIO_NUM = 21;    // Data line Y5
      const uint8_t Y4_GPIO_NUM = 19;    // Data line Y4
      const uint8_t Y3_GPIO_NUM = 18;    // Data line Y3
      const uint8_t Y2_GPIO_NUM = 5;     // Data line Y2
      const uint8_t VSYNC_GPIO_NUM = 25; // Vertical sync
      const uint8_t HREF_GPIO_NUM = 23;  // Horizontal reference
      const uint8_t PCLK_GPIO_NUM = 22;  // Pixel clock
    #else
      // Generic ESP32-S3 camera pins (fallback - AI Thinker style)
      const int8_t PWDN_GPIO_NUM = 32;   // Power down
      const int8_t RESET_GPIO_NUM = -1;  // Reset
      const uint8_t XCLK_GPIO_NUM = 0;   // Camera clock
      const uint8_t SIOD_GPIO_NUM = 26;  // I2C SDA (SCCB Data)
      const uint8_t SIOC_GPIO_NUM = 27;  // I2C SCL (SCCB Clock)
      const uint8_t Y9_GPIO_NUM = 35;    // Data line Y9
      const uint8_t Y8_GPIO_NUM = 34;    // Data line Y8
      const uint8_t Y7_GPIO_NUM = 39;    // Data line Y7
      const uint8_t Y6_GPIO_NUM = 36;    // Data line Y6
      const uint8_t Y5_GPIO_NUM = 21;    // Data line Y5
      const uint8_t Y4_GPIO_NUM = 19;    // Data line Y4
      const uint8_t Y3_GPIO_NUM = 18;    // Data line Y3
      const uint8_t Y2_GPIO_NUM = 5;     // Data line Y2
      const uint8_t VSYNC_GPIO_NUM = 25; // Vertical sync
      const uint8_t HREF_GPIO_NUM = 23;  // Horizontal reference
      const uint8_t PCLK_GPIO_NUM = 22;  // Pixel clock
    #endif
  };
  
  // Timing Configuration
  struct TimingConfig {
    uint32_t CAPTURE_INTERVAL = 500;    // 500ms between captures
    const uint32_t DEBOUNCE_DELAY = 50;       // 50ms button debounce
    const uint32_t WATCHDOG_TIMEOUT = 15000;  // 15 seconds
  };
  
  // Storage Configuration
  struct StorageConfig {
    const uint32_t MIN_FREE_SPACE_MB = 1024;  // 1GB minimum
    uint32_t DIRECTORY_RETENTION_DAYS = 7;
    const char* UPLOAD_TRACKING_FILE = "/upload_status.txt";
    const char* CONFIG_FILE = "/config.json";
    const char* ERROR_LOG_FILE = "/error_log.txt";
  };
  
  // Power Management Configuration
  struct PowerConfig {
    bool ENABLE_OPTIMIZATION = true;
    bool SLEEP_BETWEEN_CAPTURES = true;
    bool CAMERA_POWER_MANAGEMENT = true;
    uint32_t IDLE_TIMEOUT_MS = 30000;      // 30 seconds
    uint32_t DEEP_SLEEP_TIMEOUT_MS = 300000; // 5 minutes
    uint32_t CPU_FREQ_CAPTURE = 240;       // MHz during capture
    uint32_t CPU_FREQ_NORMAL = 160;        // MHz normal operation
    uint32_t CPU_FREQ_IDLE = 80;           // MHz when idle
  };
  
  // Upload Configuration
  struct UploadConfig {
    bool AUTO_UPLOAD = false;  // Auto upload every hour
    bool DELETE_AFTER_UPLOAD = false; // Delete immediately after upload
  };
  
  // Camera Configuration
  struct CameraConfig {
    uint8_t JPEG_QUALITY = 10;  // 0-63, lower is better quality
    framesize_t FRAME_SIZE = FRAMESIZE_UXGA; // 1600x1200
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