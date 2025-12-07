#ifndef CAMERA_MODE_MANAGER_H
#define CAMERA_MODE_MANAGER_H

#include <Arduino.h>
#include "esp_camera.h"
#include "config.h"

// Use CameraMode from Config namespace
using CameraMode = Config::CameraMode;

// Forward declarations
class CameraManager;

/**
 * Camera Mode Manager
 *
 * Manages switching between different camera operation modes:
 * - MISSION: High-resolution UXGA JPEG capture for mapping/inspection
 * - LANDING: Real-time VGA grayscale capture for AprilTag detection
 * - IDLE: Camera powered down or standby
 *
 * Handles dynamic reconfiguration of camera settings including:
 * - Resolution (UXGA vs VGA)
 * - Pixel format (JPEG vs Grayscale)
 * - Capture rate (2-5Hz vs 10-15Hz)
 * - Quality settings
 */
class CameraModeManager {
private:
    static CameraMode currentMode;
    static CameraMode requestedMode;
    static bool modeChangeInProgress;
    static unsigned long lastModeChange;
    static unsigned long lastCaptureTime;
    static bool autoSwitchEnabled;

    // Mode-specific capture statistics
    struct ModeStats {
        uint32_t captureCount;
        uint32_t frameDrops;
        uint32_t avgCaptureTime;
        uint32_t maxCaptureTime;
        unsigned long totalCaptureTime;
        unsigned long lastStatsReset;

        void reset() {
            captureCount = 0;
            frameDrops = 0;
            avgCaptureTime = 0;
            maxCaptureTime = 0;
            totalCaptureTime = 0;
            lastStatsReset = millis();
        }
    };

    static ModeStats missionStats;
    static ModeStats landingStats;
    static ModeStats* getCurrentStats();

    // Mode configuration helpers
    static camera_config_t getMissionCameraConfig();
    static camera_config_t getLandingCameraConfig();
    static bool reconfigureCamera(const camera_config_t& config);

    // Performance monitoring
    static void updateCaptureStats(unsigned long captureTime);
    static bool checkPerformanceRequirements();

public:
    // Initialization and control
    static bool init();
    static void update();
    static void reset();

    // Mode management
    static bool setMode(CameraMode mode);
    static CameraMode getMode() { return currentMode; }
    static bool isModeChangeInProgress() { return modeChangeInProgress; }
    static bool requestModeChange(CameraMode mode);

    // Mode-specific operations
    static bool shouldCapture();
    static uint32_t getCaptureInterval();
    static framesize_t getFrameSize();
    static pixformat_t getPixelFormat();
    static uint8_t getJPEGQuality();

    // Performance and statistics
    static void recordCaptureStart();
    static void recordCaptureComplete(bool success);
    static ModeStats getMissionStats() { return missionStats; }
    static ModeStats getLandingStats() { return landingStats; }
    static void printStats();
    static void resetStats();

    // Auto-switching logic
    static void enableAutoSwitch(bool enable) { autoSwitchEnabled = enable; }
    static bool isAutoSwitchEnabled() { return autoSwitchEnabled; }
    static void checkAutoSwitch();

    // Utility functions
    static const char* getModeString(CameraMode mode);
    static bool isMissionMode() { return currentMode == Config::CAMERA_MODE_MISSION; }
    static bool isLandingMode() { return currentMode == Config::CAMERA_MODE_LANDING; }
    static bool isIdleMode() { return currentMode == Config::CAMERA_MODE_IDLE; }

    // Integration with existing camera manager
    static void onCameraInitialized();
    static void onCameraDeinitialized();
    static bool validateModeRequirements(CameraMode mode);

    // Configuration management
    static void updateConfiguration();
    static bool saveModeSettings();
    static bool loadModeSettings();

    // Debug and diagnostics
    static void printCurrentConfig();
    static void enableDebugOutput(bool enable);
};

// Mode switching callback type
typedef void (*ModeChangeCallback)(CameraMode oldMode, CameraMode newMode);

// Global mode change notification system
extern ModeChangeCallback g_modeChangeCallback;
void setModeChangeCallback(ModeChangeCallback callback);

#endif // CAMERA_MODE_MANAGER_H