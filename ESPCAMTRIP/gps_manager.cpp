#include "gps_manager.h"
#include "config.h"
#include <time.h>

// Only include MAVLink if using MAVLink GPS input mode
#ifdef GPS_INPUT_MAVLINK
#include <MAVLink.h>
#endif

// Static member definitions
HardwareSerial GPSManager::gpsSerial(1);  // Use UART1 for GPS
GPSPosition GPSManager::currentPosition;
GPSStatistics GPSManager::stats;
char GPSManager::nmeaBuffer[256];
int GPSManager::bufferIndex = 0;
unsigned long GPSManager::lastStatsTime = 0;
uint32_t GPSManager::messageCount = 0;

bool GPSManager::init() {
    Serial.println("Initializing GPS Manager...");

#ifdef GPS_INPUT_RAW
    Serial.println("GPS input mode: RAW (NMEA/UBX from GPS receiver)");
    // Initialize UART for GPS data input
    // Note: Using -1 for TX pin since we only need to receive GPS data
    gpsSerial.begin(115200, SERIAL_8N1, Config::pins.GPS_UART_RX, -1);
#else
    Serial.println("GPS input mode: MAVLink (from ArduPilot)");
    // Initialize UART for MAVLink GPS data
    gpsSerial.begin(115200, SERIAL_8N1, Config::pins.GPS_UART_RX, -1);
#endif

    // Reset statistics and position
    stats.reset();
    resetPosition();
    bufferIndex = 0;
    lastStatsTime = millis();
    messageCount = 0;

    Serial.println("GPS Manager initialized");
    return true;
}

void GPSManager::update() {
    // Process incoming GPS data
    while (gpsSerial.available()) {
        char c = gpsSerial.read();

#ifdef GPS_INPUT_RAW
        // Process NMEA sentences
        if (c == '$') {
            // Start of new NMEA sentence
            bufferIndex = 0;
        }

        if (bufferIndex < sizeof(nmeaBuffer) - 1) {
            nmeaBuffer[bufferIndex++] = c;
        }

        if (c == '\n' && bufferIndex > 1) {
            // End of NMEA sentence
            nmeaBuffer[bufferIndex] = '\0';
            stats.messagesReceived++;

            if (parseNMEA(nmeaBuffer)) {
                stats.messagesProcessed++;
            } else {
                stats.parseErrors++;
            }

            bufferIndex = 0;
            messageCount++;
        }
#else
        // Process MAVLink messages (placeholder for future implementation)
        // This would parse MAVLink GPS_RAW_INT or GLOBAL_POSITION_INT messages
        stats.messagesReceived++;
        messageCount++;
#endif
    }

    // Update statistics periodically
    updateStatistics();

    // Check if position data is stale
    if (!isPositionFresh()) {
        currentPosition.valid = false;
    }
}

bool GPSManager::parseNMEA(const char* sentence) {
    if (!sentence || strlen(sentence) < 6) return false;

    // Validate checksum
    if (!validateChecksum(sentence)) {
        stats.checksumErrors++;
        return false;
    }

    // Identify sentence type and parse
    if (strncmp(sentence + 3, "GGA", 3) == 0) {
        return parseGGA(sentence);
    } else if (strncmp(sentence + 3, "RMC", 3) == 0) {
        return parseRMC(sentence);
    } else if (strncmp(sentence + 3, "GSA", 3) == 0) {
        return parseGSA(sentence);
    } else if (strncmp(sentence + 3, "GSV", 3) == 0) {
        return parseGSV(sentence);
    }

    return false;
}

