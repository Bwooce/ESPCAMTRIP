#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H

#include <Arduino.h>
#include <HardwareSerial.h>

// GPS Input Mode Configuration
// Choose ONE of the following input modes:
//
// GPS_INPUT_RAW (default)
//   - Receives NMEA/UBX data directly from F9P GPS receiver
//   - Use for: Direct connection to u-blox ZED-F9P, NEO-M8P, or similar
//   - Wire: ESP32 RX (GPIO7) <- GPS TX
//
// GPS_INPUT_MAVLINK
//   - Receives GPS data via MAVLink from ArduPilot flight controller
//   - Use for: When ArduPilot is the primary GPS source
//   - Wire: ESP32 RX (GPIO7) <- ArduPilot TX

#ifndef GPS_INPUT_RAW
#ifndef GPS_INPUT_MAVLINK
#define GPS_INPUT_RAW  // Default to raw NMEA/UBX input
#endif
#endif

// GPS Fix Quality definitions (matches NMEA standard)
#define GPS_FIX_INVALID     0
#define GPS_FIX_GPS         1
#define GPS_FIX_DGPS        2
#define GPS_FIX_PPS         3
#define GPS_FIX_RTK_FIXED   4
#define GPS_FIX_RTK_FLOAT   5
#define GPS_FIX_ESTIMATED   6
#define GPS_FIX_MANUAL      7
#define GPS_FIX_SIMULATION  8

// GPS Position data structure
struct GPSPosition {
    double latitude;        // Degrees (WGS84)
    double longitude;       // Degrees (WGS84)
    float altitude;         // Meters above mean sea level
    float accuracy;         // Horizontal accuracy estimate (meters)
    float speed;           // Ground speed (m/s)
    float course;          // Course over ground (degrees)
    uint8_t fixQuality;    // GPS fix quality (see definitions above)
    uint8_t satellites;    // Number of satellites in use
    float hdop;           // Horizontal dilution of precision
    float vdop;           // Vertical dilution of precision
    uint32_t timestamp;    // Unix timestamp (seconds since epoch)
    uint32_t lastUpdate;   // millis() when last updated
    bool valid;           // True if position data is valid and recent

    // RTK-specific data
    float ageOfDiff;      // Age of differential corrections (seconds)
    uint8_t baseStationId; // Base station ID

    GPSPosition() :
        latitude(0.0), longitude(0.0), altitude(0.0), accuracy(999.0),
        speed(0.0), course(0.0), fixQuality(GPS_FIX_INVALID),
        satellites(0), hdop(99.99), vdop(99.99), timestamp(0),
        lastUpdate(0), valid(false), ageOfDiff(0.0), baseStationId(0) {}
};

// GPS Statistics
struct GPSStatistics {
    uint32_t messagesReceived;
    uint32_t messagesProcessed;
    uint32_t parseErrors;
    uint32_t checksumErrors;
    uint32_t lastMessageTime;
    uint8_t messageRate;      // Messages per second

    void reset() {
        messagesReceived = 0;
        messagesProcessed = 0;
        parseErrors = 0;
        checksumErrors = 0;
        lastMessageTime = 0;
        messageRate = 0;
    }
};

class GPSManager {
private:
    static HardwareSerial gpsSerial;
    static GPSPosition currentPosition;
    static GPSStatistics stats;
    static char nmeaBuffer[256];
    static int bufferIndex;
    static unsigned long lastStatsTime;
    static uint32_t messageCount;

    // NMEA parsing functions
    static bool parseNMEA(const char* sentence);
    static bool parseGGA(const char* sentence);
    static bool parseRMC(const char* sentence);
    static bool parseGSA(const char* sentence);
    static bool parseGSV(const char* sentence);
    static bool validateChecksum(const char* sentence);
    static double parseCoordinate(const char* coord, const char* hemisphere);
    static uint32_t parseTime(const char* timeStr);
    static uint32_t parseDate(const char* dateStr, uint32_t timeSeconds);

    // MAVLink parsing functions (for GPS_INPUT_MAVLINK mode)
    #ifdef GPS_INPUT_MAVLINK
    static bool parseMAVLink();
    #endif

    // Utility functions
    static void updateStatistics();
    static bool isPositionFresh(uint32_t maxAge = 5000);
    static void resetPosition();

public:
    // Initialization and control
    static bool init();
    static void update();
    static void reset();

    // Position access
    static GPSPosition getPosition();
    static bool hasValidFix();
    static bool hasRTKFix();
    static uint8_t getFixQuality();
    static uint8_t getSatelliteCount();
    static float getHorizontalAccuracy();

    // Status and diagnostics
    static GPSStatistics getStatistics();
    static void printStatus();
    static void printPosition();
    static bool isReceivingData();

    // Utility functions
    static String getFixQualityString(uint8_t quality);
    static String formatCoordinate(double coord, bool isLatitude);
    static float calculateDistance(double lat1, double lon1, double lat2, double lon2);
    static float calculateBearing(double lat1, double lon1, double lat2, double lon2);

    // Configuration
    static void setUpdateRate(uint8_t rateHz);
    static void enableDebugOutput(bool enable);
};

#endif // GPS_MANAGER_H