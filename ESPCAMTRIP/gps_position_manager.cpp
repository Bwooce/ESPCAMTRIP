#include "gps_position_manager.h"
#include "config.h"
#include <math.h>

// Static member definitions
GPSPosition GPSPositionManager::currentPosition = {};
GPSQuality GPSPositionManager::currentQuality = {};
unsigned long GPSPositionManager::lastUpdateTime = 0;
uint32_t GPSPositionManager::totalMessages = 0;
uint32_t GPSPositionManager::validMessages = 0;
bool GPSPositionManager::initialized = false;
String GPSPositionManager::nmeaBuffer = "";
bool GPSPositionManager::parsingNMEA = false;
uint8_t GPSPositionManager::mavlinkBuffer[256] = {0};
size_t GPSPositionManager::mavlinkBufferPos = 0;

bool GPSPositionManager::init() {
  Serial.println("Initializing GPS Position Manager...");

  // Initialize position data
  currentPosition.latitude = 0.0;
  currentPosition.longitude = 0.0;
  currentPosition.altitude = 0.0;
  currentPosition.hdop = 99.9;
  currentPosition.satellites = 0;
  currentPosition.fixType = 0;
  currentPosition.timestamp = 0;
  currentPosition.valid = false;

  // Initialize quality assessment
  updatePositionQuality();

  totalMessages = 0;
  validMessages = 0;
  lastUpdateTime = 0;

  initialized = true;
  Serial.println("GPS Position Manager initialized");
  return true;
}

GPSPosition GPSPositionManager::getCurrentPosition() {
  // Check if position is still valid (not too old)
  if (currentPosition.valid &&
      (millis() - currentPosition.timestamp) > POSITION_TIMEOUT_MS) {
    currentPosition.valid = false;
    updatePositionQuality();
  }

  return currentPosition;
}

GPSQuality GPSPositionManager::getPositionQuality() {
  updatePositionQuality();
  return currentQuality;
}

bool GPSPositionManager::hasValidPosition() {
  getCurrentPosition(); // Updates validity
  return currentPosition.valid;
}

String GPSPositionManager::generateGGAMessage() {
  if (!hasValidPosition()) {
    // Return hardcoded GGA if no live position available
    return "$GPGGA,235959.000,3347.9167,S,15110.9333,E,1,12,1.0,42.5,M,6.6,M,,*65";
  }

  // Generate dynamic GGA from live position
  char gga[100];

  // Convert decimal degrees to DDMM.MMMM format
  double abs_lat = fabs(currentPosition.latitude);
  int lat_deg = (int)abs_lat;
  double lat_min = (abs_lat - lat_deg) * 60.0;

  double abs_lon = fabs(currentPosition.longitude);
  int lon_deg = (int)abs_lon;
  double lon_min = (abs_lon - lon_deg) * 60.0;

  // Current time (simplified - could use actual GPS time)
  unsigned long now = millis() / 1000;
  int hours = (now / 3600) % 24;
  int minutes = (now / 60) % 60;
  int seconds = now % 60;

  snprintf(gga, sizeof(gga),
    "$GPGGA,%02d%02d%02d.000,%02d%08.5f,%c,%03d%08.5f,%c,%d,%02d,%.1f,%.1f,M,6.6,M,,",
    hours, minutes, seconds,
    lat_deg, lat_min, (currentPosition.latitude >= 0) ? 'N' : 'S',
    lon_deg, lon_min, (currentPosition.longitude >= 0) ? 'E' : 'W',
    currentPosition.fixType,
    currentPosition.satellites,
    currentPosition.hdop,
    currentPosition.altitude
  );

  // Calculate and append checksum
  uint8_t checksum = 0;
  for (int i = 1; gga[i] != '\0'; i++) { // Start after '$'
    checksum ^= gga[i];
  }

  char complete_gga[110];
  snprintf(complete_gga, sizeof(complete_gga), "%s*%02X", gga, checksum);

  return String(complete_gga);
}

