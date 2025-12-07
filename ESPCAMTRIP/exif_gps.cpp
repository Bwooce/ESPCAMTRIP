#include "exif_gps.h"
#include <time.h>

// Helper function implementations
void EXIFGPSWriter::writeUInt16(uint8_t* buffer, uint16_t value, bool littleEndian) {
    if (littleEndian) {
        buffer[0] = value & 0xFF;
        buffer[1] = (value >> 8) & 0xFF;
    } else {
        buffer[0] = (value >> 8) & 0xFF;
        buffer[1] = value & 0xFF;
    }
}

void EXIFGPSWriter::writeUInt32(uint8_t* buffer, uint32_t value, bool littleEndian) {
    if (littleEndian) {
        buffer[0] = value & 0xFF;
        buffer[1] = (value >> 8) & 0xFF;
        buffer[2] = (value >> 16) & 0xFF;
        buffer[3] = (value >> 24) & 0xFF;
    } else {
        buffer[0] = (value >> 24) & 0xFF;
        buffer[1] = (value >> 16) & 0xFF;
        buffer[2] = (value >> 8) & 0xFF;
        buffer[3] = value & 0xFF;
    }
}

void EXIFGPSWriter::writeRational(uint8_t* buffer, uint32_t numerator, uint32_t denominator, bool littleEndian) {
    writeUInt32(buffer, numerator, littleEndian);
    writeUInt32(buffer + 4, denominator, littleEndian);
}

void EXIFGPSWriter::convertCoordinate(double coord, uint32_t* degrees, uint32_t* minutes, uint32_t* seconds) {
    double absCoord = fabs(coord);
    *degrees = (uint32_t)absCoord;

    double remainingMinutes = (absCoord - *degrees) * 60.0;
    *minutes = (uint32_t)remainingMinutes;

    double remainingSeconds = (remainingMinutes - *minutes) * 60.0;
    *seconds = (uint32_t)(remainingSeconds * 1000000); // Store as microseconds for precision
}

uint16_t EXIFGPSWriter::calculateEXIFSize(const GPSEXIFData& gpsData) {
    // Conservative estimate for GPS EXIF data
    // APP1 header (2) + APP1 length (2) + EXIF header (6) +
    // TIFF header (8) + IFD0 entries + GPS IFD entries + value data
    return 400; // Should be sufficient for essential GPS tags
}

bool EXIFGPSWriter::validateJPEG(const uint8_t* jpegBuffer, size_t jpegSize) {
    if (jpegSize < 4) return false;

    // Check for JPEG SOI marker
    uint16_t soi = (jpegBuffer[0] << 8) | jpegBuffer[1];
    return (soi == JPEG_SOI);
}

bool EXIFGPSWriter::hasEnoughSpace(size_t jpegSize, size_t maxBufferSize, const GPSEXIFData& gpsData) {
    uint16_t exifSize = calculateEXIFSize(gpsData);
    return (jpegSize + exifSize) <= maxBufferSize;
}

