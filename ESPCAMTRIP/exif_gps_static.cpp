#include "exif_gps_static.h"
#include <time.h>

// Static member definitions
StaticEXIFHeader StaticEXIFGPS::exif_header;
bool StaticEXIFGPS::initialized = false;

bool StaticEXIFGPS::init() {
    Serial.println("Initializing Static EXIF GPS...");
    initializeHeader();
    initialized = true;
    Serial.printf("Static EXIF GPS initialized, header size: %u bytes\n", sizeof(StaticEXIFHeader));
    return true;
}

void StaticEXIFGPS::initializeHeader() {
    memset(&exif_header, 0, sizeof(StaticEXIFHeader));

    // JPEG APP1 marker
    exif_header.app1_marker = 0xE1FF;  // Little endian: 0xFFE1
    exif_header.app1_length = sizeof(StaticEXIFHeader) - 2; // Don't count marker itself

    // EXIF identifier
    strcpy(exif_header.exif_id, "Exif");

    // TIFF header
    exif_header.tiff_byte_order = TIFF_LITTLE_ENDIAN;
    exif_header.tiff_magic = TIFF_MAGIC;
    exif_header.ifd0_offset = 8;

    // IFD0 setup
    exif_header.ifd0_count = 4; // make, model, datetime, gps_ifd

    // IFD0 Entry 1: Camera make
    exif_header.make_tag = 0x010F;
    exif_header.make_type = TIFF_ASCII;
    exif_header.make_count = 13; // "XIAO ESP32S3\0"
    exif_header.make_offset = calculateOffset(exif_header.camera_make);

    // IFD0 Entry 2: Camera model
    exif_header.model_tag = 0x0110;
    exif_header.model_type = TIFF_ASCII;
    exif_header.model_count = 7; // "OV2640\0"
    exif_header.model_offset = calculateOffset(exif_header.camera_model);

    // IFD0 Entry 3: DateTime
    exif_header.datetime_tag = 0x0132;
    exif_header.datetime_type = TIFF_ASCII;
    exif_header.datetime_count = 20; // "YYYY:MM:DD HH:MM:SS\0"
    exif_header.datetime_offset = calculateOffset(exif_header.datetime);

    // IFD0 Entry 4: GPS IFD pointer
    exif_header.gps_ifd_tag = 0x8825;
    exif_header.gps_ifd_type = TIFF_LONG;
    exif_header.gps_ifd_count = 1;
    exif_header.gps_ifd_offset = calculateOffset(&exif_header.gps_entry_count);

    // Next IFD pointer (none)
    exif_header.next_ifd = 0;

    // GPS IFD setup
    exif_header.gps_entry_count = 8;

    // GPS Entry 1: Version
    exif_header.gps_version_tag = 0x0000;
    exif_header.gps_version_type = TIFF_BYTE;
    exif_header.gps_version_count = 4;
    exif_header.gps_version_data = 0x00020200; // Version 2.2.0.0

    // GPS Entry 2: Latitude reference
    exif_header.gps_lat_ref_tag = 0x0001;
    exif_header.gps_lat_ref_type = TIFF_ASCII;
    exif_header.gps_lat_ref_count = 2;
    exif_header.gps_lat_ref_offset = calculateOffset(exif_header.lat_ref);

    // GPS Entry 3: Latitude
    exif_header.gps_lat_tag = 0x0002;
    exif_header.gps_lat_type = TIFF_RATIONAL;
    exif_header.gps_lat_count = 3;
    exif_header.gps_lat_offset = calculateOffset(exif_header.lat_degrees);

    // GPS Entry 4: Longitude reference
    exif_header.gps_lon_ref_tag = 0x0003;
    exif_header.gps_lon_ref_type = TIFF_ASCII;
    exif_header.gps_lon_ref_count = 2;
    exif_header.gps_lon_ref_offset = calculateOffset(exif_header.lon_ref);

    // GPS Entry 5: Longitude
    exif_header.gps_lon_tag = 0x0004;
    exif_header.gps_lon_type = TIFF_RATIONAL;
    exif_header.gps_lon_count = 3;
    exif_header.gps_lon_offset = calculateOffset(exif_header.lon_degrees);

    // GPS Entry 6: Altitude reference
    exif_header.gps_alt_ref_tag = 0x0005;
    exif_header.gps_alt_ref_type = TIFF_BYTE;
    exif_header.gps_alt_ref_count = 1;
    exif_header.gps_alt_ref_data = 0; // Above sea level

    // GPS Entry 7: Altitude
    exif_header.gps_alt_tag = 0x0006;
    exif_header.gps_alt_type = TIFF_RATIONAL;
    exif_header.gps_alt_count = 1;
    exif_header.gps_alt_offset = calculateOffset(exif_header.altitude);

    // GPS Entry 8: Timestamp
    exif_header.gps_time_tag = 0x0007;
    exif_header.gps_time_type = TIFF_RATIONAL;
    exif_header.gps_time_count = 3;
    exif_header.gps_time_offset = calculateOffset(exif_header.time_hour);

    // GPS IFD next pointer (none)
    exif_header.gps_next_ifd = 0;

    // Initialize string data
    strcpy(exif_header.camera_make, "XIAO ESP32S3");
    strcpy(exif_header.camera_model, "OV2640");
    strcpy(exif_header.datetime, "0000:00:00 00:00:00"); // Will be updated
    strcpy(exif_header.lat_ref, "N");
    strcpy(exif_header.lon_ref, "E");

    // Initialize GPS coordinates to zero
    memset(exif_header.lat_degrees, 0, sizeof(exif_header.lat_degrees));
    memset(exif_header.lat_minutes, 0, sizeof(exif_header.lat_minutes));
    memset(exif_header.lat_seconds, 0, sizeof(exif_header.lat_seconds));
    memset(exif_header.lon_degrees, 0, sizeof(exif_header.lon_degrees));
    memset(exif_header.lon_minutes, 0, sizeof(exif_header.lon_minutes));
    memset(exif_header.lon_seconds, 0, sizeof(exif_header.lon_seconds));
    memset(exif_header.altitude, 0, sizeof(exif_header.altitude));
    memset(exif_header.time_hour, 0, sizeof(exif_header.time_hour));
    memset(exif_header.time_minute, 0, sizeof(exif_header.time_minute));
    memset(exif_header.time_second, 0, sizeof(exif_header.time_second));
}