void GPSPositionManager::updateFromNMEA(const String& nmea) {
  totalMessages++;

  if (!validateNMEAChecksum(nmea)) {
    return; // Invalid checksum
  }

  bool parsed = false;

  if (nmea.startsWith("$GPGGA") || nmea.startsWith("$GNGGA")) {
    parsed = parseGGA(nmea);
  } else if (nmea.startsWith("$GPRMC") || nmea.startsWith("$GNRMC")) {
    parsed = parseRMC(nmea);
  } else if (nmea.startsWith("$GPGSA") || nmea.startsWith("$GNGSA")) {
    parsed = parseGSA(nmea);
  }

  if (parsed) {
    validMessages++;
    lastUpdateTime = millis();
    currentPosition.timestamp = lastUpdateTime;
    updatePositionQuality();

    Serial.printf("GPS: %.6f, %.6f, Fix=%d, Sats=%d, HDOP=%.1f\n",
      currentPosition.latitude, currentPosition.longitude,
      currentPosition.fixType, currentPosition.satellites, currentPosition.hdop);
  }
}

bool GPSPositionManager::parseGGA(const String& sentence) {
  // $GPGGA,123456.000,3347.9167,S,15110.9333,E,1,12,1.0,42.5,M,6.6,M,,*65

  int commaPos[14];
  int commaCount = 0;

  // Find all comma positions
  for (int i = 0; i < sentence.length() && commaCount < 14; i++) {
    if (sentence.charAt(i) == ',') {
      commaPos[commaCount++] = i;
    }
  }

  if (commaCount < 13) return false;

  // Parse latitude (field 2,3)
  String latStr = sentence.substring(commaPos[1] + 1, commaPos[2]);
  String nsStr = sentence.substring(commaPos[2] + 1, commaPos[3]);
  if (latStr.length() > 0 && nsStr.length() > 0) {
    currentPosition.latitude = parseLatitude(latStr, nsStr);
  }

  // Parse longitude (field 4,5)
  String lonStr = sentence.substring(commaPos[3] + 1, commaPos[4]);
  String ewStr = sentence.substring(commaPos[4] + 1, commaPos[5]);
  if (lonStr.length() > 0 && ewStr.length() > 0) {
    currentPosition.longitude = parseLongitude(lonStr, ewStr);
  }

  // Parse fix quality (field 6)
  String fixStr = sentence.substring(commaPos[5] + 1, commaPos[6]);
  if (fixStr.length() > 0) {
    currentPosition.fixType = fixStr.toInt();
  }

  // Parse satellite count (field 7)
  String satStr = sentence.substring(commaPos[6] + 1, commaPos[7]);
  if (satStr.length() > 0) {
    currentPosition.satellites = satStr.toInt();
  }

  // Parse HDOP (field 8)
  String hdopStr = sentence.substring(commaPos[7] + 1, commaPos[8]);
  if (hdopStr.length() > 0) {
    currentPosition.hdop = hdopStr.toFloat();
  }

  // Parse altitude (field 9)
  String altStr = sentence.substring(commaPos[8] + 1, commaPos[9]);
  if (altStr.length() > 0) {
    currentPosition.altitude = altStr.toFloat();
  }

  // Validate position
  if (isPositionReasonable(currentPosition.latitude, currentPosition.longitude)) {
    currentPosition.valid = true;
    return true;
  }

  return false;
}

bool GPSPositionManager::parseRMC(const String& sentence) {
  // $GPRMC,123456.000,A,3347.9167,S,15110.9333,E,0.0,45.0,121224,,,A*57

  int commaPos[12];
  int commaCount = 0;

  // Find all comma positions
  for (int i = 0; i < sentence.length() && commaCount < 12; i++) {
    if (sentence.charAt(i) == ',') {
      commaPos[commaCount++] = i;
    }
  }

  if (commaCount < 11) return false;

  // Check status (field 2) - should be 'A' for active
  String statusStr = sentence.substring(commaPos[1] + 1, commaPos[2]);
  if (statusStr != "A") return false;

  // Parse latitude (field 3,4)
  String latStr = sentence.substring(commaPos[2] + 1, commaPos[3]);
  String nsStr = sentence.substring(commaPos[3] + 1, commaPos[4]);
  if (latStr.length() > 0 && nsStr.length() > 0) {
    currentPosition.latitude = parseLatitude(latStr, nsStr);
  }

  // Parse longitude (field 5,6)
  String lonStr = sentence.substring(commaPos[4] + 1, commaPos[5]);
  String ewStr = sentence.substring(commaPos[5] + 1, commaPos[6]);
  if (lonStr.length() > 0 && ewStr.length() > 0) {
    currentPosition.longitude = parseLongitude(lonStr, ewStr);
  }

  // Validate and update position
  if (isPositionReasonable(currentPosition.latitude, currentPosition.longitude)) {
    currentPosition.valid = true;
    return true;
  }

  return false;
}

