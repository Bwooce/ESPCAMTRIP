/*
 * ESP32-S3-CAM Main Application
 * Combines photo capture with S3 upload and NTRIP RTK corrections
 *
 * This is the main application file that coordinates all subsystems
 */

// ============================================================================
// COMPILE-TIME CONFIGURATION FLAGS
// ============================================================================

// RTCM Output Mode: Choose ONE of the following output formats
//
// RTCM_OUTPUT_MAVLINK (default)
//   - Wraps RTCM3 messages in MAVLink GPS_RTCM_DATA packets
//   - Use for: ArduPilot, PX4, or other MAVLink-based flight controllers
//   - The receiver must understand MAVLink protocol
//
// RTCM_OUTPUT_RAW
//   - Sends raw RTCM3 binary data directly to UART
//   - Use for: Direct connection to u-blox ZED-F9P, NEO-M8P, or similar
//   - Wire: ESP32 TX (GPIO6) -> GPS RX, ESP32 RX (GPIO7) <- GPS TX (optional)
//   - Default baud: 115200 (configure GPS receiver to match, or change below)
//
#define RTCM_OUTPUT_MAVLINK  // Default: MAVLink wrapped output
// #define RTCM_OUTPUT_RAW   // Uncomment for direct GPS receiver connection

// NTRIP Atlas automatic service discovery
#define NTRIP_ATLAS_ENABLED  // Enable automatic fallback service discovery

// Visual Code Recognition Test Mode
#define VISUAL_CODE_TEST_MODE  // Enable AprilTag recognition testing

// Automatic testing mode - no buttons required
#define AUTO_APRILTAG_TEST_MODE  // Enable automatic capture for AprilTag testing

// Raw RTCM baud rate (only used when RTCM_OUTPUT_RAW is defined)
// ZED-F9P defaults to 38400 on UART1/UART2 - change to match your receiver
// or configure your receiver to 115200 via u-center
#define RTCM_RAW_BAUD_RATE 115200

// ============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
#include <math.h>

#include "config.h"
#include "camera_manager.h"
#include "storage_manager.h"
#include "wifi_manager.h"
#include "upload_manager.h"
#include "ntrip_client.h"
#include "power_manager.h"
#include "system_state.h"

#ifdef VISUAL_CODE_TEST_MODE
#include "apriltag_detection.h"
#include "apriltag.h"
#include "tagCircle21h7.h"    // 🎯 ONLY Circle21h7 family - used for all 3 ring sizes
#include "common/image_u8.h"
#include "common/zarray.h"
#endif
#include "esp_camera.h"

// Testing if typedef works BEFORE the inline function
typedef int SimpleTest;

// Helper function to convert bytes to MiB with 1 decimal place
inline float bytesToMiB(uint32_t bytes) {
  return bytes / 1048576.0f;  // 1 MiB = 1024 * 1024 bytes
}

SimpleTest simpleTestFunction() { return 0; }

// Smart detection result structure for concentric tag design (ALWAYS AVAILABLE)
struct TagDetectionResult {
  float tag_size_mm;              // Physical tag size used for calculation
  float distance_mm;              // Calculated distance to tag
  const char* detected_family;    // Which family was detected
  const char* detection_phase;    // Mission phase
  bool is_concentric_design;      // True if using concentric multi-family design
};

// Test using full struct syntax
struct TagDetectionResult testFunction() {
  struct TagDetectionResult test = {};
  return test;
}

// Task handles
TaskHandle_t cameraTaskHandle = NULL;
TaskHandle_t uploadTaskHandle = NULL;
TaskHandle_t ntripTaskHandle = NULL;

// SPI Conflict Prevention: Global mutex to prevent concurrent SD/camera operations
// The Xiao ESP32S3 Sense has hardware limitations with simultaneous SD + camera use
SemaphoreHandle_t spiMutex = nullptr;

// Button debouncing
unsigned long lastDebounceTime = 0;
bool lastCaptureState = HIGH;
bool lastUploadState = HIGH;

// Function declarations (ALWAYS AVAILABLE)
// TagDetectionResult calculateSmartAprilTagDistance - defined below, no forward declaration needed
float calculateAprilTagDistance(float tag_size_pixels);                      // Backward compatibility
float calculateAprilTagAngle(float center_x, float center_y);              // Angle calculation
// void cameraTask(void* parameter); // Disabled for AprilTag main task mode
void uploadTask(void* parameter);
void handleButtons();
#ifdef AUTO_APRILTAG_TEST_MODE
void handleAutomaticTesting();
#endif
void initializeSystem();
void performHealthCheck();


// Distance calculation functions (ALWAYS AVAILABLE - core functionality)
typedef struct { float test; } TestType; // Simple test typedef
struct TagDetectionResult calculateSmartAprilTagDistance(float tag_size_pixels, int detected_id) {
  struct TagDetectionResult result = {};

  if (!Config::camera.ENABLE_MULTI_FAMILY_DETECTION) {
    // Single family mode
    result.tag_size_mm = Config::camera.getAprilTagSize();
    result.distance_mm = (result.tag_size_mm * Config::camera.FOCAL_LENGTH_PIXELS_SVGA) / tag_size_pixels;
    result.detected_family = Config::camera.APRILTAG_FAMILY_NAMES[Config::camera.CURRENT_TAG_FAMILY];
    result.detection_phase = "Single tag";
    result.is_concentric_design = false;
    return result;
  }

