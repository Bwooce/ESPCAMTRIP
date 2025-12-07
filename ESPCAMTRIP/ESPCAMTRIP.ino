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

// Raw RTCM baud rate (only used when RTCM_OUTPUT_RAW is defined)
// ZED-F9P defaults to 38400 on UART1/UART2 - change to match your receiver
// or configure your receiver to 115200 via u-center
#define RTCM_RAW_BAUD_RATE 115200

// GPS Input Mode: Choose ONE of the following input modes
// GPS_INPUT_RAW (default) - NMEA/UBX data directly from F9P GPS receiver
// GPS_INPUT_MAVLINK       - GPS data via MAVLink from ArduPilot flight controller
#define GPS_INPUT_RAW  // Default: Direct GPS receiver connection

// Landing Mode Timeout (3 minutes)
#define LANDING_MODE_TIMEOUT_MS (3 * 60 * 1000)

// AprilTag Detection (enable when library is installed)
#define APRILTAG_ENABLED

// ============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
#include "config.h"
#include "camera_manager.h"
#include "camera_mode_manager.h"
#include "gps_manager.h"
#include "storage_manager.h"
#include "wifi_manager.h"
#include "upload_manager.h"
#include "ntrip_client.h"
#include "power_manager.h"
#include "system_state.h"
#include "apriltag_manager.h"
#include "mavlink_manager.h"
#include "psram_manager.h"
#include "exif_gps_static.h"
#include "esp_camera.h"

// Task handles
TaskHandle_t cameraTaskHandle = NULL;
TaskHandle_t uploadTaskHandle = NULL;
TaskHandle_t ntripTaskHandle = NULL;

// Button debouncing
unsigned long lastDebounceTime = 0;
bool lastCaptureState = HIGH;
bool lastUploadState = HIGH;

// Camera mode state machine
unsigned long landingModeStartTime = 0;
bool landingModeActive = false;

// Function declarations
void cameraTask(void* parameter);
void uploadTask(void* parameter);
void handleButtons();
void initializeSystem();
void performHealthCheck();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32-S3-CAM Multi-Function System ===");
  Serial.println("Version: 2.0.0");
  Serial.println("Features: Photo Capture, S3 Upload, NTRIP RTK");
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  
  // Initialize system
  initializeSystem();
  
  // Create camera task on core 0
  xTaskCreatePinnedToCore(
    cameraTask,
    "CameraTask",
    8192,
    NULL,
    2,
    &cameraTaskHandle,
    0
  );
  if (cameraTaskHandle == NULL) {
    Serial.println("FATAL: Failed to create CameraTask!");
    // ESP.restart(); // Or handle error appropriately
  }
  
  // Create upload task on core 0
  xTaskCreatePinnedToCore(
    uploadTask,
    "UploadTask",
    8192,
    NULL,
    1,
    &uploadTaskHandle,
    0
  );
  if (uploadTaskHandle == NULL) {
    Serial.println("FATAL: Failed to create UploadTask!");
    // ESP.restart(); // Or handle error appropriately
  }
  
  // Create NTRIP task on core 1
  if (Config::ntrip.enabled) {
    xTaskCreatePinnedToCore(
      ntripClientTask,
      "NTRIPClient",
      8192,
      NULL,
      1,
      &ntripTaskHandle,
      1
    );
    if (ntripTaskHandle == NULL) {
      Serial.println("FATAL: Failed to create NTRIPClientTask!");
      // ESP.restart(); // Or handle error appropriately
    } else {
      Serial.println("NTRIP client initialized on core 1");
    }
  }
  
  Serial.println("\n=== System Ready ===");
  Serial.println("Press capture button to start/stop capture");
  Serial.println("Press upload button to upload photos to S3\n");
}

void loop() {
  // Update GPS Manager
  if (Config::gps.enabled) {
    GPSManager::update();
  }

  // Update Camera Mode Manager
  CameraModeManager::update();

  // Update MAVLink Manager
  MAVLinkManager::update();

  // Handle button inputs and state machine
  handleButtons();

  // Check landing mode timeout
  if (landingModeActive &&
      (millis() - landingModeStartTime > LANDING_MODE_TIMEOUT_MS)) {
    Serial.println("Landing mode timeout - powering down");
    // Could trigger a graceful shutdown here
    landingModeActive = false;
  }

  // Perform periodic health check
  static unsigned long lastHealthCheck = 0;
  if (millis() - lastHealthCheck > 30000) {
    performHealthCheck();
    lastHealthCheck = millis();
  }

  // Coordinate power management
  PowerManager::coordinatePowerManagement();

  // Small delay to prevent CPU hogging
  delay(10);
}