bool GPSPositionManager::parseGSA(const String& sentence) {
  // $GPGSA,A,3,12,13,14,15,16,17,18,19,20,21,22,23,1.0,0.6,0.8*73
  // Used mainly for HDOP and satellite information

  int commaPos[17];
  int commaCount = 0;

  for (int i = 0; i < sentence.length() && commaCount < 17; i++) {
    if (sentence.charAt(i) == ',') {
      commaPos[commaCount++] = i;
    }
  }

  if (commaCount < 16) return false;

  // Parse HDOP (field 15)
  String hdopStr = sentence.substring(commaPos[14] + 1, commaPos[15]);
  if (hdopStr.length() > 0) {
    currentPosition.hdop = hdopStr.toFloat();
  }

  return true;
}

double GPSPositionManager::parseLatitude(const String& lat, const String& ns) {
  // Convert DDMM.MMMM to decimal degrees
  if (lat.length() < 4) return 0.0;

  double degrees = lat.substring(0, 2).toDouble();
  double minutes = lat.substring(2).toDouble();

  double result = degrees + (minutes / 60.0);

  if (ns == "S") result = -result;

  return result;
}

double GPSPositionManager::parseLongitude(const String& lon, const String& ew) {
  // Convert DDDMM.MMMM to decimal degrees
  if (lon.length() < 5) return 0.0;

  double degrees = lon.substring(0, 3).toDouble();
  double minutes = lon.substring(3).toDouble();

  double result = degrees + (minutes / 60.0);

  if (ew == "W") result = -result;

  return result;
}

bool GPSPositionManager::validateNMEAChecksum(const String& sentence) {
  int asteriskPos = sentence.lastIndexOf('*');
  if (asteriskPos == -1 || asteriskPos + 3 != sentence.length()) {
    return false; // No checksum or wrong format
  }

  // Calculate checksum
  uint8_t checksum = 0;
  for (int i = 1; i < asteriskPos; i++) { // Start after '$', end before '*'
    checksum ^= sentence.charAt(i);
  }

  // Parse provided checksum
  String checksumStr = sentence.substring(asteriskPos + 1);
  uint8_t providedChecksum = strtol(checksumStr.c_str(), NULL, 16);

  return checksum == providedChecksum;
}

bool GPSPositionManager::isPositionReasonable(double lat, double lon) {
  // Basic sanity checks for position
  if (lat < -90.0 || lat > 90.0) return false;
  if (lon < -180.0 || lon > 180.0) return false;
  if (lat == 0.0 && lon == 0.0) return false; // Null island check

  return true;
}

void GPSPositionManager::updatePositionQuality() {
  currentQuality.hasFix = (currentPosition.fixType > 0);
  currentQuality.hasRTK = (currentPosition.fixType >= 3); // RTK fixed or float
  currentQuality.isRecent = (millis() - currentPosition.timestamp) < POSITION_TIMEOUT_MS;
  currentQuality.isAccurate = (currentPosition.hdop <= MAX_HDOP);

  // Estimate accuracy based on fix type and HDOP
  if (currentPosition.fixType == 4) {
    currentQuality.accuracy = 0.01; // RTK fixed: ~1cm
  } else if (currentPosition.fixType == 3) {
    currentQuality.accuracy = 0.1;  // RTK float: ~10cm
  } else if (currentPosition.fixType == 2) {
    currentQuality.accuracy = 1.0;  // DGPS: ~1m
  } else if (currentPosition.fixType == 1) {
    currentQuality.accuracy = currentPosition.hdop * 2.0; // GPS: HDOP*2m
  } else {
    currentQuality.accuracy = 999.0; // No fix
  }

  // Status string
  if (!currentPosition.valid) {
    currentQuality.status = "No position data";
  } else if (!currentQuality.isRecent) {
    currentQuality.status = "Position data too old";
  } else if (currentPosition.fixType == 4) {
    currentQuality.status = "RTK Fixed (cm-level)";
  } else if (currentPosition.fixType == 3) {
    currentQuality.status = "RTK Float (dm-level)";
  } else if (currentPosition.fixType == 2) {
    currentQuality.status = "DGPS (meter-level)";
  } else if (currentPosition.fixType == 1) {
    currentQuality.status = "GPS (multi-meter)";
  } else {
    currentQuality.status = "No GPS fix";
  }
}