uint32_t StaticEXIFGPS::calculateOffset(void* field) {
    // Calculate offset from start of TIFF header (after EXIF identifier)
    uint8_t* base = (uint8_t*)&exif_header.tiff_byte_order;
    uint8_t* target = (uint8_t*)field;
    return (uint32_t)(target - base);
}

void StaticEXIFGPS::updateGPS(double latitude, double longitude, float altitude,
                             uint32_t timestamp, uint8_t fix_quality) {
    if (!initialized) return;

    // Update latitude
    double abs_lat = fabs(latitude);
    uint32_t lat_deg = (uint32_t)abs_lat;
    double lat_min_float = (abs_lat - lat_deg) * 60.0;
    uint32_t lat_min = (uint32_t)lat_min_float;
    uint32_t lat_sec = (uint32_t)((lat_min_float - lat_min) * 60.0 * GPS_COORD_SCALE);

    exif_header.lat_degrees[0] = lat_deg;
    exif_header.lat_degrees[1] = 1;
    exif_header.lat_minutes[0] = lat_min;
    exif_header.lat_minutes[1] = 1;
    exif_header.lat_seconds[0] = lat_sec;
    exif_header.lat_seconds[1] = GPS_COORD_SCALE;

    exif_header.lat_ref[0] = (latitude >= 0) ? 'N' : 'S';
    exif_header.lat_ref[1] = '\0';

    // Update longitude
    double abs_lon = fabs(longitude);
    uint32_t lon_deg = (uint32_t)abs_lon;
    double lon_min_float = (abs_lon - lon_deg) * 60.0;
    uint32_t lon_min = (uint32_t)lon_min_float;
    uint32_t lon_sec = (uint32_t)((lon_min_float - lon_min) * 60.0 * GPS_COORD_SCALE);

    exif_header.lon_degrees[0] = lon_deg;
    exif_header.lon_degrees[1] = 1;
    exif_header.lon_minutes[0] = lon_min;
    exif_header.lon_minutes[1] = 1;
    exif_header.lon_seconds[0] = lon_sec;
    exif_header.lon_seconds[1] = GPS_COORD_SCALE;

    exif_header.lon_ref[0] = (longitude >= 0) ? 'E' : 'W';
    exif_header.lon_ref[1] = '\0';

    // Update altitude
    uint32_t alt_scaled = (uint32_t)(fabs(altitude) * GPS_ALT_SCALE);
    exif_header.altitude[0] = alt_scaled;
    exif_header.altitude[1] = GPS_ALT_SCALE;

    // Update timestamp
    time_t time_val = (timestamp > 0) ? timestamp : time(nullptr);
    struct tm* gmt = gmtime(&time_val);

    if (gmt) {
        // Update datetime string
        sprintf(exif_header.datetime, "%04d:%02d:%02d %02d:%02d:%02d",
                gmt->tm_year + 1900, gmt->tm_mon + 1, gmt->tm_mday,
                gmt->tm_hour, gmt->tm_min, gmt->tm_sec);

        // Update GPS time rationals
        exif_header.time_hour[0] = gmt->tm_hour;
        exif_header.time_hour[1] = 1;
        exif_header.time_minute[0] = gmt->tm_min;
        exif_header.time_minute[1] = 1;
        exif_header.time_second[0] = gmt->tm_sec;
        exif_header.time_second[1] = 1;
    }

    Serial.printf("EXIF GPS updated: %.6f, %.6f @ %.1fm\n", latitude, longitude, altitude);
}

