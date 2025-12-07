#include "camera_mode_manager.h"
#include "camera_manager.h"
#include "config.h"
#include "system_state.h"
#include "apriltag_manager.h"
#include <esp_camera.h>

// Static member definitions
CameraMode CameraModeManager::currentMode = Config::CAMERA_MODE_IDLE;
CameraMode CameraModeManager::requestedMode = Config::CAMERA_MODE_IDLE;
bool CameraModeManager::modeChangeInProgress = false;
unsigned long CameraModeManager::lastModeChange = 0;
unsigned long CameraModeManager::lastCaptureTime = 0;
bool CameraModeManager::autoSwitchEnabled = true;

CameraModeManager::ModeStats CameraModeManager::missionStats;
CameraModeManager::ModeStats CameraModeManager::landingStats;

// Global mode change callback
ModeChangeCallback g_modeChangeCallback = nullptr;

bool CameraModeManager::init() {
    Serial.println("Initializing Camera Mode Manager...");

    // Initialize statistics
    missionStats.reset();
    landingStats.reset();

    // Set initial mode based on configuration
    currentMode = Config::cameraMode.current_mode;
    requestedMode = currentMode;
    modeChangeInProgress = false;
    lastModeChange = millis();
    lastCaptureTime = 0;
    autoSwitchEnabled = Config::cameraMode.auto_switch_enabled;

    // Initialize AprilTag manager for landing mode
    if (Config::apriltag.enabled) {
        if (AprilTagManager::init()) {
            Serial.println("AprilTag manager initialized for landing mode");
        } else {
            Serial.println("AprilTag manager initialization failed - landing mode will have limited functionality");
        }
    }

    Serial.printf("Camera Mode Manager initialized, starting in %s mode\n",
                  getModeString(currentMode));
    return true;
}

void CameraModeManager::update() {
    // Handle pending mode changes
    if (modeChangeInProgress && (millis() - lastModeChange > 1000)) {
        // Mode change timeout, reset
        modeChangeInProgress = false;
        Serial.println("Mode change timeout, reverting to current mode");
    }

    // Process requested mode changes
    if (requestedMode != currentMode && !modeChangeInProgress) {
        if (setMode(requestedMode)) {
            Serial.printf("Mode changed: %s -> %s\n",
                         getModeString(currentMode), getModeString(requestedMode));
        }
    }

    // Check auto-switching conditions
    if (autoSwitchEnabled) {
        checkAutoSwitch();
    }

    // Update performance statistics
    updateCaptureStats(0); // Called without capture time for periodic updates
}

bool CameraModeManager::setMode(CameraMode mode) {
    if (mode == currentMode) {
        return true; // Already in requested mode
    }

    if (modeChangeInProgress) {
        Serial.println("Mode change already in progress");
        return false;
    }

    if (!validateModeRequirements(mode)) {
        Serial.printf("Mode requirements not met for %s\n", getModeString(mode));
        return false;
    }

    Serial.printf("Changing camera mode: %s -> %s\n",
                  getModeString(currentMode), getModeString(mode));

    modeChangeInProgress = true;
    CameraMode oldMode = currentMode;

    // Get camera configuration for new mode
    camera_config_t config;
    switch (mode) {
        case Config::CAMERA_MODE_MISSION:
            config = getMissionCameraConfig();
            break;
        case Config::CAMERA_MODE_LANDING:
            config = getLandingCameraConfig();
            break;
        case Config::CAMERA_MODE_IDLE:
            // For idle mode, we'll power down the camera
            if (CameraManager::isInitialized()) {
                CameraManager::powerDown();
            }
            currentMode = mode;
            modeChangeInProgress = false;
            lastModeChange = millis();

            // Notify callback
            if (g_modeChangeCallback) {
                g_modeChangeCallback(oldMode, mode);
            }
            return true;
        default:
            modeChangeInProgress = false;
            return false;
    }

    // Reconfigure camera
    if (!reconfigureCamera(config)) {
        Serial.println("Failed to reconfigure camera");
        modeChangeInProgress = false;
        return false;
    }

    // Update mode and timing
    currentMode = mode;
    lastModeChange = millis();
    lastCaptureTime = 0; // Reset capture timing
    modeChangeInProgress = false;

    // Update configuration
    Config::cameraMode.current_mode = mode;

    // Notify callback
    if (g_modeChangeCallback) {
        g_modeChangeCallback(oldMode, mode);
    }

    Serial.printf("Camera mode changed successfully to %s\n", getModeString(mode));
    return true;
}

