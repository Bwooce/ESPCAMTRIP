#ifndef MAVLINK_MANAGER_H
#define MAVLINK_MANAGER_H

#include <Arduino.h>

/**
 * MAVLink Manager for ESP32-S3 Camera System
 *
 * Handles MAVLink message generation and transmission for:
 * - LANDING_TARGET messages from AprilTag detections
 * - RTCM GPS corrections (existing functionality)
 * - System status and telemetry
 *
 * Operates on GPIO8 (MAVLink TX) for flight controller communication
 */

// Conditional compilation for MAVLink
#ifndef MAVLINK_DISABLED

// Forward declarations for MAVLink structures
struct AprilTagDetection;

// MAVLink message IDs
#define MAVLINK_MSG_ID_LANDING_TARGET 149

// Landing target types (MAVLink standard)
typedef enum {
    LANDING_TARGET_TYPE_LIGHT_BEACON = 0,
    LANDING_TARGET_TYPE_RADIO_BEACON = 1,
    LANDING_TARGET_TYPE_VISION_FIDUCIAL = 2,  // AprilTags
    LANDING_TARGET_TYPE_VISION_OTHER = 3
} landing_target_type_t;

struct LandingTargetData {
    uint8_t target_num;        // Target ID (AprilTag ID)
    uint8_t frame;            // Coordinate frame (MAV_FRAME_BODY_FRD = 12)
    float angle_x;            // Horizontal angle (radians, positive right)
    float angle_y;            // Vertical angle (radians, positive up)
    float distance;           // Distance to target (meters)
    float size_x;             // Target size X (radians)
    float size_y;             // Target size Y (radians)
    float pos_x;              // X position (meters, if available)
    float pos_y;              // Y position (meters, if available)
    float pos_z;              // Z position (meters, if available)
    bool position_valid;      // True if pos_x/y/z are valid
};

class MAVLinkManager {
private:
    static bool initialized;
    static bool enabled;
    static HardwareSerial* mavlink_serial;
    static uint8_t system_id;
    static uint8_t component_id;
    static uint8_t target_system;
    static uint8_t target_component;

    // Message sequence tracking
    static uint8_t sequence;

    // Statistics
    static uint32_t messages_sent;
    static uint32_t landing_targets_sent;
    static unsigned long last_heartbeat;

    // Internal functions
    static void sendHeartbeat();
    static void sendMessage(uint8_t* buffer, uint16_t length);
    static uint16_t generateChecksum(uint8_t* data, uint16_t length);

public:
    /**
     * Initialize MAVLink communication system
     * Sets up UART and configures MAVLink parameters
     */
    static bool init();

    /**
     * Deinitialize MAVLink system
     */
    static void deinit();

    /**
     * Check if MAVLink is initialized and enabled
     */
    static bool isInitialized() { return initialized; }

    /**
     * Enable/disable MAVLink transmission
     */
    static void setEnabled(bool enable) { enabled = enable; }
    static bool isEnabled() { return enabled; }

    /**
     * Update function - call regularly to handle periodic messages
     * Sends heartbeat and manages connection state
     */
    static void update();

    /**
     * Send LANDING_TARGET message for AprilTag detection
     * Converts AprilTag detection to MAVLink landing target format
     *
     * @param detection AprilTag detection result
     * @return True if message sent successfully
     */
    static bool sendLandingTarget(const AprilTagDetection& detection);

    /**
     * Send LANDING_TARGET message with custom data
     * For advanced control or custom target types
     *
     * @param target_data Landing target data
     * @return True if message sent successfully
     */
    static bool sendLandingTarget(const LandingTargetData& target_data);

    /**
     * Configuration functions
     */
    static void setSystemID(uint8_t id) { system_id = id; }
    static void setComponentID(uint8_t id) { component_id = id; }
    static void setTargetSystem(uint8_t id) { target_system = id; }

    /**
     * Get transmission statistics
     */
    static uint32_t getMessagesSent() { return messages_sent; }
    static uint32_t getLandingTargetsSent() { return landing_targets_sent; }
    static void printStatistics();

    /**
     * Direct message transmission for RTCM data
     * Maintains compatibility with existing NTRIP client
     */
    static bool sendRTCMData(const uint8_t* rtcm_data, uint16_t length);

    /**
     * Test functions for development
     */
    static bool testMAVLinkConnection();
    static void sendTestLandingTarget();
};

#else
// Stub class when MAVLink is disabled
class MAVLinkManager {
public:
    static bool init() {
        Serial.println("MAVLink disabled at compile time");
        return false;
    }
    static void deinit() {}
    static bool isInitialized() { return false; }
    static void setEnabled(bool enable) {}
    static bool isEnabled() { return false; }
    static void update() {}
    static bool sendLandingTarget(const AprilTagDetection& detection) { return false; }
    static bool sendRTCMData(const uint8_t* rtcm_data, uint16_t length) { return false; }
    static void printStatistics() {
        Serial.println("MAVLink: Disabled");
    }
};
#endif // MAVLINK_DISABLED

#endif // MAVLINK_MANAGER_H