  // Triple concentric Circle21h7 mode - use tag ID for precise ring identification
  if (Config::camera.CONCENTRIC_CONFIG.USE_ID_FOR_RING_DETECTION && detected_id >= 0) {
    // ID-based detection (precise, no guessing needed)
    if (detected_id == Config::camera.CONCENTRIC_CONFIG.OUTER_TAG_ID) {
      // Outer ring detected: ID 0, 200mm Circle21h7
      result.tag_size_mm = Config::camera.CONCENTRIC_CONFIG.OUTER_TAG_SIZE_MM;
      result.detected_family = "Circle21h7 (Outer Ring)";
      result.detection_phase = "Long Distance Acquisition";
    }
    else if (detected_id == Config::camera.CONCENTRIC_CONFIG.INNER_TAG_ID) {
      // Inner ring detected: ID 1, 42mm Circle21h7
      result.tag_size_mm = Config::camera.CONCENTRIC_CONFIG.INNER_TAG_SIZE_MM;
      result.detected_family = "Circle21h7 (Inner Ring)";
      result.detection_phase = "Precision Landing";
    }
    else {
      // Unknown ID - fallback to outer tag size
      result.tag_size_mm = Config::camera.CONCENTRIC_CONFIG.OUTER_TAG_SIZE_MM; // Default to outer
      result.detected_family = "Circle21h7 (Unknown ID)";
      result.detection_phase = "Unknown Phase";
    }

    // Calculate distance using Circle49h12 multiplier (1.2x for robustness)
    result.distance_mm = (result.tag_size_mm * Config::camera.FOCAL_LENGTH_PIXELS_SVGA * 1.2f) / tag_size_pixels;
    result.is_concentric_design = true;

    return result;
  }

  // Fallback: Size-based detection (when ID not available)
  float outer_distance = (Config::camera.CONCENTRIC_CONFIG.OUTER_TAG_SIZE_MM * Config::camera.FOCAL_LENGTH_PIXELS_SVGA * 1.2f) / tag_size_pixels;
  float inner_distance = (Config::camera.CONCENTRIC_CONFIG.INNER_TAG_SIZE_MM * Config::camera.FOCAL_LENGTH_PIXELS_SVGA * 1.2f) / tag_size_pixels;

  // Choose ring based on most reasonable distance for tag size
  if (outer_distance >= 3000.0f && tag_size_pixels >= 10.0f) {
    // Likely outer tag (200mm) at longer distance
    result.distance_mm = outer_distance;
    result.tag_size_mm = Config::camera.CONCENTRIC_CONFIG.OUTER_TAG_SIZE_MM;
    result.detected_family = "Circle21h7 (Outer Ring - Size)";
    result.detection_phase = "Long Distance Acquisition";
  }
  else {
    // Likely inner tag (42mm) at closer distance
    result.distance_mm = inner_distance;
    result.tag_size_mm = Config::camera.CONCENTRIC_CONFIG.INNER_TAG_SIZE_MM;
    result.detected_family = "Circle21h7 (Inner Ring - Size)";
    result.detection_phase = "Precision Landing";
  }

  result.is_concentric_design = true;
  return result;
}

// Backward compatibility function (ALWAYS AVAILABLE)
float calculateAprilTagDistance(float tag_size_pixels) {
  return calculateSmartAprilTagDistance(tag_size_pixels, -1).distance_mm;
}

// Calculate tag POSITION offset from camera center (translation component)
float calculateTagPositionAngle(float center_x, float center_y) {
  float image_center_x = Config::camera.IMAGE_WIDTH_SVGA / 2.0f;   // 400 pixels
  float image_center_y = Config::camera.IMAGE_HEIGHT_SVGA / 2.0f;  // 300 pixels

  float offset_x = center_x - image_center_x;  // Positive = right, negative = left
  float offset_y = center_y - image_center_y;  // Positive = down, negative = up

  // Calculate horizontal bearing angle (for translation commands)
  float angle_rad = atan2(offset_x, Config::camera.FOCAL_LENGTH_PIXELS_SVGA);
  return angle_rad * 180.0f / M_PI;
}

// Calculate tag ORIENTATION (rotation component) from corner geometry
float calculateTagOrientation(const apriltag_detection_t* det) {
  // Use corner vectors to determine tag's actual rotation
  // Vector from corner 0 to corner 1 (should be the "right" edge when tag is upright)
  float dx = det->p[1][0] - det->p[0][0];
  float dy = det->p[1][1] - det->p[0][1];

  // Calculate angle of this edge relative to horizontal
  float orientation_rad = atan2(dy, dx);
  float orientation_deg = orientation_rad * 180.0f / M_PI;

  // Normalize to [-180, +180] range
  while (orientation_deg > 180.0f) orientation_deg -= 360.0f;
  while (orientation_deg < -180.0f) orientation_deg += 360.0f;

  return orientation_deg;
}

// Compatibility wrapper (deprecated - use specific functions instead)
float calculateAprilTagAngle(float center_x, float center_y) {
  return calculateTagPositionAngle(center_x, center_y);
}

