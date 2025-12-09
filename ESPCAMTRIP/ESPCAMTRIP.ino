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

// Raw RTCM baud rate (only used when RTCM_OUTPUT_RAW is defined)
// ZED-F9P defaults to 38400 on UART1/UART2 - change to match your receiver
// or configure your receiver to 115200 via u-center
#define RTCM_RAW_BAUD_RATE 115200

// ============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
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
#endif
#include "esp_camera.h"

// Task handles
TaskHandle_t cameraTaskHandle = NULL;
TaskHandle_t uploadTaskHandle = NULL;
TaskHandle_t ntripTaskHandle = NULL;

// Button debouncing
unsigned long lastDebounceTime = 0;
bool lastCaptureState = HIGH;
bool lastUploadState = HIGH;

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
  // Handle button inputs
  handleButtons();
  
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
  // Initialize GPIO pins
  pinMode(Config::pins.CAPTURE_TRIGGER_PIN, INPUT_PULLUP);
  pinMode(Config::pins.UPLOAD_TRIGGER_PIN, INPUT_PULLUP);
  pinMode(Config::pins.LED_STATUS_PIN, OUTPUT);
  digitalWrite(Config::pins.LED_STATUS_PIN, LOW);
  
  // Initialize system state
  SystemState::init();
  
  // Initialize storage (SD card) - Optional
  if (!StorageManager::init()) {
    Serial.println("WARNING: SD card initialization failed!");
    Serial.println("System will continue without SD card storage.");
    Serial.println("Photos will not be saved locally, only uploaded to S3.");
    // Continue without SD card - system can still work for NTRIP and direct upload
  } else {
    Serial.println("SD card storage available for local photo backup");
  }
  
  // Load configuration from SD card if exists
  if (!Config::loadFromFile()) {
    Serial.println("WARNING: Failed to load configuration from file or critical settings are missing. System may not operate correctly.");
    // Depending on severity, could halt or use hardcoded critical defaults
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
  
  // Initialize camera
  if (!CameraManager::init()) {
    Serial.println("WARNING: Camera initialization failed!");
    // Continue without camera - NTRIP can still work
  }
  
  // Power down camera initially
  if (Config::power.CAMERA_POWER_MANAGEMENT) {
    CameraManager::powerDown();
  }
  
  // Load upload tracking
  UploadManager::loadTracking();
  
  // Perform initial cleanup
  StorageManager::performCleanup();
  
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
  // Initialize watchdog for camera task
  esp_task_wdt_add(NULL);
  Serial.println("Camera task started with watchdog protection");

  uint32_t notificationValue;

  while (true) {
    // Reset watchdog timer
    esp_task_wdt_reset();

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

void uploadTask(void* parameter) {
  // Initialize watchdog for upload task
  esp_task_wdt_add(NULL);
  Serial.println("Upload task started with watchdog protection");

  uint32_t notificationValue;

  while (true) {
    // Reset watchdog timer
    esp_task_wdt_reset();

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
  
  Serial.println("-------------------\n");
}