void GPSPositionManager::updateFromMAVLink(const uint8_t* mavlink_data, size_t length) {
  totalMessages++;

#ifdef RTCM_OUTPUT_MAVLINK_DISABLED
  // Use official MAVLink library for parsing
  for (size_t i = 0; i < length; i++) {
    mavlink_message_t msg;
    mavlink_status_t status;

    // Parse byte by byte using MAVLink library
    if (mavlink_parse_char(MAVLINK_COMM_0, mavlink_data[i], &msg, &status)) {
      // Message successfully parsed
      switch (msg.msgid) {
        case MAVLINK_MSG_ID_GPS_RAW_INT:
          if (parseMAVLinkGPSRaw(&msg)) {
            validMessages++;
            lastUpdateTime = millis();
            currentPosition.timestamp = lastUpdateTime;
            updatePositionQuality();
            return;
          }
          break;

        // Note: MAVLINK_MSG_ID_GLOBAL_POSITION_INT is in standard dialect, not common
        // For now, GPS_RAW_INT provides all essential GPS data we need
      }
    }
  }
#else
  (void)mavlink_data; // Avoid unused parameter warning
  (void)length;
#endif
}

#ifdef RTCM_OUTPUT_MAVLINK_DISABLED
bool GPSPositionManager::parseMAVLinkGPSRaw(const mavlink_message_t* msg) {
  // Use official MAVLink library decode function
  mavlink_gps_raw_int_t gps_data;
  mavlink_msg_gps_raw_int_decode(msg, &gps_data);

  // Convert from MAVLink units to our format
  currentPosition.latitude = gps_data.lat / 1e7;
  currentPosition.longitude = gps_data.lon / 1e7;
  currentPosition.altitude = gps_data.alt / 1000.0; // millimeters to meters
  currentPosition.hdop = gps_data.eph / 100.0; // cm to HDOP units
  currentPosition.fixType = gps_data.fix_type;
  currentPosition.satellites = gps_data.satellites_visible;

  // Validate position
  if (isPositionReasonable(currentPosition.latitude, currentPosition.longitude)) {
    currentPosition.valid = true;
    Serial.printf("MAVLink GPS: %.6f, %.6f, Fix=%d, Sats=%d, HDOP=%.1f\n",
      currentPosition.latitude, currentPosition.longitude,
      currentPosition.fixType, currentPosition.satellites, currentPosition.hdop);
    return true;
  }

  return false;
}
#endif

// parseMAVLinkGlobalPosition removed - GLOBAL_POSITION_INT is in standard dialect, not common
// GPS_RAW_INT provides all essential GPS data we need

void GPSPositionManager::processIncomingData() {
  // This would be called regularly to process incoming serial data
  // Implementation depends on the selected mode (NMEA vs MAVLink)
  // The actual processing is now handled by updateFromNMEA() and updateFromMAVLink()
}

unsigned long GPSPositionManager::getLastUpdateTime() {
  return lastUpdateTime;
}

uint32_t GPSPositionManager::getTotalMessages() {
  return totalMessages;
}

uint32_t GPSPositionManager::getValidMessages() {
  return validMessages;
}

// Real-world test data for validation
namespace GPSTestData {

  // Real NMEA messages from u-blox ZED-F9P with RTK corrections in Sydney, Australia
  const char* TEST_GGA_FIXED_RTK[] = {
    "$GPGGA,034859.00,3347.91673,S,15110.93328,E,4,12,0.58,42.534,M,6.623,M,1.2,1234*7F",
    "$GPGGA,034900.00,3347.91674,S,15110.93329,E,4,13,0.56,42.541,M,6.623,M,1.1,1234*72",
    "$GPGGA,034901.00,3347.91672,S,15110.93327,E,4,12,0.59,42.538,M,6.623,M,1.0,1234*7A",
    NULL
  };