// Memory pressure monitoring function
void checkMemoryPressure(uint32_t frame_number) {
  static uint32_t heap_low_watermark = UINT32_MAX;
  static uint32_t psram_low_watermark = UINT32_MAX;
  static uint32_t warning_count = 0;

  uint32_t free_heap = ESP.getFreeHeap();
  uint32_t free_psram = ESP.getFreePsram();
  uint32_t largest_heap_block = ESP.getMaxAllocHeap();
  uint32_t largest_psram_block = ESP.getMaxAllocPsram();

  // Track low water marks
  if (free_heap < heap_low_watermark) {
    heap_low_watermark = free_heap;
  }
  if (free_psram < psram_low_watermark) {
    psram_low_watermark = free_psram;
  }

  // Memory pressure thresholds
  const uint32_t HEAP_WARNING_THRESHOLD = 20000;   // 20KB heap warning
  const uint32_t HEAP_CRITICAL_THRESHOLD = 10000;  // 10KB heap critical
  const uint32_t PSRAM_WARNING_THRESHOLD = 100000; // 100KB PSRAM warning
  const uint32_t PSRAM_CRITICAL_THRESHOLD = 50000; // 50KB PSRAM critical

  bool memory_warning = false;
  bool memory_critical = false;

  // Check heap pressure
  if (free_heap < HEAP_CRITICAL_THRESHOLD) {
    Serial.printf("🚨 CRITICAL HEAP: %.1f MiB (largest block: %.1f MiB)\n",
                   bytesToMiB(free_heap), bytesToMiB(largest_heap_block));
    memory_critical = true;
  } else if (free_heap < HEAP_WARNING_THRESHOLD) {
    Serial.printf("⚠️ LOW HEAP: %.1f MiB (largest block: %.1f MiB)\n",
                   bytesToMiB(free_heap), bytesToMiB(largest_heap_block));
    memory_warning = true;
  }

  // Check PSRAM pressure
  if (free_psram < PSRAM_CRITICAL_THRESHOLD) {
    Serial.printf("🚨 CRITICAL PSRAM: %.1f MiB (largest block: %.1f MiB)\n",
                   bytesToMiB(free_psram), bytesToMiB(largest_psram_block));
    memory_critical = true;
  } else if (free_psram < PSRAM_WARNING_THRESHOLD) {
    Serial.printf("⚠️ LOW PSRAM: %.1f MiB (largest block: %.1f MiB)\n",
                   bytesToMiB(free_psram), bytesToMiB(largest_psram_block));
    memory_warning = true;
  }

  // Check for severe fragmentation
  if (largest_heap_block < (free_heap / 4)) {
    Serial.printf("🔴 HEAP FRAGMENTED: %u free but largest block only %u\n", free_heap, largest_heap_block);
    memory_warning = true;
  }

  if (largest_psram_block < (free_psram / 4)) {
    Serial.printf("🔴 PSRAM FRAGMENTED: %u free but largest block only %u\n", free_psram, largest_psram_block);
    memory_warning = true;
  }

  // Report warnings periodically
  if (memory_warning || memory_critical) {
    warning_count++;
    if (warning_count % 10 == 1) {  // Every 10th warning
      Serial.printf("💾 Memory watermarks - Heap: %u, PSRAM: %u (frame %u)\n",
                    heap_low_watermark, psram_low_watermark, frame_number);
    }
  }

  // Force garbage collection if memory is critically low
  if (memory_critical) {
    Serial.println("🧹 Forcing garbage collection...");
    ESP.restart();  // Restart if memory is critically low to prevent system crash
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32-S3-CAM Multi-Function System ===");
  Serial.println("Version: 2.0.0");
  Serial.println("Features: Photo Capture, S3 Upload, NTRIP RTK");
  Serial.printf("Free heap: %.1f MiB\n", bytesToMiB(ESP.getFreeHeap()));

  // Initialize PSRAM for AprilTag processing
  // IMPORTANT: Compile with PSRAM=enabled (QSPI PSRAM) board option for full support
  Serial.print("Init PSRAM... ");
  psramInit();
  Serial.println("done");
  Serial.printf("Total PSRAM: %.1f MiB\n", bytesToMiB(ESP.getPsramSize()));
  Serial.printf("Free PSRAM: %.1f MiB\n", bytesToMiB(ESP.getFreePsram()));

  // Enable detailed ESP camera driver logging for error debugging
  Serial.println("Enabling camera driver debug logging...");
  esp_log_level_set("*", ESP_LOG_WARN);              // Keep other logs at warning level
  esp_log_level_set("camera", ESP_LOG_DEBUG);        // Camera driver detailed logs
  esp_log_level_set("cam_hal", ESP_LOG_DEBUG);       // Camera HAL layer
  esp_log_level_set("sccb", ESP_LOG_DEBUG);          // SCCB camera communication
  esp_log_level_set("camera_xclk", ESP_LOG_DEBUG);   // Camera clock management
  esp_log_level_set("ledc", ESP_LOG_DEBUG);          // LED controller (XCLK generation)
  esp_log_level_set("gpio", ESP_LOG_DEBUG);          // GPIO operations for camera pins
  Serial.println("Camera driver debug logging enabled - will show detailed error info");

  if (ESP.getPsramSize() == 0) {
    Serial.println("⚠️  WARNING: PSRAM not detected! AprilTag may fail due to memory constraints");
    Serial.println("   Make sure to compile with board option PSRAM=enabled (QSPI PSRAM)");
  }

  // Initialize system
  initializeSystem();
  
  // Skip camera task creation completely in AprilTag test mode
  Serial.println("Camera task creation skipped - using main task for AprilTag processing");

  // CAMERA + APRILTAG MODE: Skip upload task (no SD files to upload)
  Serial.println("Upload task skipped - no SD operations in test mode");

  // Create NTRIP task on core 1 with much larger stack for SSL operations
  #ifndef VISUAL_CODE_TEST_MODE
  if (Config::ntrip.enabled) {
    xTaskCreatePinnedToCore(
      ntripClientTask,
      "NTRIPClient",
      20480,  // 20KB - increased from 8KB for SSL/TLS handshakes and operations
      NULL,
      2,      // Medium priority between camera and upload
      &ntripTaskHandle,
      0       // Core 0 with WiFi for network operations
    );
    if (ntripTaskHandle == NULL) {
      Serial.println("FATAL: Failed to create NTRIPClientTask!");
      // ESP.restart(); // Or handle error appropriately
    } else {
      Serial.println("NTRIP client initialized on core 0 (with WiFi)");
    }
  }
  #endif // !VISUAL_CODE_TEST_MODE
  
  Serial.println("\n=== System Ready ===");
#ifdef AUTO_APRILTAG_TEST_MODE
  Serial.println("AUTOMATIC APRILTAG TEST MODE ENABLED");
  Serial.println("System will capture automatically - no buttons needed!");
#else
  Serial.println("Press capture button to start/stop capture");
  Serial.println("Press upload button to upload photos to S3");
#endif

#ifdef VISUAL_CODE_TEST_MODE
  // AprilTag initialization with full debug output (stack issue resolved by ESP32 core upgrade)
  // ESP32 Arduino Core 3.3.3+ provides 4KB camera task stack, sufficient for printf operations

  // CRITICAL: Optimize WiFi to prevent resource conflicts with camera/AprilTag operations
  // WiFi power management was causing system crashes in PHY layer
  Serial.println("⚙️ Optimizing WiFi settings for camera operation stability...");

  // Reduce WiFi power management aggressiveness
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);  // Minimal power saving to reduce conflicts

  // Set WiFi to support all protocols including 11N for maximum performance
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);

  // Core allocation optimized:
  // Core 0: WiFi tasks + NTRIP (network operations)
  // Core 1: Camera task + AprilTag (vision operations)
  Serial.println("   WiFi optimized for camera coexistence");
  Serial.println("   Core separation: WiFi+NTRIP(0) | Camera+Vision(1)");

  // Initialize camera (4KB stack should now be sufficient for debug output)
  Serial.println("Starting camera init...");
  if (!CameraManager::init()) {
    Serial.println("Camera init failed!");
    ESP.restart();
  }
  Serial.println("Camera init completed!");

  // Initialize AprilTag detector (proven working)
  Serial.println("Initializing AprilTag detector...");

  // Initialize Circle21h7 family (used for all 3 concentric ring sizes)
  apriltag_family_t *tf = tagCircle21h7_create();
  if (!tf) {
    Serial.println("ERROR: Failed to create tagCircle21h7 family");
    ESP.restart();
  }
  Serial.println("✅ Circle21h7 family initialized (triple concentric design)");

  apriltag_detector_t *td = apriltag_detector_create();
  if (!td) {
    Serial.println("ERROR: Failed to create apriltag detector");
    tagCircle21h7_destroy(tf);
    ESP.restart();
  }

  // Add family to detector with 2-bit error correction (good balance)
  apriltag_detector_add_family_bits(td, tf, 2);

  // Configure detector parameters for WiFi-compatible speed
  td->quad_sigma = 0.0;           // No blur
  td->quad_decimate = 8.0;        // Very aggressive decimation for speed (prevent WiFi starvation)
  td->refine_edges = 0;           // Disable edge refinement for speed
  td->decode_sharpening = 0.0;    // Disable sharpening for speed
  td->nthreads = 1;
  td->debug = 0;

  Serial.println("AprilTag detector ready for processing");

  // LED status indication
  pinMode(Config::pins.LED_STATUS_PIN, OUTPUT);
  digitalWrite(Config::pins.LED_STATUS_PIN, HIGH);

  Serial.println("=== STARTING APRILTAG DETECTION ===");
  Serial.printf("Memory: %u heap, %u PSRAM available\n", ESP.getFreeHeap(), ESP.getFreePsram());

  // Display camera calibration info
  Serial.println("\n📸 Camera Calibration Parameters:");
  Serial.printf("   Resolution: %dx%d pixels (SVGA)\n", Config::camera.IMAGE_WIDTH_SVGA, Config::camera.IMAGE_HEIGHT_SVGA);
  Serial.printf("   Sensor: OV2640 (%.1f × %.1f mm)\n", Config::camera.SENSOR_WIDTH_MM, Config::camera.SENSOR_HEIGHT_MM);
  Serial.printf("   Focal length: %.1f mm (%.0f pixels)\n", Config::camera.FOCAL_LENGTH_MM, Config::camera.FOCAL_LENGTH_PIXELS_SVGA);
  Serial.printf("   Image center: (%.0f, %.0f) pixels\n",
                Config::camera.IMAGE_WIDTH_SVGA / 2.0f, Config::camera.IMAGE_HEIGHT_SVGA / 2.0f);

  // Display AprilTag configuration
  Serial.println("\n🎯 AprilTag Configuration:");

  if (Config::camera.ENABLE_MULTI_FAMILY_DETECTION) {
    Serial.println("   Mode: Multi-Family Concentric Design");
    Serial.println("   ┌─────────────────────────────────────┐");
    Serial.printf("   │ Outer:  Circle21h7 ID0 %.0fmm (%.1fm range)│\n",
                  Config::camera.CONCENTRIC_CONFIG.OUTER_TAG_SIZE_MM,
                  (Config::camera.CONCENTRIC_CONFIG.OUTER_TAG_SIZE_MM * Config::camera.FOCAL_LENGTH_PIXELS_SVGA / 20.0f * 1.2f) / 1000.0f);
    Serial.printf("   │ Inner:  Circle21h7 ID1 %.0fmm (%.1fm range)│\n",
                  Config::camera.CONCENTRIC_CONFIG.INNER_TAG_SIZE_MM,
                  (Config::camera.CONCENTRIC_CONFIG.INNER_TAG_SIZE_MM * Config::camera.FOCAL_LENGTH_PIXELS_SVGA / 20.0f * 1.2f) / 1000.0f);
    Serial.println("   └─────────────────────────────────────┘");

    float total_size = Config::camera.CONCENTRIC_CONFIG.OUTER_TAG_SIZE_MM * 1.25f;
    Serial.printf("   Total physical size: %.1fm × %.1fm\n", total_size/1000.0f, total_size/1000.0f);

    // Calculate full detection range
    float max_range = (Config::camera.CONCENTRIC_CONFIG.OUTER_TAG_SIZE_MM * Config::camera.FOCAL_LENGTH_PIXELS_SVGA / 20.0f * 2.25f) / 1000.0f;
    float min_range = (Config::camera.CONCENTRIC_CONFIG.INNER_TAG_SIZE_MM * Config::camera.FOCAL_LENGTH_PIXELS_SVGA / 100.0f) / 1000.0f;
    Serial.printf("   Full detection range: %.1fm - %.1fm\n", min_range, max_range);

    Serial.println("   Auto-detection: Enabled (automatic family/ring selection)");
  }
  else {
    Serial.println("   Mode: Single Family Detection");
    Serial.printf("   Tag Family: %s\n", Config::camera.APRILTAG_FAMILY_NAMES[Config::camera.CURRENT_TAG_FAMILY]);
    Serial.printf("   %s\n", Config::camera.APRILTAG_FAMILY_RECOMMENDATIONS[Config::camera.CURRENT_TAG_FAMILY]);
    Serial.printf("   Size preset: %s\n", Config::camera.APRILTAG_DESCRIPTIONS[Config::camera.CURRENT_TAG_SIZE]);
    Serial.printf("   Tag size: %.1f mm (%s)\n",
                  Config::camera.getAprilTagSize(),
                  Config::camera.MEASURE_BLACK_SQUARE ? "black square" : "total");

    // Circle49h12 family detection range
    float family_multiplier = 1.2f;  // 🎯 Circular, robust but dense

    float min_distance = (Config::camera.getAprilTagSize() * Config::camera.FOCAL_LENGTH_PIXELS_SVGA / 100.0f * family_multiplier) / 1000.0f;
    float max_distance = (Config::camera.getAprilTagSize() * Config::camera.FOCAL_LENGTH_PIXELS_SVGA / 20.0f * family_multiplier) / 1000.0f;
    Serial.printf("   Detection range: %.1fm - %.1fm\n", min_distance, max_distance);
  }

  Serial.println();

  uint32_t frame_count = 0;
  uint32_t detections_found = 0;

  // Timing statistics
  unsigned long total_timing_ms = 0;
  unsigned long min_loop_time = 9999;
  unsigned long max_loop_time = 0;

  while (true) {
    // Performance monitoring
    unsigned long loop_start = millis();

    // Get a frame from camera (should be GRAYSCALE format)
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      // Get detailed error information without reinitializing
      sensor_t *s = esp_camera_sensor_get();

      Serial.printf("❌ Failed to get frame! Error details:\n");
      Serial.printf("   Camera sensor: %s\n", s ? "Available" : "NULL - Camera not initialized");
      Serial.printf("   Free heap: %.1f MiB (need >0.05 MiB)\n", bytesToMiB(ESP.getFreeHeap()));
      Serial.printf("   Free PSRAM: %.1f MiB (need >0.5 MiB for frames)\n", bytesToMiB(ESP.getFreePsram()));
      Serial.printf("   Largest heap block: %.1f MiB\n", bytesToMiB(ESP.getMaxAllocHeap()));
      Serial.printf("   Largest PSRAM block: %.1f MiB\n", bytesToMiB(ESP.getMaxAllocPsram()));

      // Check for specific failure conditions
      bool found_cause = false;
      if (ESP.getFreeHeap() < 50000) {
        Serial.println("   ⚠️  LIKELY CAUSE: Insufficient heap memory");
        found_cause = true;
      }
      if (ESP.getFreePsram() < 500000) {
        Serial.println("   ⚠️  LIKELY CAUSE: Insufficient PSRAM for frame buffers");
        found_cause = true;
      }
      if (!s) {
        Serial.println("   ⚠️  LIKELY CAUSE: Camera hardware/initialization failure");
        found_cause = true;
      }

      // If no obvious cause found, show ESP camera driver logs will provide details
      if (!found_cause && s) {
        Serial.println("   🔍 Memory looks good - checking camera hardware status...");
        Serial.printf("   📷 Camera sensor ID: 0x%02X (should be 0x26 for OV2640)\n", s->id.PID);
        Serial.println("   💡 ESP camera driver logs enabled - check output for detailed error info");
        Serial.println("   ⚠️  LIKELY CAUSE: Camera DMA/timing conflict or hardware communication failure");
        Serial.println("   💡 SUGGESTION: This typically indicates WiFi/camera resource competition on Core 0");
      }

      Serial.printf("   Retrying in 1 second...\n");
      delay(1000);
      continue;
    }

    unsigned long frame_get_time = millis() - loop_start;
    Serial.printf("📷 Got %dx%d GRAYSCALE frame: %.1f MiB (%lu ms)\n",
                  fb->width, fb->height, bytesToMiB(fb->len), frame_get_time);

    // Create AprilTag detector input format (direct from grayscale frame)
    image_u8_t im = {
      .width = (int32_t)fb->width,
      .height = (int32_t)fb->height,
      .stride = (int32_t)fb->width,
      .buf = fb->buf  // Direct grayscale data, no conversion needed
    };

    // Yield to WiFi tasks before intensive AprilTag processing
    yield();  // Let WiFi stack run before CPU-intensive detection

    // Detect AprilTags with timing
    unsigned long detect_start = millis();
    zarray_t *detections = apriltag_detector_detect(td, &im);
    unsigned long detect_time = millis() - detect_start;

    // Yield immediately after detection to prevent WiFi starvation
    yield();  // Let WiFi stack recover after intensive processing

    // Process detection results with full details
    int num_detections = zarray_size(detections);
    if (num_detections > 0) {
      Serial.print("DETECTED: ");
      for (int i = 0; i < num_detections; i++) {
        apriltag_detection_t *det;
        zarray_get(detections, i, &det);

        // Display full detection information
        Serial.printf("🎯 AprilTag ID %d at (%.1f,%.1f)\n", det->id, det->c[0], det->c[1]);
        Serial.printf("   Quality: %.2f (decision margin)\n", det->decision_margin);
        Serial.printf("   Corners: (%.1f,%.1f) (%.1f,%.1f) (%.1f,%.1f) (%.1f,%.1f)\n",
                      det->p[0][0], det->p[0][1], det->p[1][0], det->p[1][1],
                      det->p[2][0], det->p[2][1], det->p[3][0], det->p[3][1]);

        // Calculate tag size
        float size_sum = 0;
        for (int j = 0; j < 4; j++) {
          int next = (j + 1) % 4;
          float dx = det->p[next][0] - det->p[j][0];
          float dy = det->p[next][1] - det->p[j][1];
          size_sum += sqrt(dx*dx + dy*dy);
        }
        float avg_size = size_sum / 4.0f;
        Serial.printf("   📏 Size: %.1f pixels\n", avg_size);

        // Calculate distance using smart detection with tag ID
        TagDetectionResult smart_result = calculateSmartAprilTagDistance(avg_size, det->id);
        float distance_cm = smart_result.distance_mm / 10.0f;

        // Calculate BOTH translation and rotation components separately
        float position_angle = calculateTagPositionAngle(det->c[0], det->c[1]);
        float tag_orientation = calculateTagOrientation(det);

        // Show smart detection results
        if (smart_result.is_concentric_design) {
          Serial.printf("   🎯 Detected: %s\n", smart_result.detected_family);
          Serial.printf("   📍 Mission Phase: %s\n", smart_result.detection_phase);
          Serial.printf("   📏 Ring Size: %.0fmm black square\n", smart_result.tag_size_mm);
        }

        Serial.printf("   📐 Distance: %.1f cm (%.0f mm)\n", distance_cm, smart_result.distance_mm);

        // CRITICAL: Separate translation from rotation for drone control
        Serial.printf("   🧭 TRANSLATION: %.1f° %s (move drone %s)\n",
                      abs(position_angle),
                      position_angle > 0 ? "right" : position_angle < 0 ? "left" : "centered",
                      position_angle > 0 ? "LEFT" : position_angle < 0 ? "RIGHT" : "nowhere");
        Serial.printf("   🔄 ROTATION: %.1f° (rotate drone %s to align)\n",
                      tag_orientation,
                      tag_orientation > 0 ? "CCW" : "CW");

        // Additional navigation info
        float image_center_x = Config::camera.IMAGE_WIDTH_SVGA / 2.0f;
        float offset_pixels = det->c[0] - image_center_x;
        Serial.printf("   🎯 Position: %.0fpx %s of center (image center: %.0fpx)\n",
                      abs(offset_pixels), offset_pixels > 0 ? "right" : "left", image_center_x);

        // Mission guidance based on distance and detected ring
        if (smart_result.is_concentric_design) {
          if (smart_result.distance_mm > 20000.0f) {
            Serial.printf("   🚁 Guidance: Continue approach - target %.0fm for next phase\n",
                          smart_result.distance_mm / 2000.0f);  // Suggest halfway point
          } else if (smart_result.distance_mm > 5000.0f) {
            Serial.printf("   🚁 Guidance: Approach phase - prepare for precision landing below 5m\n");
          } else if (smart_result.distance_mm > 1000.0f) {
            Serial.printf("   🚁 Guidance: Precision landing zone - maintain position accuracy\n");
          } else {
            Serial.printf("   🚁 Guidance: Final approach - ready for landing!\n");
          }
        }

        detections_found++;

        // Yield between processing multiple detections to prevent WiFi starvation
        yield();  // Let WiFi tasks run between detection processing
      }
      Serial.println();
    } else {
      if (frame_count % 10 == 0) {
        Serial.printf("❌ No AprilTags detected (frame %u, detection time: %lu ms)\n", frame_count, detect_time);
      } else {
        Serial.print(".");  // Progress indicator
      }
    }

    // Cleanup
    apriltag_detections_destroy(detections);
    esp_camera_fb_return(fb);

    // Memory pressure monitoring after each AprilTag processing cycle
    checkMemoryPressure(frame_count);

    // Show frame count every 100 frames
    frame_count++;
    if (frame_count % 100 == 0) {
      Serial.printf("\nFrame %u processed (Total detections: %u)\n", frame_count, detections_found);
      Serial.printf("Memory: %.1f MiB heap, %.1f MiB PSRAM free\n",
                     bytesToMiB(ESP.getFreeHeap()), bytesToMiB(ESP.getFreePsram()));
    }

    // FPS rate limiting to reduce WiFi/camera resource conflicts
    unsigned long frame_end_time = millis();
    unsigned long frame_duration = frame_end_time - loop_start;

    // Calculate remaining time to maintain target FPS
    uint32_t target_frame_time = Config::camera.MIN_FRAME_INTERVAL_MS;
    if (frame_duration < target_frame_time) {
      uint32_t delay_needed = target_frame_time - frame_duration;
      Serial.printf("📊 Frame %u: %lums processing, delaying %ums (target: %u FPS)\n",
                    frame_count + 1, frame_duration, delay_needed, Config::camera.MAX_FPS);

      // Use delay() which yields to other tasks in Arduino ESP32 framework
      delay(delay_needed);
    } else {
      // Frame took longer than target - no additional delay needed
      Serial.printf("⚠️  Frame %u: %lums processing (slower than %u FPS target)\n",
                    frame_count + 1, frame_duration, Config::camera.MAX_FPS);
      // Still yield briefly to allow other processing
      delay(10);
    }
  }
