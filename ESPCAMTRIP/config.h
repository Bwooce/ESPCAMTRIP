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
      // ESP32-S3-CAM-LCD: Camera pins are defined by board variant as macros
      // Board defines: XCLK=40, SIOD=17, SIOC=18, Y9=39, Y8=41, Y7=42, etc.
      // Pin macros are automatically available - no need to redefine them
      // The board profile provides the correct pin definitions for ESP32-S3-CAM
    #else
      // Xiao ESP32S3 Sense camera pins (OV2640 older/OV3660 newer revisions)
      const int8_t PWDN_GPIO_NUM = -1;   // Power down (not connected)
      const int8_t RESET_GPIO_NUM = -1;  // Reset (not connected)
      const uint8_t XCLK_GPIO_NUM = 10;  // Camera clock
      const uint8_t SIOD_GPIO_NUM = 40;  // I2C SDA (SCCB Data)
      const uint8_t SIOC_GPIO_NUM = 39;  // I2C SCL (SCCB Clock)
      const uint8_t Y9_GPIO_NUM = 48;    // Data line Y9 (MSB)
      const uint8_t Y8_GPIO_NUM = 11;    // Data line Y8
      const uint8_t Y7_GPIO_NUM = 12;    // Data line Y7
      const uint8_t Y6_GPIO_NUM = 14;    // Data line Y6
      const uint8_t Y5_GPIO_NUM = 16;    // Data line Y5
      const uint8_t Y4_GPIO_NUM = 18;    // Data line Y4
      const uint8_t Y3_GPIO_NUM = 17;    // Data line Y3
      const uint8_t Y2_GPIO_NUM = 15;    // Data line Y2 (LSB)
      const uint8_t VSYNC_GPIO_NUM = 38; // Vertical sync
      const uint8_t HREF_GPIO_NUM = 47;  // Horizontal reference
      const uint8_t PCLK_GPIO_NUM = 13;  // Pixel clock
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
    const uint32_t MIN_FREE_SPACE_MB = 512;   // 512MB minimum
    uint32_t DIRECTORY_RETENTION_DAYS = 7;
    const char* UPLOAD_TRACKING_FILE = "/upload_status.txt";
    const char* CONFIG_FILE = "/config.json";
    const char* ERROR_LOG_FILE = "/error_log.txt";
  };
  
  // Power Management Configuration
  struct PowerConfig {
    bool ENABLE_OPTIMIZATION = true;
    bool SLEEP_BETWEEN_CAPTURES = false;   // Disabled for PSRAM compatibility
    bool CAMERA_POWER_MANAGEMENT = true;
    uint32_t IDLE_TIMEOUT_MS = 30000;      // 30 seconds
    uint32_t DEEP_SLEEP_TIMEOUT_MS = 300000; // 5 minutes
    uint32_t CPU_FREQ_CAPTURE = 240;       // MHz during capture
    uint32_t CPU_FREQ_NORMAL = 160;        // MHz normal operation
    uint32_t CPU_FREQ_IDLE = 80;           // MHz when idle (PSRAM minimum)
  };
  
  // Upload Configuration
  struct UploadConfig {
    bool AUTO_UPLOAD = false;  // Auto upload every hour
    bool DELETE_AFTER_UPLOAD = false; // Delete immediately after upload
  };
  
  // Camera Configuration
  struct CameraConfig {
    uint8_t JPEG_QUALITY = 10;  // 0-63, lower is better quality
    framesize_t FRAME_SIZE = FRAMESIZE_SVGA; // 800x600 (start with stable resolution)
    framesize_t APRILTAG_FRAME_SIZE = FRAMESIZE_SVGA; // 800x600 for AprilTag detection (test stability first)
    const uint32_t XCLK_FREQ = 16000000; // 16MHz - enables EDMA mode on ESP32-S3 for better DMA efficiency

    // FPS Configuration - limits frame acquisition rate to reduce WiFi/camera conflicts
    uint8_t MAX_FPS = 2;                  // Maximum frames per second (configurable, max 2 FPS)
    uint32_t MIN_FRAME_INTERVAL_MS = 500; // Minimum milliseconds between frames (1000/MAX_FPS)

    // XIAO ESP32-S3 Sense Camera Calibration (OV2640 sensor)
    // Based on 1/4" sensor format: 3.6mm × 2.7mm active area
    const float FOCAL_LENGTH_MM = 2.8f;        // Approximate focal length in mm
    const float SENSOR_WIDTH_MM = 3.6f;        // Sensor width in mm (1/4" format)
    const float SENSOR_HEIGHT_MM = 2.7f;       // Sensor height in mm (1/4" format)

    // For SVGA 800x600 resolution
    const uint16_t IMAGE_WIDTH_SVGA = 800;     // Pixels
    const uint16_t IMAGE_HEIGHT_SVGA = 600;    // Pixels

    // For UXGA 1600x1200 resolution (maximum OV2640)
    const uint16_t IMAGE_WIDTH_UXGA = 1600;    // Pixels
    const uint16_t IMAGE_HEIGHT_UXGA = 1200;   // Pixels

    // Calculated focal length in pixels for different resolutions
    // focal_length_pixels = (focal_length_mm * image_width_pixels) / sensor_width_mm
    const float FOCAL_LENGTH_PIXELS_SVGA = (FOCAL_LENGTH_MM * IMAGE_WIDTH_SVGA) / SENSOR_WIDTH_MM;  // ≈ 622 pixels
    const float FOCAL_LENGTH_PIXELS_UXGA = (FOCAL_LENGTH_MM * IMAGE_WIDTH_UXGA) / SENSOR_WIDTH_MM;  // ≈ 1244 pixels

    // ============================================================================
    // APRILTAG CONFIGURATION
    // ============================================================================
    //
    // 🎯 TO CHANGE TAG: Modify CURRENT_TAG_SIZE and CURRENT_TAG_FAMILY below
    //
    // 📐 TAG SIZE Examples:
    //   CURRENT_TAG_SIZE = TAG_25MM_BLACK;    // For 25mm lab tags
    //   CURRENT_TAG_SIZE = TAG_100MM_BLACK;   // For 100mm standard robotics tags
    //   CURRENT_TAG_SIZE = TAG_300MM_BLACK;   // For 300mm drone landing tags
    //   CURRENT_TAG_SIZE = TAG_2400MM_BLACK;  // For 2.4m long-distance detection
    //
    // 🏷️  TAG FAMILY Examples:
    //   FAMILY_16H5:  Large features, 30 tags,  BEST for long distance (50m+)
    //   FAMILY_25H9:  Medium features, 35 tags, GOOD for medium distance (5-20m)
    //   FAMILY_36H11: Small features, 587 tags, BEST for close range (<5m)
    //
    // 🎯 RECOMMENDED COMBINATIONS:
    //   50m detection:    TAG_2400MM_BLACK + FAMILY_16H5  (large tag, large features)
    //   Drone landing:    TAG_300MM_BLACK  + FAMILY_25H9  (medium tag, medium features)
    //   Indoor robotics:  TAG_100MM_BLACK  + FAMILY_36H11 (small tag, many IDs)
    //   Close precision:  TAG_50MM_BLACK   + FAMILY_36H11 (small tag, high precision)
    //
    // 📏 MEASUREMENT METHOD:
    //   - Black square = just the data-carrying part (most common measurement)
    //   - Total tag = including white border (add ~25% to black square size)
    // ============================================================================

    // Choose measurement method:
    bool MEASURE_BLACK_SQUARE = true;          // true = measure black square only, false = measure total tag including border

    // AprilTag Family Configuration (🎯 CHANGE THIS LINE TO SWITCH TAG FAMILY):
    enum AprilTagFamily {
      FAMILY_16H5,         // 16×16 bits, 30 tags,  Large features - BEST for long distance
      FAMILY_25H9,         // 25×25 bits, 35 tags,  Medium features - GOOD for medium distance
      FAMILY_36H11,        // 36×36 bits, 587 tags, Small features - BEST for close range/many IDs
      FAMILY_STANDARD41H12, // 41×41 bits, 2115 tags, AprilTag 3 standard family - BEST for high ID count
      FAMILY_CIRCLE21H7,   // 21×21 circular, Hamming 7, AprilTag 3 - BEST for motion blur resistance
      FAMILY_CIRCLE49H12   // 49×49 circular, Hamming 12, AprilTag 3 - BEST for maximum robustness
    };

    // Multi-tag detection configuration
    bool ENABLE_MULTI_FAMILY_DETECTION = true;          // ✅ Enable for triple concentric Circle49h12 design
    AprilTagFamily CURRENT_TAG_FAMILY = FAMILY_CIRCLE21H7; // Circle21h7: Direct LUT with 3-bit error correction

    // AprilTag family descriptions
    const char* APRILTAG_FAMILY_NAMES[6] = {
      "Tag16h5 (30 tags, large features)",
      "Tag25h9 (35 tags, medium features)",
      "Tag36h11 (587 tags, small features)",
      "TagStandard41h12 (2115 tags, AprilTag 3)",
      "TagCircle21h7 (2115 tags, circular, Hamming 7)",
      "TagCircle49h12 (2115 tags, circular, Hamming 12)"
    };

    const char* APRILTAG_FAMILY_RECOMMENDATIONS[6] = {
      "Best for: Long distance (50m+), single targets",
      "Best for: Medium distance (5-20m), small swarms",
      "Best for: Close range (<5m), large swarms, high precision",
      "Best for: High ID count applications, AprilTag 3 standard",
      "Best for: Motion blur resistance, fast movement, symmetry",
      "Best for: Maximum robustness, extreme conditions, long range"
    };

    // Current tag size selection (🎯 CHANGE THIS LINE TO SWITCH TAG SIZES):
    enum AprilTagSizePreset {
      TAG_25MM_BLACK,      // 25mm black square (~31mm total) - Lab/desktop use
      TAG_50MM_BLACK,      // 50mm black square (~63mm total) - Indoor robotics
      TAG_75MM_BLACK,      // 75mm black square (~94mm total) - Small drone/robot
      TAG_95MM_BLACK,      // 95mm black square (~119mm total) - Previous tag
      TAG_100MM_BLACK,     // 100mm black square (~125mm total) - Standard robotics
      TAG_145MM_BLACK,     // 145mm black square (~181mm total) - Current tag (250mm total)
      TAG_150MM_BLACK,     // 150mm black square (~188mm total) - Medium outdoor
      TAG_200MM_BLACK,     // 200mm black square (~250mm total) - Large outdoor
      TAG_300MM_BLACK,     // 300mm black square (~375mm total) - Drone landing
      TAG_500MM_BLACK,     // 500mm black square (~625mm total) - Long distance
      TAG_1000MM_BLACK,    // 1000mm black square (~1250mm total) - Very long distance
      TAG_2400MM_BLACK,    // 2400mm black square (~3000mm total) - 50m detection
      TAG_CUSTOM           // Custom size (set APRILTAG_SIZE_MM manually)
    };

    AprilTagSizePreset CURRENT_TAG_SIZE = TAG_145MM_BLACK;  // Current 145mm black square tag

    // AprilTag size lookup table (black square sizes in mm)
    const float APRILTAG_SIZES_MM[13] = {
      25.0f,    // TAG_25MM_BLACK
      50.0f,    // TAG_50MM_BLACK
      75.0f,    // TAG_75MM_BLACK
      95.0f,    // TAG_95MM_BLACK (previous tag)
      100.0f,   // TAG_100MM_BLACK
      145.0f,   // TAG_145MM_BLACK (current tag - 145mm black square, 250mm total)
      150.0f,   // TAG_150MM_BLACK
      200.0f,   // TAG_200MM_BLACK
      300.0f,   // TAG_300MM_BLACK
      500.0f,   // TAG_500MM_BLACK
      1000.0f,  // TAG_1000MM_BLACK
      2400.0f,  // TAG_2400MM_BLACK (for 50m detection)
      100.0f    // TAG_CUSTOM (placeholder, override in code)
    };

    // AprilTag size descriptions
    const char* APRILTAG_DESCRIPTIONS[13] = {
      "25mm - Lab/Desktop",
      "50mm - Indoor Robotics",
      "75mm - Small Drone/Robot",
      "95mm - Previous Tag",
      "100mm - Standard Robotics",
      "145mm - Current Tag (250mm total)",
      "150mm - Medium Outdoor",
      "200mm - Large Outdoor",
      "300mm - Drone Landing",
      "500mm - Long Distance",
      "1000mm - Very Long Distance",
      "2400mm - 50m Detection",
      "Custom - Manual Setting"
    };

    // Get current AprilTag size based on selection
    float getAprilTagSize() const {
      if (CURRENT_TAG_SIZE == TAG_CUSTOM) {
        return APRILTAG_SIZE_MM_CUSTOM;  // Use custom value
      }
      return APRILTAG_SIZES_MM[CURRENT_TAG_SIZE];
    }

    // Convert black square to total tag size (add ~25% border)
    float getAprilTagTotalSize() const {
      float black_square = getAprilTagSize();
      return MEASURE_BLACK_SQUARE ? black_square * 1.25f : black_square;
    }

    // Custom size override (only used when CURRENT_TAG_SIZE = TAG_CUSTOM)
    float APRILTAG_SIZE_MM_CUSTOM = 95.0f;     // Custom tag size in mm

    // ============================================================================
    // TRIPLE CONCENTRIC CIRCLE49H12 DESIGN CONFIGURATION
    // ============================================================================
    //
    // 🎯 TRIPLE CONCENTRIC CIRCLE DESIGN: All Circle49h12 family for memory efficiency
    //   Outer ring:  Circle49h12 (200mm black) ID 0 - Long distance acquisition (50m+)
    //   Middle ring: Circle49h12 (85mm black)  ID 1 - Approach phase (5-25m)
    //   Inner core:  Circle49h12 (26mm black)  ID 2 - Precision landing (0.5-8m)
    //
    // 📏 TOTAL PHYSICAL SIZE: ~250mm × 250mm (compact, manageable)
    //   - Single family compilation saves ~300KB memory
    //   - Circular design: Superior blur resistance and symmetric detection
    //   - Hamming distance 12: Maximum error correction for challenging conditions
    //   - Different IDs enable automatic phase detection during approach
    // ============================================================================

    struct ConcentricTagConfig {
      // Outer ring: Circle21h7 with LUT-based 3-bit error correction
      float OUTER_TAG_SIZE_MM = 200.0f;          // 200mm tag, ID 0 (partially corrupted by inner)
      AprilTagFamily OUTER_TAG_FAMILY = FAMILY_CIRCLE21H7;
      int OUTER_TAG_ID = 0;

      // Inner ring: Circle21h7 for close approach
      float INNER_TAG_SIZE_MM = 42.0f;           // 42mm tag, ID 1 (corrupts outer, fully visible)
      AprilTagFamily INNER_TAG_FAMILY = FAMILY_CIRCLE21H7;
      int INNER_TAG_ID = 1;

      // Distance ranges for detection (7m down to nearly 0m coverage)
      float OUTER_TAG_MAX_DISTANCE_M = 7.0f;     // 200mm visible from ~7m
      float INNER_TAG_MAX_DISTANCE_M = 2.0f;     // 42mm visible from ~2m
      float MIN_DETECTION_DISTANCE_M = 0.1f;     // Minimum useful detection range

      // Dual concentric configuration
      bool ENABLE_AUTO_FAMILY_SELECTION = false; // Single family (Circle21h7)
      bool USE_ID_FOR_RING_DETECTION = true;     // Use tag ID: 0=outer, 1=inner
    };

    ConcentricTagConfig CONCENTRIC_CONFIG;
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