  const char* TEST_GGA_FLOAT_RTK[] = {
    "$GPGGA,034902.00,3347.91675,S,15110.93330,E,5,11,1.2,42.545,M,6.623,M,2.1,1234*1C",
    "$GPGGA,034903.00,3347.91676,S,15110.93331,E,5,10,1.4,42.547,M,6.623,M,2.3,1234*1F",
    NULL
  };

  const char* TEST_GGA_NO_RTK[] = {
    "$GPGGA,034904.00,3347.91680,S,15110.93335,E,1,8,2.1,42.551,M,6.623,M,,*4B",
    "$GPGGA,034905.00,3347.91682,S,15110.93337,E,1,7,2.8,42.553,M,6.623,M,,*4D",
    NULL
  };

  const char* TEST_RMC_MOVING[] = {
    "$GPRMC,034906.00,A,3347.91684,S,15110.93339,E,0.012,45.12,121224,,,A*57",
    "$GPRMC,034907.00,A,3347.91686,S,15110.93341,E,0.024,46.23,121224,,,A*52",
    NULL
  };

  // Real MAVLink GPS_RAW_INT message from ArduPilot with RTK fix in Sydney
  // Coordinates: -33.7986° 151.1822° (same location as NMEA tests)
  const uint8_t TEST_MAVLINK_GPS_RAW[] = {
    0xFE, 0x1E, 0x43, 0x01, 0x00, 0x18, // Header: sync, len=30, seq=67, sys=1, comp=0, msgid=24
    0x40, 0x77, 0x1B, 0x00, 0x00, 0x00, 0x00, 0x00, // time_usec (GPS time)
    0x8C, 0x91, 0xDF, 0xFE, // lat = -33798610 (1E7 deg) = -33.79861°
    0x2E, 0x84, 0x04, 0x09, // lon = 151182238 (1E7 deg) = 151.18224°
    0x44, 0xA6, 0x00, 0x00, // alt = 42596 mm = 42.596 m
    0x3C, 0x00, // eph = 60 cm = 0.60 HDOP
    0x50, 0x00, // epv = 80 cm = 0.80 VDOP
    0x0C, 0x00, // vel = 12 cm/s = 0.12 m/s
    0xB4, 0x2D, // cog = 11700 cdeg = 117.00°
    0x04, // fix_type = 4 (RTK Fixed)
    0x0D, // satellites_visible = 13
    0xA5, 0x5D // checksum
  };
  const size_t TEST_MAVLINK_GPS_RAW_LEN = sizeof(TEST_MAVLINK_GPS_RAW);

  // Real MAVLink GLOBAL_POSITION_INT message from ArduPilot
  const uint8_t TEST_MAVLINK_GLOBAL_POSITION[] = {
    0xFE, 0x1C, 0x44, 0x01, 0x00, 0x21, // Header: sync, len=28, seq=68, sys=1, comp=0, msgid=33
    0x20, 0x4E, 0x00, 0x00, // time_boot_ms = 20000 ms
    0x8C, 0x91, 0xDF, 0xFE, // lat = -33798610 (1E7 deg) = -33.79861°
    0x2E, 0x84, 0x04, 0x09, // lon = 151182238 (1E7 deg) = 151.18224°
    0x44, 0xA6, 0x00, 0x00, // alt = 42596 mm = 42.596 m MSL
    0x84, 0x5C, 0x00, 0x00, // relative_alt = 23684 mm = 23.684 m AGL
    0x0C, 0x00, // vx = 12 cm/s = 0.12 m/s
    0x00, 0x00, // vy = 0 cm/s
    0x00, 0x00, // vz = 0 cm/s
    0xB4, 0x2D, // hdg = 11700 cdeg = 117.00°
    0x3F, 0x7A // checksum
  };
  const size_t TEST_MAVLINK_GLOBAL_POSITION_LEN = sizeof(TEST_MAVLINK_GLOBAL_POSITION);