#endif

  Serial.println();
}

void loop() {
  // Loop function disabled - all processing done in setup() infinite loop
  // This prevents any interference with main task AprilTag processing
  delay(1000);
}

void initializeSystem() {
  // Initialize GPIO pins
  pinMode(Config::pins.CAPTURE_TRIGGER_PIN, INPUT_PULLUP);
  pinMode(Config::pins.UPLOAD_TRIGGER_PIN, INPUT_PULLUP);
  pinMode(Config::pins.LED_STATUS_PIN, OUTPUT);
  digitalWrite(Config::pins.LED_STATUS_PIN, LOW);
  
  // Initialize system state
  SystemState::init();

  // Initialize SPI mutex to prevent SD/camera conflicts
  spiMutex = xSemaphoreCreateMutex();
  if (spiMutex == nullptr) {
    Serial.println("FATAL: Failed to create SPI mutex!");
    ESP.restart();
  }
  Serial.println("SPI conflict prevention mutex created");

  // ESP32S3 CRITICAL: Initialize SPI early and set camera CS high for proper SPI sharing
  // ESP32S3 has no default SPI pins and needs explicit early initialization
  #if defined(ARDUINO_XIAO_ESP32S3)
    Serial.println("Early SPI initialization for ESP32S3...");
    SPI.begin(7, 8, 9, 21); // SCK=7, MISO=8, MOSI=9, CS=21
    delay(100); // Allow SPI to stabilize

    // ESP-IDF SPI SHARING REQUIREMENT: Set camera CS lines HIGH before SD mount
    // This prevents camera from interfering with SD card SPI operations
    Serial.println("Setting camera SPI chip select lines HIGH for SPI bus sharing...");
    pinMode(10, OUTPUT); digitalWrite(10, HIGH);  // XCLK as CS (camera clock control)
    pinMode(39, OUTPUT); digitalWrite(39, HIGH);  // SIOC as CS (camera I2C clock)
    pinMode(40, OUTPUT); digitalWrite(40, HIGH);  // SIOD as CS (camera I2C data)
    delay(50); // Ensure lines are stable

    Serial.println("SPI configured for safe SD/camera sharing");

    // ESP32S3 PSRAM FIX: Check memory allocation and warn if PSRAM is affecting SD
    // PSRAM causes additional SPI round-trips that can hang SD operations
    Serial.println("Checking memory allocation configuration...");
    Serial.printf("Free internal SRAM: %.1f MiB\n", bytesToMiB(ESP.getFreeHeap()));
    #ifdef CONFIG_SPIRAM
      Serial.printf("Free PSRAM: %.1f MiB\n", bytesToMiB(ESP.getPsramSize() - (ESP.getPsramSize() - ESP.getFreePsram())));
      Serial.println("PSRAM detected - SD operations use internal SRAM buffers");
    #else
      Serial.println("No PSRAM detected - using internal SRAM only");
    #endif
    Serial.println("Memory configuration checked for SD card compatibility");
  #endif
  
  // Initialize storage (SD card) - Required for config loading
  Serial.println("=== SD CARD INIT (Config Only) ===");
  Serial.println("SD card enabled for config loading");
  Serial.println("Photo writing disabled to avoid SPI conflicts");
  if (!StorageManager::init()) {
    Serial.println("WARNING: SD card initialization failed!");
    Serial.println("System will use hardcoded config and continue.");
    Serial.println("Photo capture will work but no local storage.");
    // Continue without SD card - system can still work for camera + NTRIP
  } else {
    Serial.println("SD card available for config loading");
  }
  
  // Load configuration from SD card if exists
  if (!Config::loadFromFile()) {
    Serial.println("WARNING: Failed to load configuration from file or critical settings are missing.");
    Serial.println("Using hardcoded default configuration.");
    // Continue with hardcoded defaults
  } else {
    Serial.println("Configuration loaded successfully from SD card");
  }
  
  // Initialize network (WiFi)
  if (!WiFiManager::connectWiFi()) {
    Serial.println("WARNING: WiFi connection failed!");
    Serial.println("System will continue without network connectivity.");
    Serial.println("NTRIP and S3 upload will be disabled, but camera and AprilTag detection will work.");
    // Continue without WiFi - core functionality can still work
  } else {
    Serial.println("WiFi connected successfully");

    // Initialize time (only when WiFi is connected)
    WiFiManager::initializeTime();
  }
  
  // Camera initialization moved to camera task (32KB stack) for AprilTag compatibility
  // This prevents stack overflow that occurs when running AprilTag on main task (8KB stack)
  Serial.println("Camera initialization deferred to camera task to avoid stack overflow");
  
  // CAMERA + APRILTAG MODE: Skip upload tracking and storage cleanup
  Serial.println("Skipping upload tracking and storage cleanup (no SD operations)");
  
  // Initialize power management
  if (Config::power.ENABLE_OPTIMIZATION) {
    PowerManager::init();
  }
}

