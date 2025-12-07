#ifndef MAVLINK_HARDCODED_H
#define MAVLINK_HARDCODED_H

#include <Arduino.h>

/**
 * Hardcoded MAVLink Messages for ESP32 Precision Landing
 *
 * Implements only the essential messages needed for precision landing:
 * - HEARTBEAT (ID 0): Connection keepalive
 * - LANDING_TARGET (ID 149): AprilTag position for precision landing
 * - GPS_RTCM_DATA (ID 233): RTCM corrections (optional)
 *
 * DESIGN PRINCIPLE: Only send LANDING_TARGET messages when you have valid
 * AprilTag detection data. Don't send placeholder or default data - this
 * just wastes bandwidth and confuses the flight controller.
 *
 * Memory footprint: ~5KB vs 200-400KB for full MAVLink library
 * Perfect for resource-constrained ESP32 applications
 */

// MAVLink message IDs
#define MAVLINK_MSG_ID_HEARTBEAT 0
#define MAVLINK_MSG_ID_GPS_RAW_INT 24
#define MAVLINK_MSG_ID_GLOBAL_POSITION_INT 33
#define MAVLINK_MSG_ID_LANDING_TARGET 149
#define MAVLINK_MSG_ID_GPS_RTCM_DATA 233

// MAVLink constants
#define MAVLINK_STX 0xFE              // MAVLink v1 start byte
#define MAVLINK_MAX_PAYLOAD_LEN 255
#define MAVLINK_CHECKSUM_SEED 0xFFFF

// System/Component IDs
#define MAV_COMP_ID_CAMERA 100
#define MAV_COMP_ID_AUTOPILOT1 1

// Landing target types
#define LANDING_TARGET_TYPE_VISION_FIDUCIAL 2  // AprilTags

// MAVLink frame coordinate systems
#define MAV_FRAME_BODY_FRD 12  // Body fixed frame, Z-down

// Autopilot types and states
#define MAV_AUTOPILOT_GENERIC 0
#define MAV_STATE_ACTIVE 4

// Message structures (packed for exact byte layout)
struct __attribute__((packed)) mavlink_heartbeat_t {
    uint32_t custom_mode;      // Custom mode (autopilot-specific)
    uint8_t type;              // Type of system (MAV_TYPE_CAMERA = 30)
    uint8_t autopilot;         // Autopilot type (MAV_AUTOPILOT_GENERIC = 0)
    uint8_t base_mode;         // System mode bitfield
    uint8_t system_status;     // System status flag
    uint8_t mavlink_version;   // MAVLink version (3 for MAVLink 1.0)
};

struct __attribute__((packed)) mavlink_landing_target_t {
    uint64_t time_usec;        // Timestamp (microseconds since UNIX epoch)
    float angle_x;             // X-axis angular offset (radians)
    float angle_y;             // Y-axis angular offset (radians)
    float distance;            // Distance to target (meters)
    float size_x;              // Size in x-direction (radians)
    float size_y;              // Size in y-direction (radians)
    uint8_t target_num;        // Landing target number (AprilTag ID)
    uint8_t frame;             // Coordinate frame (MAV_FRAME_*)
    uint8_t type;              // Landing target type
    uint8_t position_valid;    // Position fields are valid
    float x;                   // X position (meters, if position_valid)
    float y;                   // Y position (meters, if position_valid)
    float z;                   // Z position (meters, if position_valid)
    float q[4];                // Quaternion (if orientation valid)
};

struct __attribute__((packed)) mavlink_gps_rtcm_data_t {
    uint8_t flags;             // LSB: fragmentation flag
    uint8_t len;               // Data length
    uint8_t data[180];         // RTCM message data (max 180 bytes)
};

struct __attribute__((packed)) mavlink_gps_raw_int_t {
    uint64_t time_usec;        // Timestamp (microseconds since UNIX epoch)
    int32_t lat;               // Latitude (degE7)
    int32_t lon;               // Longitude (degE7)
    int32_t alt;               // Altitude (millimeters above MSL)
    uint16_t eph;              // GPS HDOP horizontal dilution of position
    uint16_t epv;              // GPS VDOP vertical dilution of position
    uint16_t vel;              // GPS ground speed (cm/s)
    uint16_t cog;              // Course over ground (cdeg)
    uint8_t fix_type;          // GPS fix type
    uint8_t satellites_visible; // Number of satellites visible
    int32_t alt_ellipsoid;     // Altitude (above WGS84 ellipsoid)
    uint32_t h_acc;            // Position accuracy (mm)
    uint32_t v_acc;            // Altitude accuracy (mm)
    uint32_t vel_acc;          // Speed accuracy (mm/s)
    uint32_t hdg_acc;          // Heading accuracy (degE5)
    uint16_t yaw;              // Yaw angle (cdeg)
};

