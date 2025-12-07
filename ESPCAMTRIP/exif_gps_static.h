#ifndef EXIF_GPS_STATIC_H
#define EXIF_GPS_STATIC_H

#include <Arduino.h>

/**
 * Static EXIF GPS Writer for ESP32 (Memory Optimized)
 *
 * Based on ESP32-CAM_Interval approach with static allocation
 * Pre-builds complete TIFF/EXIF structure for in-place GPS updates
 *
 * Advantages:
 * - Zero heap allocation during capture
 * - Predictable memory footprint
 * - Fast GPS coordinate updates (~10ms vs 200ms)
 * - No memory fragmentation
 */

// TIFF/EXIF constants
#define TIFF_LITTLE_ENDIAN 0x4949
#define TIFF_MAGIC 0x002A
#define TIFF_RATIONAL 5
#define TIFF_ASCII 2
#define TIFF_BYTE 1
#define TIFF_SHORT 3
#define TIFF_LONG 4

// GPS coordinate precision
#define GPS_COORD_SCALE 1000000  // 6 decimal places
#define GPS_ALT_SCALE 1000       // 3 decimal places

// JPEG/EXIF header structure (pre-allocated)
struct __attribute__((packed)) StaticEXIFHeader {
    // JPEG APP1 marker
    uint16_t app1_marker;        // 0xFFE1
    uint16_t app1_length;        // Length of APP1 segment

    // EXIF identifier
    char exif_id[6];             // "Exif\0\0"

    // TIFF header
    uint16_t tiff_byte_order;    // 0x4949 (little endian)
    uint16_t tiff_magic;         // 0x002A
    uint32_t ifd0_offset;        // Offset to IFD0 (8)

    // IFD0 (Image File Directory)
    uint16_t ifd0_count;         // Number of entries

    // IFD0 Entry 1: Camera make
    uint16_t make_tag;           // 0x010F
    uint16_t make_type;          // TIFF_ASCII
    uint32_t make_count;         // Length of make string
    uint32_t make_offset;        // Offset to make string

    // IFD0 Entry 2: Camera model
    uint16_t model_tag;          // 0x0110
    uint16_t model_type;         // TIFF_ASCII
    uint32_t model_count;        // Length of model string
    uint32_t model_offset;       // Offset to model string

    // IFD0 Entry 3: DateTime
    uint16_t datetime_tag;       // 0x0132
    uint16_t datetime_type;      // TIFF_ASCII
    uint32_t datetime_count;     // 20 ("YYYY:MM:DD HH:MM:SS\0")
    uint32_t datetime_offset;    // Offset to datetime string

    // IFD0 Entry 4: GPS IFD pointer
    uint16_t gps_ifd_tag;        // 0x8825
    uint16_t gps_ifd_type;       // TIFF_LONG
    uint32_t gps_ifd_count;      // 1
    uint32_t gps_ifd_offset;     // Offset to GPS IFD

    // Next IFD pointer (none)
    uint32_t next_ifd;           // 0

    // GPS IFD
    uint16_t gps_entry_count;    // Number of GPS entries (8)

    // GPS Entry 1: Version
    uint16_t gps_version_tag;    // 0x0000
    uint16_t gps_version_type;   // TIFF_BYTE
    uint32_t gps_version_count;  // 4
    uint32_t gps_version_data;   // Version bytes (2,2,0,0)

    // GPS Entry 2: Latitude reference
    uint16_t gps_lat_ref_tag;    // 0x0001
    uint16_t gps_lat_ref_type;   // TIFF_ASCII
    uint32_t gps_lat_ref_count;  // 2
    uint32_t gps_lat_ref_offset; // Offset to lat ref ("N" or "S")

    // GPS Entry 3: Latitude
    uint16_t gps_lat_tag;        // 0x0002
    uint16_t gps_lat_type;       // TIFF_RATIONAL
    uint32_t gps_lat_count;      // 3 (deg, min, sec)
    uint32_t gps_lat_offset;     // Offset to latitude rationals