void initializeSystem() {
  // Initialize PSRAM first (critical for camera operations)
  if (!PSRAMManager::init()) {
    Serial.println("CRITICAL: PSRAM initialization failed!");
    Serial.println("Camera operations may fail at high resolutions");
    // Continue anyway for testing
  }

  // Initialize GPIO pins
  pinMode(Config::pins.CAPTURE_TRIGGER_PIN, INPUT_PULLUP);
  pinMode(Config::pins.UPLOAD_TRIGGER_PIN, INPUT_PULLUP);
  pinMode(Config::pins.LED_STATUS_PIN, OUTPUT);
  digitalWrite(Config::pins.LED_STATUS_PIN, LOW);

  // Initialize system state
  SystemState::init();

  // Initialize static EXIF GPS system
  if (!StaticEXIFGPS::init()) {
    Serial.println("WARNING: Static EXIF GPS initialization failed!");
  }
  
  // Initialize storage (SD card)
  if (!StorageManager::init()) {
    Serial.println("FATAL: Storage initialization failed!");
    ESP.restart();
  }
  
  // Load configuration from SD card if exists
  if (!Config::loadFromFile()) {
    Serial.println("WARNING: Failed to load configuration from file or critical settings are missing. System may not operate correctly.");
    // Depending on severity, could halt or use hardcoded critical defaults
  }
  
  // Initialize network (WiFi)
  if (!WiFiManager::connectWiFi()) {
    Serial.println("FATAL: WiFi connection failed!");
    ESP.restart();
  }
  
  // Initialize time
  WiFiManager::initializeTime();
  
  // Initialize GPS Manager
  if (Config::gps.enabled) {
    if (!GPSManager::init()) {
      Serial.println("WARNING: GPS Manager initialization failed!");
      // Continue without GPS - system can still work
    } else {
      Serial.println("GPS Manager initialized successfully");
    }
  }

  // Initialize Camera Mode Manager
  if (!CameraModeManager::init()) {
    Serial.println("WARNING: Camera Mode Manager initialization failed!");
  }

  // Initialize camera (starts in IDLE mode)
  if (!CameraManager::init()) {
    Serial.println("WARNING: Camera initialization failed!");
    // Continue without camera - NTRIP can still work
  }

  // Enable GPS geotagging if available
  if (Config::gps.enabled && Config::gps.enable_geotagging) {
    CameraManager::enableGeotagging(true);
    Serial.println("GPS geotagging enabled");
  }

  // Power down camera initially (will be in IDLE mode)
  if (Config::power.CAMERA_POWER_MANAGEMENT) {
    CameraManager::powerDown();
  }
  
  // Load upload tracking
  UploadManager::loadTracking();
  
  // Perform initial cleanup
  StorageManager::performCleanup();

  // Initialize MAVLink manager for landing target output
  if (MAVLinkManager::init()) {
    Serial.println("MAVLink manager initialized for landing target messages");
  } else {
    Serial.println("MAVLink manager initialization failed - landing target output disabled");
  }

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
    
    // Handle capture button (falling edge) - Camera Mode State Machine
    if (lastCaptureState == HIGH && currentCaptureState == LOW) {
      SystemState::updateActivity();

      CameraMode currentMode = CameraModeManager::getMode();

      switch (currentMode) {
        case Config::CAMERA_MODE_IDLE:
          // Transition: IDLE → MISSION (start capturing)
          Serial.println("State transition: IDLE → MISSION");
          if (CameraModeManager::setMode(Config::CAMERA_MODE_MISSION)) {
            // Signal camera task to start mission capture
            if (cameraTaskHandle) {
              xTaskNotify(cameraTaskHandle, 1, eSetValueWithOverwrite);
            }
          }
          break;

        case Config::CAMERA_MODE_MISSION:
          // Transition: MISSION → LANDING (stop capturing, start AprilTag detection)
          Serial.println("State transition: MISSION → LANDING");
          if (CameraModeManager::setMode(Config::CAMERA_MODE_LANDING)) {
            // Signal camera task to stop mission capture and start landing mode
            if (cameraTaskHandle) {
              xTaskNotify(cameraTaskHandle, 2, eSetValueWithOverwrite);
            }
            // Start landing mode timer
            landingModeStartTime = millis();
            landingModeActive = true;
          }
          break;

        case Config::CAMERA_MODE_LANDING:
          // No action in landing mode - will timeout or power down
          Serial.println("In landing mode - button press ignored");
          break;
      }
    }
    
    // Handle upload button (falling edge)
    if (lastUploadState == HIGH && currentUploadState == LOW) {
      SystemState::updateActivity();
      
      // Signal upload task
      if (uploadTaskHandle) {
        xTaskNotify(uploadTaskHandle, 1, eSetValueWithOverwrite);
      }
    }
    
    lastCaptureState = currentCaptureState;
    lastUploadState = currentUploadState;
  }
}