size_t EXIFGPSWriter::embedGPSData(uint8_t* jpegBuffer, size_t jpegSize, size_t maxBufferSize, const GPSEXIFData& gpsData) {
    if (!validateJPEG(jpegBuffer, jpegSize)) {
        Serial.println("EXIF: Invalid JPEG format");
        return 0;
    }

    if (!hasEnoughSpace(jpegSize, maxBufferSize, gpsData)) {
        Serial.println("EXIF: Insufficient buffer space");
        return 0;
    }

    // Calculate EXIF segment size
    uint16_t exifDataSize = calculateEXIFSize(gpsData);
    uint16_t app1Size = exifDataSize + 2; // +2 for size field itself

    // Find insertion point (after SOI marker)
    size_t insertPos = 2; // After JPEG SOI marker

    // Make space for EXIF data by moving existing data
    memmove(jpegBuffer + insertPos + 2 + app1Size,
            jpegBuffer + insertPos,
            jpegSize - insertPos);

    // Write APP1 header
    uint8_t* writePos = jpegBuffer + insertPos;
    writeUInt16(writePos, JPEG_APP1, false); // Big-endian for JPEG markers
    writePos += 2;
    writeUInt16(writePos, app1Size, false); // Big-endian for JPEG length
    writePos += 2;

    // Write EXIF identifier
    memcpy(writePos, "Exif\0\0", 6);
    writePos += 6;

    // Write TIFF header (little-endian)
    writeUInt16(writePos, 0x4949, false); // "II" for little-endian
    writePos += 2;
    writeUInt16(writePos, 0x002A, true); // TIFF magic number
    writePos += 2;
    writeUInt32(writePos, 0x00000008, true); // Offset to first IFD
    writePos += 4;

    // Calculate offsets for GPS IFD and data
    uint8_t* ifd0Start = writePos;
    uint8_t* gpsIfdOffset = ifd0Start + 2 + (1 * 12) + 4; // After IFD0
    uint8_t* gpsDataStart = gpsIfdOffset + 2 + (8 * 12) + 4; // After GPS IFD

    // Write IFD0 (main image directory)
    writeUInt16(writePos, 1, true); // Number of entries in IFD0
    writePos += 2;

    // GPS IFD pointer entry
    writeUInt16(writePos, 0x8825, true); // GPS IFD tag
    writePos += 2;
    writeUInt16(writePos, TIFF_LONG, true); // Type: LONG
    writePos += 2;
    writeUInt32(writePos, 1, true); // Count: 1
    writePos += 4;
    writeUInt32(writePos, (uint32_t)(gpsIfdOffset - ifd0Start + 8), true); // Offset to GPS IFD
    writePos += 4;

    // IFD0 next pointer (none)
    writeUInt32(writePos, 0, true);
    writePos += 4;

    // Write GPS IFD
    writePos = gpsIfdOffset;
    writeUInt16(writePos, 8, true); // Number of GPS entries
    writePos += 2;

    uint32_t dataOffset = (uint32_t)(gpsDataStart - ifd0Start + 8);

    // GPS Version ID
    writeUInt16(writePos, GPS_TAG_VERSION_ID, true); // Tag
    writePos += 2;
    writeUInt16(writePos, 1, true); // Type: BYTE (but stored as SHORT for simplicity)
    writePos += 2;
    writeUInt32(writePos, 4, true); // Count: 4 bytes
    writePos += 4;
    writeUInt32(writePos, dataOffset, true); // Offset to version data
    writePos += 4;
    dataOffset += 4;

    // GPS Latitude Reference
    writeUInt16(writePos, GPS_TAG_LATITUDE_REF, true);
    writePos += 2;
    writeUInt16(writePos, TIFF_ASCII, true);
    writePos += 2;
    writeUInt32(writePos, 2, true); // Count: 2 chars
    writePos += 4;
    writeUInt32(writePos, dataOffset, true);
    writePos += 4;
    dataOffset += 2;

    // GPS Latitude
    writeUInt16(writePos, GPS_TAG_LATITUDE, true);
    writePos += 2;
    writeUInt16(writePos, TIFF_RATIONAL, true);
    writePos += 2;
    writeUInt32(writePos, 3, true); // Count: 3 rationals (deg, min, sec)
    writePos += 4;
    writeUInt32(writePos, dataOffset, true);
    writePos += 4;
    dataOffset += 24; // 3 rationals * 8 bytes each

    // GPS Longitude Reference
    writeUInt16(writePos, GPS_TAG_LONGITUDE_REF, true);
    writePos += 2;
    writeUInt16(writePos, TIFF_ASCII, true);
    writePos += 2;
    writeUInt32(writePos, 2, true);
    writePos += 4;
    writeUInt32(writePos, dataOffset, true);
    writePos += 4;
    dataOffset += 2;

    // GPS Longitude
    writeUInt16(writePos, GPS_TAG_LONGITUDE, true);
    writePos += 2;
    writeUInt16(writePos, TIFF_RATIONAL, true);
    writePos += 2;
    writeUInt32(writePos, 3, true);
    writePos += 4;
    writeUInt32(writePos, dataOffset, true);
    writePos += 4;
    dataOffset += 24;

    // GPS Altitude Reference
    writeUInt16(writePos, GPS_TAG_ALTITUDE_REF, true);
    writePos += 2;
    writeUInt16(writePos, 1, true); // Type: BYTE
    writePos += 2;
    writeUInt32(writePos, 1, true);
    writePos += 4;
    writeUInt32(writePos, 0, true); // 0 = above sea level
    writePos += 4;

    // GPS Altitude
    writeUInt16(writePos, GPS_TAG_ALTITUDE, true);
    writePos += 2;
    writeUInt16(writePos, TIFF_RATIONAL, true);
    writePos += 2;
    writeUInt32(writePos, 1, true);
    writePos += 4;
    writeUInt32(writePos, dataOffset, true);
    writePos += 4;
    dataOffset += 8;

    // GPS DateTime Stamp
    writeUInt16(writePos, GPS_TAG_DATESTAMP, true);
    writePos += 2;
    writeUInt16(writePos, TIFF_ASCII, true);
    writePos += 2;
    writeUInt32(writePos, 11, true); // "YYYY:MM:DD\0"
    writePos += 4;
    writeUInt32(writePos, dataOffset, true);
    writePos += 4;
    dataOffset += 11;

    // GPS IFD next pointer (none)
    writeUInt32(writePos, 0, true);
    writePos += 4;

    // Write GPS data values
    writePos = gpsDataStart;

    // GPS Version (2.2.0.0)
    writePos[0] = 2;
    writePos[1] = 2;
    writePos[2] = 0;
    writePos[3] = 0;
    writePos += 4;

    // Latitude reference
    writePos[0] = (gpsData.latitude >= 0) ? 'N' : 'S';
    writePos[1] = '\0';
    writePos += 2;

    // Latitude degrees, minutes, seconds
    uint32_t latDeg, latMin, latSec;
    convertCoordinate(gpsData.latitude, &latDeg, &latMin, &latSec);
    writeRational(writePos, latDeg, 1, true);
    writePos += 8;
    writeRational(writePos, latMin, 1, true);
    writePos += 8;
    writeRational(writePos, latSec, 1000000, true); // Microsecond precision
    writePos += 8;

    // Longitude reference
    writePos[0] = (gpsData.longitude >= 0) ? 'E' : 'W';
    writePos[1] = '\0';
    writePos += 2;

    // Longitude degrees, minutes, seconds
    uint32_t lonDeg, lonMin, lonSec;
    convertCoordinate(gpsData.longitude, &lonDeg, &lonMin, &lonSec);
    writeRational(writePos, lonDeg, 1, true);
    writePos += 8;
    writeRational(writePos, lonMin, 1, true);
    writePos += 8;
    writeRational(writePos, lonSec, 1000000, true);
    writePos += 8;

    // Altitude (meters, stored as rational)
    uint32_t altitudeInt = (uint32_t)(gpsData.altitude * 1000); // mm precision
    writeRational(writePos, altitudeInt, 1000, true);
    writePos += 8;

    // Date stamp
    if (gpsData.timestamp > 0) {
        struct tm* timeinfo = gmtime((time_t*)&gpsData.timestamp);
        snprintf((char*)writePos, 11, "%04d:%02d:%02d",
                timeinfo->tm_year + 1900,
                timeinfo->tm_mon + 1,
                timeinfo->tm_mday);
    } else {
        strcpy((char*)writePos, "0000:00:00");
    }
    writePos += 11;

    // Return new JPEG size
    return jpegSize + 2 + app1Size;
}