void handleButtons() {
  bool currentCaptureState = digitalRead(Config::pins.CAPTURE_TRIGGER_PIN);
  bool currentUploadState = digitalRead(Config::pins.UPLOAD_TRIGGER_PIN);
  
  // Debounce logic
  if ((currentCaptureState != lastCaptureState || currentUploadState != lastUploadState) &&
      (millis() - lastDebounceTime > Config::timing.DEBOUNCE_DELAY)) {
    
    lastDebounceTime = millis();
    
    // Handle capture button (falling edge)
    if (lastCaptureState == HIGH && currentCaptureState == LOW) {
      SystemState::updateActivity();
      
      if (!SystemState::isCapturing()) {
        // Signal camera task to start capture
        if (cameraTaskHandle) {
          xTaskNotify(cameraTaskHandle, 1, eSetValueWithOverwrite);
        }
      } else {
        // Signal camera task to stop capture
        if (cameraTaskHandle) {
          xTaskNotify(cameraTaskHandle, 2, eSetValueWithOverwrite);
        }
      }
    }
    
    // Handle upload button (falling edge) - DISABLED in camera+AprilTag mode
    if (lastUploadState == HIGH && currentUploadState == LOW) {
      SystemState::updateActivity();

      // CAMERA + APRILTAG MODE: No upload operations
      Serial.println("Upload button pressed but upload disabled in test mode");
    }
    
    lastCaptureState = currentCaptureState;
    lastUploadState = currentUploadState;
  }
}