size_t StaticEXIFGPS::embedIntoJPEG(uint8_t* jpeg_buffer, size_t jpeg_size, size_t max_buffer_size) {
    if (!initialized || !jpeg_buffer || jpeg_size < 4) {
        return 0;
    }

    // Check JPEG SOI marker
    if (jpeg_buffer[0] != 0xFF || jpeg_buffer[1] != 0xD8) {
        Serial.println("Invalid JPEG format for EXIF embedding");
        return 0;
    }

    // Check buffer space
    size_t header_size = sizeof(StaticEXIFHeader);
    if (jpeg_size + header_size > max_buffer_size) {
        Serial.println("Insufficient buffer space for EXIF embedding");
        return 0;
    }

    // Make space for EXIF header (insert after SOI)
    memmove(jpeg_buffer + 2 + header_size, jpeg_buffer + 2, jpeg_size - 2);

    // Copy EXIF header
    memcpy(jpeg_buffer + 2, &exif_header, header_size);

    Serial.printf("Static EXIF embedded: %u bytes added\n", header_size);
    return jpeg_size + header_size;
}

bool StaticEXIFGPS::hasValidGPS() {
    // Check if latitude or longitude is non-zero
    return (exif_header.lat_degrees[0] != 0 || exif_header.lon_degrees[0] != 0);
}

void StaticEXIFGPS::getCurrentGPS(double* lat, double* lon, float* alt) {
    if (!lat || !lon || !alt) return;

    // Reconstruct latitude
    *lat = exif_header.lat_degrees[0] +
           (double)exif_header.lat_minutes[0] / 60.0 +
           (double)exif_header.lat_seconds[0] / GPS_COORD_SCALE / 3600.0;
    if (exif_header.lat_ref[0] == 'S') *lat = -*lat;

    // Reconstruct longitude
    *lon = exif_header.lon_degrees[0] +
           (double)exif_header.lon_minutes[0] / 60.0 +
           (double)exif_header.lon_seconds[0] / GPS_COORD_SCALE / 3600.0;
    if (exif_header.lon_ref[0] == 'W') *lon = -*lon;

    // Reconstruct altitude
    *alt = (float)exif_header.altitude[0] / GPS_ALT_SCALE;
}