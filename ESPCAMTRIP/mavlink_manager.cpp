#include "mavlink_manager.h"
#include "mavlink_hardcoded.h"
#include "config.h"
#include "apriltag_manager.h"

// Use hardcoded MAVLink implementation
#define MAVLINK_HARDCODED_IMPL

// Static member definitions
bool MAVLinkManager::initialized = false;
bool MAVLinkManager::enabled = false;
HardwareSerial* MAVLinkManager::mavlink_serial = nullptr;
uint8_t MAVLinkManager::system_id = 1;
uint8_t MAVLinkManager::component_id = 191;  // MAV_COMP_ID_CAMERA
uint8_t MAVLinkManager::target_system = 1;
uint8_t MAVLinkManager::target_component = 1;
uint8_t MAVLinkManager::sequence = 0;
uint32_t MAVLinkManager::messages_sent = 0;
uint32_t MAVLinkManager::landing_targets_sent = 0;
unsigned long MAVLinkManager::last_heartbeat = 0;

bool MAVLinkManager::init() {
    Serial.println("Initializing MAVLink Manager (Hardcoded Implementation)...");

#ifdef MAVLINK_HARDCODED_IMPL
    // Check if MAVLink output is enabled in configuration
    if (!Config::mavlink.enabled) {
        Serial.println("MAVLink output disabled in configuration");
        return false;
    }

    // Initialize UART for MAVLink communication
    mavlink_serial = &Serial2;
    mavlink_serial->begin(57600, SERIAL_8N1, -1, Config::pins.MAVLINK_UART_TX);

    // Initialize hardcoded MAVLink
    if (!HardcodedMAVLink::init(mavlink_serial, system_id, component_id)) {
        Serial.println("Failed to initialize hardcoded MAVLink");
        return false;
    }

    initialized = true;
    enabled = true;

    Serial.printf("Hardcoded MAVLink initialized on GPIO%d\n", Config::pins.MAVLINK_UART_TX);
    Serial.printf("Memory footprint: ~5KB (vs 200-400KB full library)\n");
    Serial.printf("Supported messages: HEARTBEAT, LANDING_TARGET, GPS_RTCM_DATA\n");

    return true;
#else
    Serial.println("MAVLink hardcoded implementation not available");
    return false;
#endif
}

void MAVLinkManager::deinit() {
    if (initialized) {
        if (mavlink_serial) {
            mavlink_serial->end();
            mavlink_serial = nullptr;
        }
        initialized = false;
        enabled = false;
        Serial.println("MAVLink Manager deinitialized");
    }
}

void MAVLinkManager::update() {
    if (!initialized || !enabled) {
        return;
    }

#ifdef MAVLINK_HARDCODED_IMPL
    // Update hardcoded MAVLink (handles heartbeat automatically)
    HardcodedMAVLink::update();
#endif
}

bool MAVLinkManager::sendLandingTarget(const AprilTagDetection& detection) {
    if (!initialized || !enabled) {
        return false;
    }

#ifdef MAVLINK_HARDCODED_IMPL
    // Convert AprilTag detection to MAVLink coordinates
    float angle_x, angle_y, distance;
    if (!AprilTagManager::convertToMAVLinkTarget(detection, &angle_x, &angle_y, &distance)) {
        return false;
    }

    // Send landing target using hardcoded MAVLink
    if (detection.pose_valid) {
        // Send with position data
        return HardcodedMAVLink::sendLandingTargetWithPosition(
            detection.id, angle_x, angle_y, distance,
            detection.translation[0], detection.translation[1], detection.translation[2]);
    } else {
        // Send without position data
        return HardcodedMAVLink::sendLandingTarget(
            detection.id, angle_x, angle_y, distance);
    }
#else
    // Stub implementation when hardcoded MAVLink is disabled
    static int call_count = 0;
    if (++call_count % 10 == 1) {
        Serial.printf("MAVLink stub: Would send LANDING_TARGET for tag ID=%d\n", detection.id);
    }
    return false;
#endif
}