bool CameraModeManager::requestModeChange(CameraMode mode) {
    if (mode == currentMode) {
        return true;
    }

    requestedMode = mode;
    Serial.printf("Mode change requested: %s\n", getModeString(mode));
    return true;
}

bool CameraModeManager::shouldCapture() {
    if (currentMode == Config::CAMERA_MODE_IDLE) {
        return false;
    }

    unsigned long now = millis();
    uint32_t interval = getCaptureInterval();

    return (now - lastCaptureTime) >= interval;
}

uint32_t CameraModeManager::getCaptureInterval() {
    switch (currentMode) {
        case Config::CAMERA_MODE_MISSION:
            return Config::cameraMode.MISSION_CAPTURE_INTERVAL;
        case Config::CAMERA_MODE_LANDING:
            return Config::cameraMode.LANDING_CAPTURE_INTERVAL;
        default:
            return 1000; // Default 1 second
    }
}

framesize_t CameraModeManager::getFrameSize() {
    switch (currentMode) {
        case Config::CAMERA_MODE_MISSION:
            return Config::cameraMode.MISSION_FRAME_SIZE;
        case Config::CAMERA_MODE_LANDING:
            return Config::cameraMode.LANDING_FRAME_SIZE;
        default:
            return FRAMESIZE_VGA;
    }
}

pixformat_t CameraModeManager::getPixelFormat() {
    switch (currentMode) {
        case Config::CAMERA_MODE_MISSION:
            return Config::cameraMode.MISSION_PIXEL_FORMAT;
        case Config::CAMERA_MODE_LANDING:
            return Config::cameraMode.LANDING_PIXEL_FORMAT;
        default:
            return PIXFORMAT_JPEG;
    }
}

uint8_t CameraModeManager::getJPEGQuality() {
    switch (currentMode) {
        case Config::CAMERA_MODE_MISSION:
            return Config::cameraMode.MISSION_JPEG_QUALITY;
        case Config::CAMERA_MODE_LANDING:
            return Config::cameraMode.LANDING_JPEG_QUALITY;
        default:
            return 10;
    }
}

camera_config_t CameraModeManager::getMissionCameraConfig() {
    camera_config_t config;

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Config::cameraPins.Y2_GPIO_NUM;
    config.pin_d1 = Config::cameraPins.Y3_GPIO_NUM;
    config.pin_d2 = Config::cameraPins.Y4_GPIO_NUM;
    config.pin_d3 = Config::cameraPins.Y5_GPIO_NUM;
    config.pin_d4 = Config::cameraPins.Y6_GPIO_NUM;
    config.pin_d5 = Config::cameraPins.Y7_GPIO_NUM;
    config.pin_d6 = Config::cameraPins.Y8_GPIO_NUM;
    config.pin_d7 = Config::cameraPins.Y9_GPIO_NUM;
    config.pin_xclk = Config::cameraPins.XCLK_GPIO_NUM;
    config.pin_pclk = Config::cameraPins.PCLK_GPIO_NUM;
    config.pin_vsync = Config::cameraPins.VSYNC_GPIO_NUM;
    config.pin_href = Config::cameraPins.HREF_GPIO_NUM;
    config.pin_sccb_sda = Config::cameraPins.SIOD_GPIO_NUM;
    config.pin_sccb_scl = Config::cameraPins.SIOC_GPIO_NUM;
    config.pin_pwdn = Config::cameraPins.PWDN_GPIO_NUM;
    config.pin_reset = Config::cameraPins.RESET_GPIO_NUM;
    config.xclk_freq_hz = Config::camera.XCLK_FREQ;
    config.pixel_format = Config::cameraMode.MISSION_PIXEL_FORMAT;
    config.frame_size = Config::cameraMode.MISSION_FRAME_SIZE;
    config.jpeg_quality = Config::cameraMode.MISSION_JPEG_QUALITY;
    config.fb_count = 1; // Single buffer for mission mode

    return config;
}