struct __attribute__((packed)) mavlink_global_position_int_t {
    uint32_t time_boot_ms;     // Timestamp (milliseconds since system boot)
    int32_t lat;               // Latitude (degE7)
    int32_t lon;               // Longitude (degE7)
    int32_t alt;               // Altitude above MSL (millimeters)
    int32_t relative_alt;      // Altitude above ground (millimeters)
    int16_t vx;                // Ground X speed (cm/s)
    int16_t vy;                // Ground Y speed (cm/s)
    int16_t vz;                // Ground Z speed (cm/s)
    uint16_t hdg;              // Vehicle heading (cdeg)
};

// MAVLink message header
struct __attribute__((packed)) mavlink_header_t {
    uint8_t stx;               // Start of text (0xFE for MAVLink v1)
    uint8_t len;               // Payload length
    uint8_t seq;               // Packet sequence
    uint8_t sysid;             // System ID
    uint8_t compid;            // Component ID
    uint8_t msgid;             // Message ID
};

// Complete MAVLink message structure
struct __attribute__((packed)) mavlink_message_t {
    mavlink_header_t header;
    uint8_t payload[MAVLINK_MAX_PAYLOAD_LEN];
    uint8_t ck_a;              // Checksum byte A
    uint8_t ck_b;              // Checksum byte B
};

class HardcodedMAVLink {
private:
    static HardwareSerial* serial_port;
    static uint8_t system_id;
    static uint8_t component_id;
    static uint8_t sequence;
    static unsigned long last_heartbeat;

    // Checksum calculation
    static void crc_accumulate(uint8_t data, uint16_t* crcAccum);
    static uint16_t crc_calculate(const uint8_t* buffer, uint16_t length);

    // Message packing
    static uint16_t pack_heartbeat(uint8_t* buffer, uint8_t sysid, uint8_t compid);
    static uint16_t pack_landing_target(uint8_t* buffer, uint8_t sysid, uint8_t compid,
                                      const mavlink_landing_target_t* landing_target);
    static uint16_t pack_gps_rtcm_data(uint8_t* buffer, uint8_t sysid, uint8_t compid,
                                     const mavlink_gps_rtcm_data_t* rtcm_data);

    // Generic message packing
    static uint16_t pack_message(uint8_t* buffer, uint8_t sysid, uint8_t compid, uint8_t msgid,
                               const void* payload, uint8_t payload_len);

    // Helper function for rotation matrix to quaternion conversion
    static void rotationMatrixToQuaternion(const float R[3][3], float q[4]);

public:
    /**
     * Initialize hardcoded MAVLink communication
     */
    static bool init(HardwareSerial* serial, uint8_t sys_id = 1, uint8_t comp_id = MAV_COMP_ID_CAMERA);

    /**
     * Update function - sends periodic heartbeat
     */
    static void update();

    /**
     * Send heartbeat message
     */
    static bool sendHeartbeat(uint8_t type = 30, uint8_t autopilot = MAV_AUTOPILOT_GENERIC);

    /**
     * Send landing target message for AprilTag detection
     */
    static bool sendLandingTarget(uint8_t target_id, float angle_x, float angle_y, float distance,
                                float size_x = 0.1f, float size_y = 0.1f, uint8_t frame = MAV_FRAME_BODY_FRD);

    /**
     * Send landing target with position data
     */
    static bool sendLandingTargetWithPosition(uint8_t target_id, float angle_x, float angle_y, float distance,
                                            float x, float y, float z, float size_x = 0.1f, float size_y = 0.1f);

    /**
     * Send landing target with orientation data from AprilTag detection
     * Only call when you have valid pose estimation from AprilTag processing!
     */
    static bool sendLandingTargetWithOrientation(uint8_t target_id, float angle_x, float angle_y, float distance,
                                               float size_x, float size_y, const float rotation_matrix[3][3],
                                               uint8_t frame = MAV_FRAME_BODY_FRD);

    /**
     * Send complete landing target with position and orientation data
     * Only call when you have full pose estimation from AprilTag processing!
     */
    static bool sendLandingTargetFull(uint8_t target_id, float angle_x, float angle_y, float distance,
                                    float x, float y, float z, float size_x, float size_y,
                                    const float rotation_matrix[3][3]);

    /**
     * Send RTCM correction data
     */
    static bool sendRTCMData(const uint8_t* data, uint8_t length, bool fragmented = false);

    /**
     * Get statistics
     */
    static uint32_t getMessagesSent();
    static void printStatistics();

    /**
     * Message decoding functions
     */
    static bool parseMessage(const uint8_t* buffer, uint16_t length, uint8_t* msg_id, void* payload);
    static bool parseGPSRawInt(const uint8_t* payload, mavlink_gps_raw_int_t* gps_raw);
    static bool parseGlobalPositionInt(const uint8_t* payload, mavlink_global_position_int_t* global_pos);

    /**
     * Test functions
     */
    static void sendTestMessages();
    static void testMessageDecoding();
};

#endif // MAVLINK_HARDCODED_H