bool MAVLinkManager::sendLandingTarget(const LandingTargetData& target_data) {
    if (!initialized || !enabled || !mavlink_serial) {
        return false;
    }

#ifdef MAVLINK_BASIC_IMPL
    // Create simplified MAVLink message format
    // This is a basic implementation - full MAVLink would use proper message packing

    uint8_t message_buffer[128];
    uint16_t message_length = 0;

    // MAVLink v1 header format (simplified)
    message_buffer[0] = 0xFE; // STX (MAVLink v1)
    message_buffer[1] = 64;   // Payload length (LANDING_TARGET message)
    message_buffer[2] = sequence++; // Sequence
    message_buffer[3] = system_id; // System ID
    message_buffer[4] = component_id; // Component ID
    message_buffer[5] = MAVLINK_MSG_ID_LANDING_TARGET; // Message ID

    // LANDING_TARGET payload (64 bytes)
    uint16_t payload_index = 6;
    uint64_t time_usec = (uint64_t)millis() * 1000; // Convert to microseconds

    // Pack payload data (little-endian format)
    // time_usec (8 bytes)
    memcpy(&message_buffer[payload_index], &time_usec, 8);
    payload_index += 8;

    // angle_x (4 bytes float)
    memcpy(&message_buffer[payload_index], &target_data.angle_x, 4);
    payload_index += 4;

    // angle_y (4 bytes float)
    memcpy(&message_buffer[payload_index], &target_data.angle_y, 4);
    payload_index += 4;

    // distance (4 bytes float)
    memcpy(&message_buffer[payload_index], &target_data.distance, 4);
    payload_index += 4;

    // size_x (4 bytes float)
    memcpy(&message_buffer[payload_index], &target_data.size_x, 4);
    payload_index += 4;

    // size_y (4 bytes float)
    memcpy(&message_buffer[payload_index], &target_data.size_y, 4);
    payload_index += 4;

    // target_num (1 byte)
    message_buffer[payload_index++] = target_data.target_num;

    // frame (1 byte)
    message_buffer[payload_index++] = target_data.frame;

    // type (1 byte) - AprilTag is vision fiducial
    message_buffer[payload_index++] = LANDING_TARGET_TYPE_VISION_FIDUCIAL;

    // position_valid flag and position data (optional)
    if (target_data.position_valid) {
        memcpy(&message_buffer[payload_index], &target_data.pos_x, 4);
        payload_index += 4;
        memcpy(&message_buffer[payload_index], &target_data.pos_y, 4);
        payload_index += 4;
        memcpy(&message_buffer[payload_index], &target_data.pos_z, 4);
        payload_index += 4;
    } else {
        // Fill with zeros if position not valid
        memset(&message_buffer[payload_index], 0, 12);
        payload_index += 12;
    }

    // Pad remaining payload to 64 bytes
    while (payload_index < 70) { // 6 header + 64 payload
        message_buffer[payload_index++] = 0;
    }

    message_length = payload_index;

    // Calculate and append checksum (simplified)
    uint16_t checksum = generateChecksum(&message_buffer[1], message_length - 3);
    message_buffer[message_length++] = checksum & 0xFF;
    message_buffer[message_length++] = (checksum >> 8) & 0xFF;

    // Send message
    sendMessage(message_buffer, message_length);

    landing_targets_sent++;
    Serial.printf("MAVLink: LANDING_TARGET sent for tag ID=%d, angle=(%.3f,%.3f), dist=%.2fm\n",
                  target_data.target_num, target_data.angle_x, target_data.angle_y, target_data.distance);

    return true;
#else
    return false;
#endif
}

void MAVLinkManager::sendHeartbeat() {
#ifdef MAVLINK_HARDCODED_IMPL
    // Use hardcoded MAVLink implementation
    HardcodedMAVLink::sendHeartbeat();
#endif
}

// Old utility functions removed - now handled by hardcoded MAVLink implementation

bool MAVLinkManager::sendRTCMData(const uint8_t* rtcm_data, uint16_t length) {
    if (!initialized || !enabled || !rtcm_data || length == 0) {
        return false;
    }

#ifdef MAVLINK_HARDCODED_IMPL
    // Use hardcoded MAVLink implementation to wrap RTCM in GPS_RTCM_DATA message
    return HardcodedMAVLink::sendRTCMData(rtcm_data, length);
#else
    return false;
#endif
}

void MAVLinkManager::printStatistics() {
    Serial.println("\n--- MAVLink Statistics ---");
    Serial.printf("Status: %s\n", initialized ? (enabled ? "Enabled" : "Disabled") : "Not initialized");
    Serial.printf("Total messages sent: %u\n", messages_sent);
    Serial.printf("Landing target messages: %u\n", landing_targets_sent);
    Serial.printf("System ID: %d, Component ID: %d\n", system_id, component_id);

    if (mavlink_serial) {
        Serial.printf("UART: GPIO%d @ 57600 baud\n", Config::pins.MAVLINK_UART_TX);
    }

    Serial.println("-------------------------\n");
}

bool MAVLinkManager::testMAVLinkConnection() {
#ifdef MAVLINK_HARDCODED_IMPL
    Serial.println("Testing MAVLink connection...");

    // Use the hardcoded test function
    HardcodedMAVLink::sendTestMessages();

    return true;
#else
    Serial.println("MAVLink test: Hardcoded implementation not available");
    return false;
#endif
}

void MAVLinkManager::sendTestLandingTarget() {
    Serial.println("Sending test LANDING_TARGET message...");
    testMAVLinkConnection();
}