#ifndef EXIF_GPS_H
#define EXIF_GPS_H

#include <Arduino.h>

/**
 * Minimal EXIF GPS Writer for ESP32
 *
 * Lightweight implementation to embed GPS coordinates into JPEG files.
 * Focuses on essential GPS tags while minimizing memory usage.
 *
 * Supports:
 * - GPS Latitude/Longitude (decimal degrees)
 * - GPS Altitude (meters)
 * - GPS DateTime
 * - GPS Fix Quality indication
 */

struct GPSEXIFData {
    double latitude;         // Decimal degrees (positive = North)
    double longitude;        // Decimal degrees (positive = East)
    float altitude;          // Meters above sea level
    uint32_t timestamp;      // Unix timestamp
    uint8_t fixQuality;      // GPS fix quality (1=GPS, 4=RTK Fixed, 5=RTK Float)

    GPSEXIFData() : latitude(0), longitude(0), altitude(0), timestamp(0), fixQuality(0) {}
};

class EXIFGPSWriter {
private:
    // JPEG/EXIF constants
    static const uint16_t JPEG_SOI = 0xFFD8;
    static const uint16_t JPEG_APP1 = 0xFFE1;
    static const uint16_t EXIF_HEADER_SIZE = 8;
    static const uint16_t TIFF_HEADER_SIZE = 8;

    // EXIF GPS tag definitions
    static const uint16_t GPS_TAG_VERSION_ID = 0x0000;
    static const uint16_t GPS_TAG_LATITUDE_REF = 0x0001;
    static const uint16_t GPS_TAG_LATITUDE = 0x0002;
    static const uint16_t GPS_TAG_LONGITUDE_REF = 0x0003;
    static const uint16_t GPS_TAG_LONGITUDE = 0x0004;
    static const uint16_t GPS_TAG_ALTITUDE_REF = 0x0005;
    static const uint16_t GPS_TAG_ALTITUDE = 0x0006;
    static const uint16_t GPS_TAG_TIMESTAMP = 0x0007;
    static const uint16_t GPS_TAG_DATESTAMP = 0x001D;

    // TIFF data types
    static const uint16_t TIFF_ASCII = 2;
    static const uint16_t TIFF_SHORT = 3;
    static const uint16_t TIFF_LONG = 4;
    static const uint16_t TIFF_RATIONAL = 5;

    // Helper functions
    static void writeUInt16(uint8_t* buffer, uint16_t value, bool littleEndian = true);
    static void writeUInt32(uint8_t* buffer, uint32_t value, bool littleEndian = true);
    static void writeRational(uint8_t* buffer, uint32_t numerator, uint32_t denominator, bool littleEndian = true);
    static void convertCoordinate(double coord, uint32_t* degrees, uint32_t* minutes, uint32_t* seconds);
    static uint16_t calculateEXIFSize(const GPSEXIFData& gpsData);

public:
    /**
     * Embed GPS data into JPEG frame buffer
     * Modifies the JPEG data in-place by inserting EXIF APP1 segment
     *
     * @param jpegBuffer - Pointer to JPEG data buffer
     * @param jpegSize - Size of JPEG data
     * @param maxBufferSize - Maximum size available in buffer
     * @param gpsData - GPS data to embed
     * @return New size of JPEG with EXIF data, or 0 if failed
     */
    static size_t embedGPSData(uint8_t* jpegBuffer, size_t jpegSize, size_t maxBufferSize, const GPSEXIFData& gpsData);

    /**
     * Check if buffer has enough space for EXIF injection
     *
     * @param jpegSize - Current JPEG size
     * @param maxBufferSize - Maximum buffer size
     * @param gpsData - GPS data to embed
     * @return True if sufficient space available
     */
    static bool hasEnoughSpace(size_t jpegSize, size_t maxBufferSize, const GPSEXIFData& gpsData);

    /**
     * Validate JPEG header for EXIF injection
     *
     * @param jpegBuffer - JPEG data buffer
     * @param jpegSize - JPEG size
     * @return True if valid JPEG format
     */
    static bool validateJPEG(const uint8_t* jpegBuffer, size_t jpegSize);

    /**
     * Get estimated EXIF GPS data size
     *
     * @return Estimated bytes needed for EXIF GPS data
     */
    static uint16_t getEXIFGPSSize() { return 400; } // Conservative estimate
};

#endif // EXIF_GPS_H