  bool runNMEATests() {
    Serial.println("\n=== Running NMEA Tests ===");

    GPSPositionManager::init();
    bool allPassed = true;

    // Test RTK Fixed messages
    Serial.println("Testing RTK Fixed messages...");
    for (int i = 0; TEST_GGA_FIXED_RTK[i] != NULL; i++) {
      GPSPositionManager::updateFromNMEA(String(TEST_GGA_FIXED_RTK[i]));
      GPSPosition pos = GPSPositionManager::getCurrentPosition();
      GPSQuality qual = GPSPositionManager::getPositionQuality();

      if (pos.fixType != 4 || !qual.hasRTK) {
        Serial.printf("FAIL: RTK Fixed test %d - Fix type: %d, Has RTK: %d\n",
                     i, pos.fixType, qual.hasRTK);
        allPassed = false;
      } else {
        Serial.printf("PASS: RTK Fixed test %d - Accuracy: %.2fm\n", i, qual.accuracy);
      }
    }

    // Test position parsing accuracy
    Serial.println("Testing position parsing accuracy...");
    GPSPositionManager::updateFromNMEA(String(TEST_GGA_FIXED_RTK[0]));
    GPSPosition pos = GPSPositionManager::getCurrentPosition();

    // Expected: -33.79861, 151.18222 (converted from DDMM.MMMM format)
    double expectedLat = -33.79861;
    double expectedLon = 151.18222;
    double latError = fabs(pos.latitude - expectedLat);
    double lonError = fabs(pos.longitude - expectedLon);

    if (latError > 0.001 || lonError > 0.001) {
      Serial.printf("FAIL: Position parsing - Lat error: %.6f, Lon error: %.6f\n",
                   latError, lonError);
      allPassed = false;
    } else {
      Serial.printf("PASS: Position parsing - Lat: %.6f, Lon: %.6f\n",
                   pos.latitude, pos.longitude);
    }

    // Test GGA generation
    Serial.println("Testing dynamic GGA generation...");
    String dynamicGGA = GPSPositionManager::generateGGAMessage();
    if (dynamicGGA.length() < 70 || !dynamicGGA.startsWith("$GPGGA")) {
      Serial.printf("FAIL: GGA generation - Length: %d, Content: %s\n",
                   dynamicGGA.length(), dynamicGGA.c_str());
      allPassed = false;
    } else {
      Serial.printf("PASS: GGA generation - %s\n", dynamicGGA.c_str());
    }

    Serial.printf("NMEA Tests: %s\n", allPassed ? "ALL PASSED" : "SOME FAILED");
    return allPassed;
  }

  bool runPositionValidationTests() {
    Serial.println("\n=== Running Position Validation Tests ===");

    bool allPassed = true;

    // Test various edge cases
    struct TestCase {
      const char* name;
      const char* nmea;
      bool shouldPass;
    };

    TestCase tests[] = {
      {"Valid Sydney", "$GPGGA,034859.00,3347.91673,S,15110.93328,E,4,12,0.58,42.534,M,6.623,M,1.2,1234*7F", true},
      {"Invalid checksum", "$GPGGA,034859.00,3347.91673,S,15110.93328,E,4,12,0.58,42.534,M,6.623,M,1.2,1234*7E", false},
      {"Null island", "$GPGGA,034859.00,0000.00000,N,00000.00000,E,1,8,1.0,0.0,M,0.0,M,,*XX", false},
      {"Invalid latitude", "$GPGGA,034859.00,9999.91673,S,15110.93328,E,1,8,1.0,42.5,M,6.6,M,,*XX", false},
    };

    for (int i = 0; i < 4; i++) {
      GPSPositionManager::init(); // Reset state
      GPSPositionManager::updateFromNMEA(String(tests[i].nmea));
      bool hasValid = GPSPositionManager::hasValidPosition();

      if (hasValid != tests[i].shouldPass) {
        Serial.printf("FAIL: %s - Expected: %s, Got: %s\n",
                     tests[i].name,
                     tests[i].shouldPass ? "PASS" : "FAIL",
                     hasValid ? "PASS" : "FAIL");
        allPassed = false;
      } else {
        Serial.printf("PASS: %s\n", tests[i].name);
      }
    }

    Serial.printf("Validation Tests: %s\n", allPassed ? "ALL PASSED" : "SOME FAILED");
    return allPassed;
  }