#ifdef AUTO_APRILTAG_TEST_MODE
void handleAutomaticTesting() {
  static unsigned long lastAutoCapture = 0;
  static bool testModeStarted = false;
  static bool hasLoggedTestMode = false;

  // Log test mode activation once
  if (!hasLoggedTestMode) {
    Serial.println("\n=== AUTOMATIC APRILTAG TEST MODE ACTIVE ===");
    Serial.println("Captures will start automatically in 5 seconds...");
    Serial.println("No button presses required!");
    Serial.println("==========================================");
    hasLoggedTestMode = true;
  }

  // Wait 5 seconds after boot before starting automatic captures
  if (!testModeStarted && millis() > 5000) {
    testModeStarted = true;
    lastAutoCapture = millis();
    Serial.println("\n>>> Starting automatic AprilTag test captures <<<");

    // Automatically start capture mode
    SystemState::updateActivity();
    if (cameraTaskHandle) {
      xTaskNotify(cameraTaskHandle, 1, eSetValueWithOverwrite);
    }
  }

  // Trigger uploads every 30 seconds in test mode
  static unsigned long lastAutoUpload = 0;
  if (testModeStarted && millis() - lastAutoUpload > 30000) {
    lastAutoUpload = millis();
    Serial.println("\n>>> Triggering automatic upload <<<");
    SystemState::updateActivity();
    if (uploadTaskHandle) {
      xTaskNotify(uploadTaskHandle, 1, eSetValueWithOverwrite);
    }
  }

  // Show status every 10 seconds
  static unsigned long lastStatusLog = 0;
  if (testModeStarted && millis() - lastStatusLog > 10000) {
    lastStatusLog = millis();
    Serial.printf("[AUTO-TEST] Capture active: %s, Photos taken this session\n",
                  SystemState::isCapturing() ? "YES" : "NO");
  }
}
#endif