void cameraTask(void* parameter) {
  uint32_t notificationValue;

  while (true) {
    // Wait for notification with timeout
    if (xTaskNotifyWait(0, 0xFFFFFFFF, &notificationValue, pdMS_TO_TICKS(100))) {
      CameraMode currentMode = CameraModeManager::getMode();

      if (notificationValue == 1) {
        // Start mission capture (IDLE → MISSION transition)
        if (currentMode == Config::CAMERA_MODE_MISSION) {
          Serial.println("Starting mission capture (geotagged photos)");
          CameraManager::startCapture();
        }
      } else if (notificationValue == 2) {
        // Stop mission capture (MISSION → LANDING transition)
        if (currentMode == Config::CAMERA_MODE_LANDING) {
          Serial.println("Stopping mission capture, starting landing mode");
          CameraManager::stopCapture();
          // Landing mode will be handled in the continuous processing below
        }
      }
    }

    CameraMode currentMode = CameraModeManager::getMode();

    // Handle mode-specific continuous processing
    switch (currentMode) {
      case Config::CAMERA_MODE_MISSION:
        // Mission mode: Capture geotagged photos at configured interval
        if (SystemState::isCapturing() && CameraModeManager::shouldCapture()) {
          CameraModeManager::recordCaptureStart();

          // Use geotagged capture if GPS is available
          bool success;
          if (CameraManager::isGeotaggingEnabled()) {
            success = CameraManager::captureGeotaggedPhoto();
          } else {
            success = CameraManager::capturePhoto();
          }

          CameraModeManager::recordCaptureComplete(success);
          SystemState::setLastCaptureTime(millis());
        }
        break;

      case Config::CAMERA_MODE_LANDING:
        // Landing mode: Process frames for AprilTag detection (no file saving)
        if (CameraModeManager::shouldCapture()) {
          CameraModeManager::recordCaptureStart();

          // Get frame for AprilTag processing
          camera_fb_t *fb = esp_camera_fb_get();
          if (fb) {
            // Process frame for AprilTag detection
            int tags_detected = 0;
            if (AprilTagManager::isInitialized() && AprilTagManager::isEnabled()) {
              tags_detected = AprilTagManager::processFrame(fb);

              // Log detection results periodically
              static uint32_t frame_count = 0;
              frame_count++;
              if (frame_count % 50 == 0 || tags_detected > 0) {
                if (tags_detected > 0) {
                  AprilTagDetection detection = AprilTagManager::getLastDetection();
                  Serial.printf("LANDING: AprilTag ID=%d detected at (%.1f,%.1f), margin=%.2f\n",
                                detection.id, detection.center_x, detection.center_y,
                                detection.decision_margin);

                  // Generate MAVLink LANDING_TARGET message
                  if (MAVLinkManager::isInitialized() && MAVLinkManager::isEnabled()) {
                    MAVLinkManager::sendLandingTarget(detection);
                  }
                } else if (frame_count % 100 == 0) {
                  Serial.printf("LANDING: Processed %u frames, no tags detected\n", frame_count);
                }
              }
            }

            esp_camera_fb_return(fb);

            // Record processing for statistics (success if frame processed)
            CameraModeManager::recordCaptureComplete(true);
          } else {
            CameraModeManager::recordCaptureComplete(false);
          }
        }
        break;

      case Config::CAMERA_MODE_IDLE:
        // Idle mode: Do nothing
        break;
    }

    // Small delay to prevent task hogging
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void uploadTask(void* parameter) {
  uint32_t notificationValue;
  
  while (true) {
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
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("WiFi RSSI: %d dBm\n", WiFi.RSSI());

  // Check PSRAM status
  PSRAMManager::printMemoryStatus();
  
  // Check SD card
  if (!StorageManager::verifyCard()) {
    Serial.println("WARNING: SD card verification failed!");
  } else {
    uint64_t totalBytes, usedBytes;
    StorageManager::getSpaceInfo(totalBytes, usedBytes);
    Serial.printf("Storage: %.1f/%.1f GB used\n", 
                  usedBytes / 1024.0 / 1024.0 / 1024.0,
                  totalBytes / 1024.0 / 1024.0 / 1024.0);
  }
  
  // Check task status
  if (cameraTaskHandle) {
    Serial.printf("Camera task: %s\n", 
                  eTaskGetState(cameraTaskHandle) == eRunning ? "Running" : "Not running");
  }
  
  if (uploadTaskHandle) {
    Serial.printf("Upload task: %s\n", 
                  eTaskGetState(uploadTaskHandle) == eRunning ? "Running" : "Not running");
  }
  
  if (ntripTaskHandle) {
    Serial.printf("NTRIP task: %s\n",
                  eTaskGetState(ntripTaskHandle) == eRunning ? "Running" : "Not running");
    NtripClient::printStatistics();
  }

  // Check GPS status
  if (Config::gps.enabled) {
    Serial.printf("GPS: %s", GPSManager::hasValidFix() ? "Valid fix" : "No fix");
    if (GPSManager::hasValidFix()) {
      Serial.printf(" (%s, %d sats)",
                    GPSManager::getFixQualityString(GPSManager::getFixQuality()).c_str(),
                    GPSManager::getSatelliteCount());
    }
    Serial.println();
    if (CameraManager::isGeotaggingEnabled()) {
      Serial.println("Geotagging: Enabled");
    }
  }

  // Check camera mode
  CameraMode currentMode = CameraModeManager::getMode();
  Serial.printf("Camera Mode: %s", CameraModeManager::getModeString(currentMode));
  if (landingModeActive) {
    unsigned long remaining = (LANDING_MODE_TIMEOUT_MS - (millis() - landingModeStartTime)) / 1000;
    Serial.printf(" (timeout in %lu sec)", remaining);
  }
  Serial.println();

  Serial.println("-------------------\n");
}