    // GPS Entry 4: Longitude reference
    uint16_t gps_lon_ref_tag;    // 0x0003
    uint16_t gps_lon_ref_type;   // TIFF_ASCII
    uint32_t gps_lon_ref_count;  // 2
    uint32_t gps_lon_ref_offset; // Offset to lon ref ("E" or "W")

    // GPS Entry 5: Longitude
    uint16_t gps_lon_tag;        // 0x0004
    uint16_t gps_lon_type;       // TIFF_RATIONAL
    uint32_t gps_lon_count;      // 3 (deg, min, sec)
    uint32_t gps_lon_offset;     // Offset to longitude rationals

    // GPS Entry 6: Altitude reference
    uint16_t gps_alt_ref_tag;    // 0x0005
    uint16_t gps_alt_ref_type;   // TIFF_BYTE
    uint32_t gps_alt_ref_count;  // 1
    uint32_t gps_alt_ref_data;   // 0 (above sea level)

    // GPS Entry 7: Altitude
    uint16_t gps_alt_tag;        // 0x0006
    uint16_t gps_alt_type;       // TIFF_RATIONAL
    uint32_t gps_alt_count;      // 1
    uint32_t gps_alt_offset;     // Offset to altitude rational

    // GPS Entry 8: Timestamp
    uint16_t gps_time_tag;       // 0x0007
    uint16_t gps_time_type;      // TIFF_RATIONAL
    uint32_t gps_time_count;     // 3 (hour, min, sec)
    uint32_t gps_time_offset;    // Offset to time rationals

    // GPS IFD next pointer (none)
    uint32_t gps_next_ifd;       // 0

    // String data area
    char camera_make[16];        // "XIAO ESP32S3\0"
    char camera_model[16];       // "OV2640\0"
    char datetime[20];           // "YYYY:MM:DD HH:MM:SS\0"
    char lat_ref[2];            // "N\0" or "S\0"
    char lon_ref[2];            // "E\0" or "W\0"

    // GPS rational data (3 rationals each = 24 bytes)
    uint32_t lat_degrees[2];     // degrees rational
    uint32_t lat_minutes[2];     // minutes rational
    uint32_t lat_seconds[2];     // seconds rational

    uint32_t lon_degrees[2];     // degrees rational
    uint32_t lon_minutes[2];     // minutes rational
    uint32_t lon_seconds[2];     // seconds rational

    uint32_t altitude[2];        // altitude rational

    uint32_t time_hour[2];       // hour rational
    uint32_t time_minute[2];     // minute rational
    uint32_t time_second[2];     // second rational
};

class StaticEXIFGPS {
private:
    static StaticEXIFHeader exif_header;
    static bool initialized;

    static void initializeHeader();
    static uint32_t calculateOffset(void* field);

public:
    /**
     * Initialize static EXIF header
     * Call once during system startup
     */
    static bool init();

    /**
     * Update GPS coordinates in pre-allocated header
     * Fast in-place update for capture performance
     *
     * @param latitude Latitude in decimal degrees
     * @param longitude Longitude in decimal degrees
     * @param altitude Altitude in meters
     * @param timestamp Unix timestamp (0 = use current time)
     * @param fix_quality GPS fix quality indicator
     */
    static void updateGPS(double latitude, double longitude, float altitude,
                         uint32_t timestamp = 0, uint8_t fix_quality = 1);

    /**
     * Embed GPS EXIF data into JPEG
     * Zero allocation - direct memory copy
     *
     * @param jpeg_buffer JPEG data buffer
     * @param jpeg_size Current JPEG size
     * @param max_buffer_size Maximum buffer capacity
     * @return New JPEG size with EXIF, or 0 if failed
     */
    static size_t embedIntoJPEG(uint8_t* jpeg_buffer, size_t jpeg_size, size_t max_buffer_size);

    /**
     * Get EXIF header size
     */
    static size_t getHeaderSize() { return sizeof(StaticEXIFHeader); }

    /**
     * Check if GPS data has been set
     */
    static bool hasValidGPS();

    /**
     * Get current GPS coordinates from header
     */
    static void getCurrentGPS(double* lat, double* lon, float* alt);
};

#endif // EXIF_GPS_STATIC_H