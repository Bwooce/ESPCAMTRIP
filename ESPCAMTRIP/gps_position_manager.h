#ifndef GPS_POSITION_MANAGER_H
#define GPS_POSITION_MANAGER_H

#include <Arduino.h>
#include <HardwareSerial.h>

// Default to MAVLink mode if neither is defined
#if !defined(RTCM_OUTPUT_MAVLINK_DISABLED) && !defined(RTCM_OUTPUT_RAW)
#define RTCM_OUTPUT_RAW
#endif

// MAVLink support - temporarily disabled due to header conflicts
#ifdef RTCM_OUTPUT_MAVLINK_DISABLED
#include <MAVLink.h>
#endif

/**
 * GPS Position Manager - Handles live GPS position data from multiple sources
 *
 * Supports:
 * - NMEA parsing from GPS receivers (ZED-F9P, NEO-M8P, etc.)
 * - MAVLink parsing from flight controllers (ArduPilot, PX4)
 * - Position validation and quality assessment
 * - Dynamic GGA message generation for NTRIP
 */

struct GPSPosition {
  double latitude;          // Degrees (WGS84)
  double longitude;         // Degrees (WGS84)
  float altitude;           // Meters above MSL
  float hdop;              // Horizontal dilution of precision
  int satellites;          // Number of satellites in use
  int fixType;             // 0=No fix, 1=GPS, 2=DGPS, 3=RTK Fixed, 4=RTK Float
  unsigned long timestamp; // millis() when position was received
  bool valid;              // True if position data is valid and recent
};

struct GPSQuality {
  bool hasFix;             // Basic GPS fix available
  bool hasRTK;             // RTK fix active (high precision)
  bool isRecent;           // Position data is recent (<5 seconds)
  bool isAccurate;         // HDOP acceptable for positioning
  float accuracy;          // Estimated accuracy in meters
  String status;           // Human readable status
};

class GPSPositionManager {
public:
  // Initialization
  static bool init();

  // Position access
  static GPSPosition getCurrentPosition();
  static GPSQuality getPositionQuality();
  static bool hasValidPosition();

  // Dynamic GGA generation for NTRIP
  static String generateGGAMessage();

  // Update from different sources
  static void updateFromNMEA(const String& nmea);
  static void updateFromMAVLink(const uint8_t* mavlink_data, size_t length);

  // Statistics and monitoring
  static unsigned long getLastUpdateTime();
  static uint32_t getTotalMessages();
  static uint32_t getValidMessages();

  // Background processing
  static void processIncomingData();
  static bool isDataAvailable();

private:
  // NMEA parsing functions
  static bool parseGGA(const String& sentence);
  static bool parseRMC(const String& sentence);
  static bool parseGSA(const String& sentence);
  static double parseLatitude(const String& lat, const String& ns);
  static double parseLongitude(const String& lon, const String& ew);
  static bool validateNMEAChecksum(const String& sentence);

  // MAVLink parsing functions (temporarily disabled)
#ifdef RTCM_OUTPUT_MAVLINK_DISABLED
  static bool parseMAVLinkGPSRaw(const mavlink_message_t* msg);
  static bool parseMAVLinkGlobalPosition(const mavlink_message_t* msg);
#endif

  // Position validation
  static bool isPositionReasonable(double lat, double lon);
  static void updatePositionQuality();

  // State variables
  static GPSPosition currentPosition;
  static GPSQuality currentQuality;
  static unsigned long lastUpdateTime;
  static uint32_t totalMessages;
  static uint32_t validMessages;
  static bool initialized;

  // NMEA parsing state
  static String nmeaBuffer;
  static bool parsingNMEA;

  // MAVLink parsing state (basic implementation)
  static uint8_t mavlinkBuffer[256];
  static size_t mavlinkBufferPos;

  // Configuration
  static const unsigned long POSITION_TIMEOUT_MS = 5000; // Position valid for 5 seconds
  static constexpr float MAX_HDOP = 5.0f;                 // Maximum acceptable HDOP
  static const int MIN_SATELLITES = 4;                    // Minimum satellites for valid fix
};

// Test data for validation (real-world wire messages)
namespace GPSTestData {
  // Real NMEA messages from ZED-F9P with RTK fix in Sydney, Australia
  extern const char* TEST_GGA_FIXED_RTK[];
  extern const char* TEST_GGA_FLOAT_RTK[];
  extern const char* TEST_GGA_NO_RTK[];
  extern const char* TEST_RMC_MOVING[];
  extern const char* TEST_GSA_QUALITY[];

  // Real MAVLink GPS_RAW_INT messages from ArduPilot
  extern const uint8_t TEST_MAVLINK_GPS_RAW[];
  extern const size_t TEST_MAVLINK_GPS_RAW_LEN;

  // Test functions
  bool runNMEATests();
  bool runMAVLinkTests();
  bool runPositionValidationTests();
  bool runAllTests();
}

#endif // GPS_POSITION_MANAGER_H