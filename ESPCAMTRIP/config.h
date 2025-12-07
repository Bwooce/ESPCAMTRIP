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
  
  // GPS Configuration
  struct GPSConfig {
    bool enabled = true;
    const uint32_t GPS_UPDATE_RATE = 5;    // Hz - GPS position update rate
    const uint32_t GPS_TIMEOUT = 5000;     // 5 seconds - GPS data timeout
    const float MIN_HDOP = 2.0;           // Minimum HDOP for valid fix
    const uint8_t MIN_SATELLITES = 4;      // Minimum satellites for valid fix
    bool enable_geotagging = true;         // Add GPS data to photos
    bool debug_output = false;             // Enable GPS debug messages
  };

  // MAVLink Configuration
  struct MAVLinkConfig {
    bool enabled = true;                   // Enable MAVLink output
    const uint32_t BAUD_RATE = 57600;      // Standard MAVLink baud rate
    const uint8_t SYSTEM_ID = 1;           // MAVLink system ID
    const uint8_t COMPONENT_ID = 191;      // MAV_COMP_ID_CAMERA
    const uint8_t TARGET_SYSTEM = 1;       // Target system ID (flight controller)
    const uint8_t TARGET_COMPONENT = 1;    // Target component ID
    const uint32_t HEARTBEAT_INTERVAL = 1000; // Heartbeat interval (ms)
    bool send_landing_target = true;       // Send LANDING_TARGET messages
    bool send_rtcm_data = true;           // Send RTCM corrections via MAVLink
  };

  // Pin Configuration
  struct PinConfig {
    const uint8_t CAPTURE_TRIGGER_PIN = 2; // Was 1, changed to GPIO2 (XIAO D0/IO2)
    const uint8_t UPLOAD_TRIGGER_PIN = 4;  // Was 2, changed to GPIO4 (XIAO D2/IO4)
    const uint8_t LED_STATUS_PIN = 21;     // Was 33, changed to GPIO21 (built-in LED)
    const uint8_t RTCM_UART_TX = 6;        // RTCM corrections to F9P
    const uint8_t GPS_UART_RX = 7;         // GPS data from F9P (was RTCM_UART_RX)
    const uint8_t MAVLINK_UART_TX = 8;     // MAVLink data to ArduPilot
  };
  
  // Camera Pin Configuration
  struct CameraPins {
    const int8_t PWDN_GPIO_NUM = -1;
    const int8_t RESET_GPIO_NUM = -1;
    const uint8_t XCLK_GPIO_NUM = 10;
    const uint8_t SIOD_GPIO_NUM = 40; // D(-)=40, D(+)=39 on schematic, but seems this is for I2C. Camera DVP uses other pins.
                                      // Standard XIAO ESP32S3 Sense Camera pins are:
                                      // XCLK: 10
                                      // PCLK: 13
                                      // VSYNC: 38
                                      // HREF: 47
                                      // D0/Y2: 15
                                      // D1/Y3: 17
                                      // D2/Y4: 18
                                      // D3/Y5: 16
                                      // D4/Y6: 14
                                      // D5/Y7: 12
                                      // D6/Y8: 11
                                      // D7/Y9: 48
                                      // SIOC: 39 (SCCB Clock)
                                      // SIOD: 40 (SCCB Data)
                                      // PWDN: -1 (not connected on XIAO schematic for camera itself)
                                      // RESET: -1
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

  // Camera Mode Configuration
  enum CameraMode {
    CAMERA_MODE_MISSION,    // High-res photo capture for mapping/inspection
    CAMERA_MODE_LANDING,    // Real-time AprilTag detection for precision landing
    CAMERA_MODE_IDLE
  };

  struct CameraModeConfig {
    // Mission Mode (Gutter Scanning)
    framesize_t MISSION_FRAME_SIZE = FRAMESIZE_UXGA;      // 1600x1200
    pixformat_t MISSION_PIXEL_FORMAT = PIXFORMAT_JPEG;
    uint32_t MISSION_CAPTURE_INTERVAL = 200;             // 5Hz
    uint8_t MISSION_JPEG_QUALITY = 10;                   // High quality

    // Landing Mode (AprilTag Detection)
    framesize_t LANDING_FRAME_SIZE = FRAMESIZE_VGA;       // 640x480
    pixformat_t LANDING_PIXEL_FORMAT = PIXFORMAT_GRAYSCALE;
    uint32_t LANDING_CAPTURE_INTERVAL = 100;             // 10Hz
    uint8_t LANDING_JPEG_QUALITY = 15;                   // N/A for grayscale

    // Mode switching
    CameraMode current_mode = CAMERA_MODE_MISSION;
    bool auto_switch_enabled = true;
  };

  // AprilTag Configuration
  struct AprilTagConfig {
    bool enabled = false;                    // Enable AprilTag detection
    const char* family = "tag36h11";         // AprilTag family
    float quad_decimate = 2.0;               // Processing speed vs accuracy
    int nthreads = 1;                        // Processing threads
    float decision_margin = 10.0;            // Detection threshold

    // Camera calibration for angle calculation
    float focal_length_px = 500.0;          // Calibrated focal length
    float cx = 320.0;                       // Image center X (VGA/2)
    float cy = 240.0;                       // Image center Y (VGA/2)

    // MAVLink output configuration
    uint8_t mavlink_system_id = 1;
    uint8_t mavlink_component_id = 196;     // MAV_COMP_ID_VISUAL_INERTIAL_ODOMETRY
    uint16_t mavlink_baud_rate = 115200;

    // Performance settings
    uint8_t target_detection_rate = 10;     // Hz
    float max_detection_distance = 5.0;     // Meters
  };

  // Create instances
  extern WiFiConfig wifi;
  extern S3Config s3;
  extern NtripConfig ntrip;
  extern GPSConfig gps;
  extern MAVLinkConfig mavlink;
  extern PinConfig pins;
  extern CameraPins cameraPins;
  extern TimingConfig timing;
  extern StorageConfig storage;
  extern PowerConfig power;
  extern UploadConfig upload;
  extern CameraConfig camera;
  extern CameraModeConfig cameraMode;
  extern AprilTagConfig apriltag;
  
  // Configuration file functions
  bool loadFromFile();
  bool saveToFile();
  void printConfig();
}

#endif // CONFIG_H