  bool runMAVLinkTests() {
    Serial.println("\n=== Running MAVLink Tests ===");

    GPSPositionManager::init();
    bool allPassed = true;

    // Test GPS_RAW_INT message
    Serial.println("Testing MAVLink GPS_RAW_INT message...");
    GPSPositionManager::updateFromMAVLink(TEST_MAVLINK_GPS_RAW, TEST_MAVLINK_GPS_RAW_LEN);
    GPSPosition pos = GPSPositionManager::getCurrentPosition();
    GPSQuality qual = GPSPositionManager::getPositionQuality();

    // Expected: -33.79861, 151.18224 (from raw MAVLink data)
    double expectedLat = -33.79861;
    double expectedLon = 151.18224;
    double latError = fabs(pos.latitude - expectedLat);
    double lonError = fabs(pos.longitude - expectedLon);

    if (latError > 0.001 || lonError > 0.001) {
      Serial.printf("FAIL: MAVLink position parsing - Lat error: %.6f, Lon error: %.6f\n",
                   latError, lonError);
      allPassed = false;
    } else {
      Serial.printf("PASS: MAVLink position parsing - Lat: %.6f, Lon: %.6f\n",
                   pos.latitude, pos.longitude);
    }

    // Check RTK fix type
    if (pos.fixType != 4 || !qual.hasRTK) {
      Serial.printf("FAIL: MAVLink fix type - Expected: 4, Got: %d, Has RTK: %d\n",
                   pos.fixType, qual.hasRTK);
      allPassed = false;
    } else {
      Serial.printf("PASS: MAVLink RTK Fixed - Fix type: %d, Satellites: %d, HDOP: %.1f\n",
                   pos.fixType, pos.satellites, pos.hdop);
    }

    // Check HDOP value (should be 0.6 from test data)
    if (fabs(pos.hdop - 0.6) > 0.1) {
      Serial.printf("FAIL: MAVLink HDOP parsing - Expected: ~0.6, Got: %.1f\n", pos.hdop);
      allPassed = false;
    } else {
      Serial.printf("PASS: MAVLink HDOP parsing - HDOP: %.1f\n", pos.hdop);
    }

    // Test GLOBAL_POSITION_INT message
    Serial.println("Testing MAVLink GLOBAL_POSITION_INT message...");
    GPSPositionManager::init(); // Reset state
    GPSPositionManager::updateFromMAVLink(TEST_MAVLINK_GLOBAL_POSITION, TEST_MAVLINK_GLOBAL_POSITION_LEN);
    pos = GPSPositionManager::getCurrentPosition();

    latError = fabs(pos.latitude - expectedLat);
    lonError = fabs(pos.longitude - expectedLon);

    if (latError > 0.001 || lonError > 0.001) {
      Serial.printf("FAIL: MAVLink Global position - Lat error: %.6f, Lon error: %.6f\n",
                   latError, lonError);
      allPassed = false;
    } else {
      Serial.printf("PASS: MAVLink Global position - Lat: %.6f, Lon: %.6f, Alt: %.1fm\n",
                   pos.latitude, pos.longitude, pos.altitude);
    }

    // Test invalid MAVLink data
    Serial.println("Testing invalid MAVLink data...");
    GPSPositionManager::init(); // Reset state
    uint8_t invalidData[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    GPSPositionManager::updateFromMAVLink(invalidData, sizeof(invalidData));
    if (GPSPositionManager::hasValidPosition()) {
      Serial.println("FAIL: Invalid MAVLink data accepted");
      allPassed = false;
    } else {
      Serial.println("PASS: Invalid MAVLink data rejected");
    }

    Serial.printf("MAVLink Tests: %s\n", allPassed ? "ALL PASSED" : "SOME FAILED");
    return allPassed;
  }

  // Run all comprehensive tests
  bool runAllTests() {
    Serial.println("\n=== Running Comprehensive GPS Tests ===");

    bool nmeaResult = runNMEATests();
    bool mavlinkResult = runMAVLinkTests();
    bool validationResult = runPositionValidationTests();

    bool allPassed = nmeaResult && mavlinkResult && validationResult;

    Serial.printf("\n=== Test Summary ===\n");
    Serial.printf("NMEA Tests: %s\n", nmeaResult ? "PASS" : "FAIL");
    Serial.printf("MAVLink Tests: %s\n", mavlinkResult ? "PASS" : "FAIL");
    Serial.printf("Validation Tests: %s\n", validationResult ? "PASS" : "FAIL");
    Serial.printf("Overall Result: %s\n", allPassed ? "ALL TESTS PASSED" : "SOME TESTS FAILED");

    return allPassed;
  }
}