camera_config_t CameraModeManager::getLandingCameraConfig() {
    camera_config_t config;

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Config::cameraPins.Y2_GPIO_NUM;
    config.pin_d1 = Config::cameraPins.Y3_GPIO_NUM;
    config.pin_d2 = Config::cameraPins.Y4_GPIO_NUM;
    config.pin_d3 = Config::cameraPins.Y5_GPIO_NUM;
    config.pin_d4 = Config::cameraPins.Y6_GPIO_NUM;
    config.pin_d5 = Config::cameraPins.Y7_GPIO_NUM;
    config.pin_d6 = Config::cameraPins.Y8_GPIO_NUM;
    config.pin_d7 = Config::cameraPins.Y9_GPIO_NUM;
    config.pin_xclk = Config::cameraPins.XCLK_GPIO_NUM;
    config.pin_pclk = Config::cameraPins.PCLK_GPIO_NUM;
    config.pin_vsync = Config::cameraPins.VSYNC_GPIO_NUM;
    config.pin_href = Config::cameraPins.HREF_GPIO_NUM;
    config.pin_sccb_sda = Config::cameraPins.SIOD_GPIO_NUM;
    config.pin_sccb_scl = Config::cameraPins.SIOC_GPIO_NUM;
    config.pin_pwdn = Config::cameraPins.PWDN_GPIO_NUM;
    config.pin_reset = Config::cameraPins.RESET_GPIO_NUM;
    config.xclk_freq_hz = Config::camera.XCLK_FREQ;
    config.pixel_format = Config::cameraMode.LANDING_PIXEL_FORMAT;
    config.frame_size = Config::cameraMode.LANDING_FRAME_SIZE;
    config.jpeg_quality = Config::cameraMode.LANDING_JPEG_QUALITY;
    config.fb_count = 2; // Dual buffer for landing mode (higher rate)

    return config;
}

bool CameraModeManager::reconfigureCamera(const camera_config_t& config) {
    // Deinitialize current camera if needed
    if (CameraManager::isInitialized()) {
        CameraManager::deinit();
        delay(100); // Allow time for cleanup
    }

    // Initialize with new configuration
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera reconfiguration failed with error 0x%x\n", err);
        return false;
    }

    // Apply mode-specific settings
    sensor_t * s = esp_camera_sensor_get();
    if (s) {
        if (currentMode == Config::CAMERA_MODE_LANDING) {
            // Landing mode optimizations
            s->set_brightness(s, 0);
            s->set_contrast(s, 0);
            s->set_saturation(s, 0); // Not relevant for grayscale
            s->set_exposure_ctrl(s, 1);
            s->set_gain_ctrl(s, 1);
            s->set_agc_gain(s, 0);
        } else {
            // Mission mode optimizations
            s->set_brightness(s, 0);
            s->set_contrast(s, 0);
            s->set_saturation(s, 0);
            s->set_whitebal(s, 1);
            s->set_awb_gain(s, 1);
            s->set_exposure_ctrl(s, 1);
            s->set_gain_ctrl(s, 1);
        }
    }

    return true;
}

void CameraModeManager::recordCaptureStart() {
    lastCaptureTime = millis();
}

void CameraModeManager::recordCaptureComplete(bool success) {
    unsigned long captureTime = millis() - lastCaptureTime;

    ModeStats* stats = getCurrentStats();
    if (stats) {
        if (success) {
            stats->captureCount++;
            stats->totalCaptureTime += captureTime;
            stats->avgCaptureTime = stats->totalCaptureTime / stats->captureCount;
            if (captureTime > stats->maxCaptureTime) {
                stats->maxCaptureTime = captureTime;
            }
        } else {
            stats->frameDrops++;
        }
    }
}