bool GPSManager::parseGGA(const char* sentence) {
    // $GPGGA,hhmmss.ss,ddmm.mm,N,ddmm.mm,E,q,ss,h.h,a.a,M,b.b,M,t.t,nnnn*hh
    // Example: $GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76

    char* tokens[15];
    char buffer[256];
    strncpy(buffer, sentence, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    // Tokenize the sentence
    int tokenCount = 0;
    char* token = strtok(buffer, ",");
    while (token && tokenCount < 15) {
        tokens[tokenCount++] = token;
        token = strtok(NULL, ",");
    }

    if (tokenCount < 10) return false;

    // Parse time (tokens[1])
    if (strlen(tokens[1]) >= 6) {
        currentPosition.timestamp = parseTime(tokens[1]);
    }

    // Parse latitude (tokens[2] and tokens[3])
    if (strlen(tokens[2]) > 0 && strlen(tokens[3]) > 0) {
        currentPosition.latitude = parseCoordinate(tokens[2], tokens[3]);
    }

    // Parse longitude (tokens[4] and tokens[5])
    if (strlen(tokens[4]) > 0 && strlen(tokens[5]) > 0) {
        currentPosition.longitude = parseCoordinate(tokens[4], tokens[5]);
    }

    // Parse fix quality (tokens[6])
    if (strlen(tokens[6]) > 0) {
        currentPosition.fixQuality = atoi(tokens[6]);
    }

    // Parse number of satellites (tokens[7])
    if (strlen(tokens[7]) > 0) {
        currentPosition.satellites = atoi(tokens[7]);
    }

    // Parse HDOP (tokens[8])
    if (strlen(tokens[8]) > 0) {
        currentPosition.hdop = atof(tokens[8]);
    }

    // Parse altitude (tokens[9])
    if (strlen(tokens[9]) > 0) {
        currentPosition.altitude = atof(tokens[9]);
    }

    // Update position status
    currentPosition.lastUpdate = millis();
    currentPosition.valid = (currentPosition.fixQuality > GPS_FIX_INVALID &&
                           currentPosition.satellites >= 4);

    // Estimate accuracy based on fix quality and HDOP
    if (currentPosition.fixQuality == GPS_FIX_RTK_FIXED) {
        currentPosition.accuracy = 0.02f;  // 2cm RTK fixed
    } else if (currentPosition.fixQuality == GPS_FIX_RTK_FLOAT) {
        currentPosition.accuracy = 0.5f;   // 50cm RTK float
    } else if (currentPosition.fixQuality == GPS_FIX_DGPS) {
        currentPosition.accuracy = 1.0f;   // 1m DGPS
    } else if (currentPosition.fixQuality == GPS_FIX_GPS) {
        currentPosition.accuracy = currentPosition.hdop * 2.0f;  // ~4m typical
    } else {
        currentPosition.accuracy = 999.0f; // Invalid
    }

    return true;
}

bool GPSManager::parseRMC(const char* sentence) {
    // $GPRMC,hhmmss.ss,A,ddmm.mm,N,ddmm.mm,E,x.x,x.x,ddmmyy,x.x,a*hh
    // Example: $GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A*43

    char* tokens[13];
    char buffer[256];
    strncpy(buffer, sentence, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    // Tokenize the sentence
    int tokenCount = 0;
    char* token = strtok(buffer, ",");
    while (token && tokenCount < 13) {
        tokens[tokenCount++] = token;
        token = strtok(NULL, ",");
    }

    if (tokenCount < 10) return false;

    // Check validity (tokens[2])
    if (strlen(tokens[2]) == 0 || tokens[2][0] != 'A') {
        return false; // Invalid fix
    }

    // Parse speed (tokens[7]) - knots to m/s
    if (strlen(tokens[7]) > 0) {
        currentPosition.speed = atof(tokens[7]) * 0.514444f;
    }

    // Parse course (tokens[8])
    if (strlen(tokens[8]) > 0) {
        currentPosition.course = atof(tokens[8]);
    }

    // Parse date and combine with time
    if (strlen(tokens[1]) >= 6 && strlen(tokens[9]) >= 6) {
        uint32_t timeSeconds = parseTime(tokens[1]);
        currentPosition.timestamp = parseDate(tokens[9], timeSeconds);
    }

    return true;
}

bool GPSManager::parseGSA(const char* sentence) {
    // $GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39
    // Parse PDOP, HDOP, VDOP from this sentence

    char* tokens[18];
    char buffer[256];
    strncpy(buffer, sentence, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    int tokenCount = 0;
    char* token = strtok(buffer, ",");
    while (token && tokenCount < 18) {
        tokens[tokenCount++] = token;
        token = strtok(NULL, ",");
    }

    if (tokenCount < 17) return false;

    // Parse HDOP (tokens[16])
    if (strlen(tokens[16]) > 0) {
        currentPosition.hdop = atof(tokens[16]);
    }

    // Parse VDOP (tokens[17] - but may include checksum)
    if (strlen(tokens[17]) > 0) {
        char* asterisk = strchr(tokens[17], '*');
        if (asterisk) *asterisk = '\0';
        currentPosition.vdop = atof(tokens[17]);
    }

    return true;
}

bool GPSManager::parseGSV(const char* sentence) {
    // $GPGSV,2,1,07,07,79,048,42,02,51,062,43,26,36,256,42,27,27,138,42*71
    // We can extract satellite count from this but it's already in GGA
    return true; // Acknowledge parsing but don't need to extract data
}

bool GPSManager::validateChecksum(const char* sentence) {
    if (!sentence || strlen(sentence) < 5) return false;

    // Find checksum delimiter
    const char* asterisk = strrchr(sentence, '*');
    if (!asterisk) return false;

    // Calculate checksum
    uint8_t checksum = 0;
    for (const char* p = sentence + 1; p < asterisk; p++) {
        checksum ^= *p;
    }

    // Parse expected checksum
    uint8_t expectedChecksum = strtol(asterisk + 1, NULL, 16);

    return (checksum == expectedChecksum);
}

double GPSManager::parseCoordinate(const char* coord, const char* hemisphere) {
    if (!coord || !hemisphere || strlen(coord) < 7) return 0.0;

    // Parse DDMM.MMMM format
    double degrees = 0.0;
    double minutes = 0.0;

    if (strchr(coord, '.')) {
        // Find decimal point
        char* decimal = strchr(coord, '.');
        int beforeDecimal = decimal - coord;

        if (beforeDecimal >= 4) {
            // Extract degrees (first 2 or 3 digits)
            char degreeStr[4];
            int degreeDigits = beforeDecimal - 2;
            strncpy(degreeStr, coord, degreeDigits);
            degreeStr[degreeDigits] = '\0';
            degrees = atof(degreeStr);

            // Extract minutes (last 2 digits before decimal + fractional part)
            minutes = atof(coord + degreeDigits);
        }
    }

    double result = degrees + (minutes / 60.0);

    // Apply hemisphere
    if (hemisphere[0] == 'S' || hemisphere[0] == 'W') {
        result = -result;
    }

    return result;
}

uint32_t GPSManager::parseTime(const char* timeStr) {
    if (!timeStr || strlen(timeStr) < 6) return 0;

    // Parse HHMMSS.SS format
    int hours = (timeStr[0] - '0') * 10 + (timeStr[1] - '0');
    int minutes = (timeStr[2] - '0') * 10 + (timeStr[3] - '0');
    int seconds = (timeStr[4] - '0') * 10 + (timeStr[5] - '0');

    return hours * 3600 + minutes * 60 + seconds;
}

uint32_t GPSManager::parseDate(const char* dateStr, uint32_t timeSeconds) {
    if (!dateStr || strlen(dateStr) < 6) return timeSeconds;

    // Parse DDMMYY format
    int day = (dateStr[0] - '0') * 10 + (dateStr[1] - '0');
    int month = (dateStr[2] - '0') * 10 + (dateStr[3] - '0');
    int year = (dateStr[4] - '0') * 10 + (dateStr[5] - '0') + 2000;

    // Convert to Unix timestamp
    struct tm timeinfo = {0};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = timeSeconds / 3600;
    timeinfo.tm_min = (timeSeconds % 3600) / 60;
    timeinfo.tm_sec = timeSeconds % 60;

    return mktime(&timeinfo);
}

void GPSManager::updateStatistics() {
    unsigned long now = millis();
    if (now - lastStatsTime >= 1000) {
        stats.messageRate = messageCount;
        stats.lastMessageTime = now;
        messageCount = 0;
        lastStatsTime = now;
    }
}

bool GPSManager::isPositionFresh(uint32_t maxAge) {
    return (millis() - currentPosition.lastUpdate) < maxAge;
}

void GPSManager::resetPosition() {
    currentPosition = GPSPosition(); // Reset to default values
}

GPSPosition GPSManager::getPosition() {
    return currentPosition;
}

bool GPSManager::hasValidFix() {
    return currentPosition.valid && isPositionFresh();
}

bool GPSManager::hasRTKFix() {
    return hasValidFix() &&
           (currentPosition.fixQuality == GPS_FIX_RTK_FIXED ||
            currentPosition.fixQuality == GPS_FIX_RTK_FLOAT);
}

uint8_t GPSManager::getFixQuality() {
    return hasValidFix() ? currentPosition.fixQuality : GPS_FIX_INVALID;
}

uint8_t GPSManager::getSatelliteCount() {
    return currentPosition.satellites;
}

float GPSManager::getHorizontalAccuracy() {
    return hasValidFix() ? currentPosition.accuracy : 999.0f;
}

GPSStatistics GPSManager::getStatistics() {
    return stats;
}

void GPSManager::printStatus() {
    Serial.println("\n--- GPS Status ---");
    Serial.printf("Fix: %s (%d satellites)\n",
                  getFixQualityString(currentPosition.fixQuality).c_str(),
                  currentPosition.satellites);
    Serial.printf("HDOP: %.2f, VDOP: %.2f\n", currentPosition.hdop, currentPosition.vdop);
    Serial.printf("Accuracy: %.2fm\n", currentPosition.accuracy);
    Serial.printf("Messages: %lu received, %lu processed\n",
                  stats.messagesReceived, stats.messagesProcessed);
    Serial.printf("Errors: %lu parse, %lu checksum\n",
                  stats.parseErrors, stats.checksumErrors);
    Serial.printf("Rate: %d msg/sec\n", stats.messageRate);
    Serial.println("----------------\n");
}

void GPSManager::printPosition() {
    if (!hasValidFix()) {
        Serial.println("GPS: No valid fix");
        return;
    }

    Serial.printf("GPS: %s %s %.2fm (±%.2fm) %s\n",
                  formatCoordinate(currentPosition.latitude, true).c_str(),
                  formatCoordinate(currentPosition.longitude, false).c_str(),
                  currentPosition.altitude,
                  currentPosition.accuracy,
                  getFixQualityString(currentPosition.fixQuality).c_str());
}

bool GPSManager::isReceivingData() {
    return stats.messageRate > 0 && (millis() - stats.lastMessageTime) < 2000;
}

String GPSManager::getFixQualityString(uint8_t quality) {
    switch (quality) {
        case GPS_FIX_INVALID:    return "None";
        case GPS_FIX_GPS:        return "GPS";
        case GPS_FIX_DGPS:       return "DGPS";
        case GPS_FIX_PPS:        return "PPS";
        case GPS_FIX_RTK_FIXED:  return "RTK Fixed";
        case GPS_FIX_RTK_FLOAT:  return "RTK Float";
        case GPS_FIX_ESTIMATED:  return "Estimated";
        case GPS_FIX_MANUAL:     return "Manual";
        case GPS_FIX_SIMULATION: return "Simulation";
        default:                 return "Unknown";
    }
}

String GPSManager::formatCoordinate(double coord, bool isLatitude) {
    char hemisphere = isLatitude ? (coord >= 0 ? 'N' : 'S') : (coord >= 0 ? 'E' : 'W');
    coord = abs(coord);

    int degrees = (int)coord;
    double minutes = (coord - degrees) * 60.0;

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%02d°%07.4f'%c", degrees, minutes, hemisphere);
    return String(buffer);
}

float GPSManager::calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    // Haversine formula for distance calculation
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;

    lat1 = lat1 * M_PI / 180.0;
    lat2 = lat2 * M_PI / 180.0;

    double a = sin(dLat/2) * sin(dLat/2) +
               cos(lat1) * cos(lat2) * sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));

    return 6371000 * c; // Distance in meters
}

float GPSManager::calculateBearing(double lat1, double lon1, double lat2, double lon2) {
    // Calculate bearing between two points
    double dLon = (lon2 - lon1) * M_PI / 180.0;

    lat1 = lat1 * M_PI / 180.0;
    lat2 = lat2 * M_PI / 180.0;

    double y = sin(dLon) * cos(lat2);
    double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);

    double bearing = atan2(y, x) * 180.0 / M_PI;
    return fmod(bearing + 360.0, 360.0);
}

void GPSManager::reset() {
    stats.reset();
    resetPosition();
    bufferIndex = 0;
}

void GPSManager::setUpdateRate(uint8_t rateHz) {
    // This could send commands to GPS receiver to change update rate
    // Implementation depends on GPS receiver type (u-blox vs others)
}

void GPSManager::enableDebugOutput(bool enable) {
    // Enable/disable debug output for troubleshooting
    // Could be implemented with a static debug flag
}