// Camera task disabled in AprilTag main task mode
/*
void cameraTask(void* parameter) {
  // CRITICAL: Debug task creation and stack allocation immediately
  TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
  char* taskName = pcTaskGetName(currentTask);
  UBaseType_t stackWatermark = uxTaskGetStackHighWaterMark(currentTask);

  Serial.println("=== CAMERA TASK STARTUP DEBUG ===");
  Serial.print("Task name: ");
  Serial.println(taskName);
  Serial.print("Stack high watermark: ");
  Serial.print(stackWatermark);
  Serial.print(" words (");
  Serial.print(stackWatermark * 4);
  Serial.println(" bytes)");
  Serial.print("Expected stack: 8192 words (32768 bytes)");
  if (stackWatermark > 10000) {
    Serial.println(" --> ERROR: Stack measurement impossible!");
  } else {
    Serial.println(" --> Stack measurement looks normal");
  }
  Serial.println("=== END CAMERA TASK DEBUG ===");

  // CRITICAL: Initialize camera on 32KB camera task (not 8KB main task)
  Serial.println("🎥 Initializing camera on camera task...");
  if (!CameraManager::init()) {
    Serial.println("❌ Camera initialization FAILED on camera task!");
    vTaskDelete(NULL); // Delete this task if camera init fails
    return;
  }
  Serial.println("✅ Camera initialization SUCCESS on camera task!");

  // Initialize AprilTag detector on 32KB camera task
  #ifdef VISUAL_CODE_TEST_MODE
  Serial.println("🔍 Initializing AprilTag detector on camera task...");
  if (!AprilTagDetector::init()) {
    Serial.println("❌ AprilTag detector initialization FAILED!");
  } else {
    Serial.println("✅ AprilTag detector initialization SUCCESS!");
  }
  #endif

  // Initialize watchdog for camera task
  esp_task_wdt_add(NULL);
  Serial.println("Camera task started with watchdog protection");

  uint32_t notificationValue;

  while (true) {
    // Reset watchdog timer with debugging
    esp_task_wdt_reset();
    static unsigned long lastCameraWatchdogReset = 0;
    if (millis() - lastCameraWatchdogReset > 5000) { // Log every 5 seconds
      Serial.printf("[DEBUG] Camera task watchdog reset at %lu ms\n", millis());
      lastCameraWatchdogReset = millis();
    }

    // Wait for notification with timeout
    if (xTaskNotifyWait(0, 0xFFFFFFFF, &notificationValue, pdMS_TO_TICKS(100))) {
      if (notificationValue == 1) {
        // Start capture
        CameraManager::startCapture();
      } else if (notificationValue == 2) {
        // Stop capture
        CameraManager::stopCapture();
      }
    }

    // Handle continuous capture
    if (SystemState::isCapturing()) {
      unsigned long currentTime = millis();
      if (currentTime - SystemState::getLastCaptureTime() >= Config::timing.CAPTURE_INTERVAL) {
        CameraManager::capturePhoto();
        SystemState::setLastCaptureTime(currentTime);
      }
    }

    // Small delay to prevent task hogging
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
*/