CameraModeManager::ModeStats* CameraModeManager::getCurrentStats() {
    switch (currentMode) {
        case Config::CAMERA_MODE_MISSION:
            return &missionStats;
        case Config::CAMERA_MODE_LANDING:
            return &landingStats;
        default:
            return nullptr;
    }
}

void CameraModeManager::updateCaptureStats(unsigned long captureTime) {
    // Periodic statistics update - could be expanded for performance monitoring
}

void CameraModeManager::checkAutoSwitch() {
    // Placeholder for auto-switching logic
    // Could switch based on:
    // - Flight mode from ArduPilot
    // - Altitude
    // - User button press
    // - Time-based scheduling
    // - AprilTag detection requirements
}

bool CameraModeManager::validateModeRequirements(CameraMode mode) {
    switch (mode) {
        case Config::CAMERA_MODE_MISSION:
            // Check if storage is available
            return true; // Basic validation for now
        case Config::CAMERA_MODE_LANDING:
            // Check if AprilTag detection is enabled
            return Config::apriltag.enabled;
        case Config::CAMERA_MODE_IDLE:
            return true;
        default:
            return false;
    }
}

const char* CameraModeManager::getModeString(CameraMode mode) {
    switch (mode) {
        case Config::CAMERA_MODE_MISSION:
            return "MISSION";
        case Config::CAMERA_MODE_LANDING:
            return "LANDING";
        case Config::CAMERA_MODE_IDLE:
            return "IDLE";
        default:
            return "UNKNOWN";
    }
}

void CameraModeManager::printStats() {
    Serial.println("\n--- Camera Mode Statistics ---");
    Serial.printf("Current Mode: %s\n", getModeString(currentMode));

    Serial.println("\nMission Mode:");
    Serial.printf("  Captures: %lu, Drops: %lu\n",
                  missionStats.captureCount, missionStats.frameDrops);
    Serial.printf("  Avg/Max time: %lu/%lu ms\n",
                  missionStats.avgCaptureTime, missionStats.maxCaptureTime);

    Serial.println("\nLanding Mode:");
    Serial.printf("  Captures: %lu, Drops: %lu\n",
                  landingStats.captureCount, landingStats.frameDrops);
    Serial.printf("  Avg/Max time: %lu/%lu ms\n",
                  landingStats.avgCaptureTime, landingStats.maxCaptureTime);

    Serial.println("-----------------------------\n");
}

void CameraModeManager::resetStats() {
    missionStats.reset();
    landingStats.reset();
    Serial.println("Camera mode statistics reset");
}

void CameraModeManager::printCurrentConfig() {
    Serial.printf("\n--- Current Camera Configuration ---\n");
    Serial.printf("Mode: %s\n", getModeString(currentMode));
    Serial.printf("Frame Size: %dx%d\n",
                  getFrameSize() == FRAMESIZE_UXGA ? 1600 : 640,
                  getFrameSize() == FRAMESIZE_UXGA ? 1200 : 480);
    Serial.printf("Pixel Format: %s\n",
                  getPixelFormat() == PIXFORMAT_JPEG ? "JPEG" : "GRAYSCALE");
    Serial.printf("Capture Interval: %lu ms\n", getCaptureInterval());
    if (getPixelFormat() == PIXFORMAT_JPEG) {
        Serial.printf("JPEG Quality: %d\n", getJPEGQuality());
    }
    Serial.println("----------------------------------\n");
}

void CameraModeManager::reset() {
    currentMode = Config::CAMERA_MODE_IDLE;
    requestedMode = Config::CAMERA_MODE_IDLE;
    modeChangeInProgress = false;
    lastModeChange = 0;
    lastCaptureTime = 0;
    resetStats();
}

void setModeChangeCallback(ModeChangeCallback callback) {
    g_modeChangeCallback = callback;
}