void uploadTask(void* parameter) {
  // Initialize watchdog for upload task
  esp_task_wdt_add(NULL);
  Serial.println("Upload task started with watchdog protection");

  uint32_t notificationValue;

  while (true) {
    // Reset watchdog timer with debugging
    esp_task_wdt_reset();
    static unsigned long lastUploadWatchdogReset = 0;
    if (millis() - lastUploadWatchdogReset > 5000) { // Log every 5 seconds
      Serial.printf("[DEBUG] Upload task watchdog reset at %lu ms\n", millis());
      lastUploadWatchdogReset = millis();
    }

    // Wait for notification with timeout
    if (xTaskNotifyWait(0, 0xFFFFFFFF, &notificationValue, pdMS_TO_TICKS(1000))) {
      if (notificationValue == 1) {
        // Pause capture if active
        bool wasCapturing = SystemState::isCapturing();
        if (wasCapturing) {
          CameraManager::stopCapture();
        }

        // Perform upload
        UploadManager::uploadPendingDirectories();

        // Cleanup after upload
        StorageManager::performCleanup();

        // Resume capture if it was active
        if (wasCapturing) {
          CameraManager::startCapture();
        }
      }
    }

    // Check for scheduled uploads (every hour)
    static unsigned long lastScheduledUpload = 0;
    if (Config::upload.AUTO_UPLOAD &&
        millis() - lastScheduledUpload > 3600000) { // 1 hour
      lastScheduledUpload = millis();
      if (uploadTaskHandle != NULL) {
        xTaskNotify(uploadTaskHandle, 1, eSetValueWithOverwrite);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void performHealthCheck() {
  Serial.println("\n--- Health Check ---");
  Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
  Serial.printf("Free heap: %.1f MiB\n", bytesToMiB(ESP.getFreeHeap()));
  Serial.printf("WiFi RSSI: %d dBm\n", WiFi.RSSI());

  // Check storage with rate limiting to prevent SPI contention
  uint64_t totalBytes, usedBytes;
  StorageManager::getSpaceInfo(totalBytes, usedBytes);
  if (totalBytes > 0) {
    Serial.printf("Storage: %.1f/%.1f GB used\n",
                  usedBytes / 1024.0 / 1024.0 / 1024.0,
                  totalBytes / 1024.0 / 1024.0 / 1024.0);
  } else {
    Serial.println("Storage: SD card not accessible or rate limited");
  }

  // ESP32 DEBUGGING: Enhanced task state monitoring to identify watchdog timeout culprit
  Serial.println("\n--- Task State Debug ---");
  if (cameraTaskHandle) {
    eTaskState cameraState = eTaskGetState(cameraTaskHandle);
    UBaseType_t cameraStackWatermark = uxTaskGetStackHighWaterMark(cameraTaskHandle);
    Serial.printf("Camera task: %s (stack free: %u words)\n",
                  cameraState == eRunning ? "RUNNING" :
                  cameraState == eReady ? "READY" :
                  cameraState == eBlocked ? "BLOCKED" :
                  cameraState == eSuspended ? "SUSPENDED" : "DELETED",
                  cameraStackWatermark);
  }

  if (uploadTaskHandle) {
    eTaskState uploadState = eTaskGetState(uploadTaskHandle);
    UBaseType_t uploadStackWatermark = uxTaskGetStackHighWaterMark(uploadTaskHandle);
    Serial.printf("Upload task: %s (stack free: %u words)\n",
                  uploadState == eRunning ? "RUNNING" :
                  uploadState == eReady ? "READY" :
                  uploadState == eBlocked ? "BLOCKED" :
                  uploadState == eSuspended ? "SUSPENDED" : "DELETED",
                  uploadStackWatermark);
  }

  if (ntripTaskHandle) {
    eTaskState ntripState = eTaskGetState(ntripTaskHandle);
    UBaseType_t ntripStackWatermark = uxTaskGetStackHighWaterMark(ntripTaskHandle);
    Serial.printf("NTRIP task: %s (stack free: %u words)\n",
                  ntripState == eRunning ? "RUNNING" :
                  ntripState == eReady ? "READY" :
                  ntripState == eBlocked ? "BLOCKED" :
                  ntripState == eSuspended ? "SUSPENDED" : "DELETED",
                  ntripStackWatermark);
    NtripClient::printStatistics();
  }

  Serial.println("-------------------\n");
}