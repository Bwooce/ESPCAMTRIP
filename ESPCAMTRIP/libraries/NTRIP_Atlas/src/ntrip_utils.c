/**
 * NTRIP Atlas Utility Functions
 *
 * Distance calculations and other utilities.
 *
 * Copyright (c) 2024 NTRIP Atlas Contributors
 * Licensed under MIT License
 */

#include "ntrip_atlas.h"
#include <math.h>
#include <string.h>

// Earth's radius in kilometers
#define EARTH_RADIUS_KM 6371.0

/**
 * Calculate distance between two coordinates using Haversine formula
 *
 * @param lat1  Latitude of point 1 in decimal degrees
 * @param lon1  Longitude of point 1 in decimal degrees
 * @param lat2  Latitude of point 2 in decimal degrees
 * @param lon2  Longitude of point 2 in decimal degrees
 * @return     Distance in kilometers
 */
double ntrip_atlas_calculate_distance(double lat1, double lon1, double lat2, double lon2) {
    // Convert degrees to radians
    double lat1_rad = lat1 * M_PI / 180.0;
    double lon1_rad = lon1 * M_PI / 180.0;
    double lat2_rad = lat2 * M_PI / 180.0;
    double lon2_rad = lon2 * M_PI / 180.0;

    // Haversine formula
    double dlat = lat2_rad - lat1_rad;
    double dlon = lon2_rad - lon1_rad;

    double a = sin(dlat / 2.0) * sin(dlat / 2.0) +
               cos(lat1_rad) * cos(lat2_rad) *
               sin(dlon / 2.0) * sin(dlon / 2.0);

    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));

    return EARTH_RADIUS_KM * c;
}

/**
 * Get library version string
 */
const char* ntrip_atlas_get_version(void) {
    return "NTRIP Atlas v1.0.0 (Streaming)";
}

/**
 * Get error description string
 */
const char* ntrip_atlas_error_string(ntrip_atlas_error_t error) {
    switch (error) {
        case NTRIP_ATLAS_SUCCESS:
            return "Success";
        case NTRIP_ATLAS_ERROR_INVALID_PARAM:
            return "Invalid parameter";
        case NTRIP_ATLAS_ERROR_NO_SERVICES:
            return "No services available";
        case NTRIP_ATLAS_ERROR_NO_NETWORK:
            return "Network error";
        case NTRIP_ATLAS_ERROR_AUTH_FAILED:
            return "Authentication failed";
        case NTRIP_ATLAS_ERROR_INVALID_RESPONSE:
            return "Invalid response from server";
        case NTRIP_ATLAS_ERROR_DISTANCE_LIMIT:
            return "No services within distance limit";
        case NTRIP_ATLAS_ERROR_NO_MEMORY:
            return "Out of memory";
        case NTRIP_ATLAS_ERROR_TIMEOUT:
            return "Operation timed out";
        case NTRIP_ATLAS_ERROR_PLATFORM:
            return "Platform-specific error";
        case NTRIP_ATLAS_ERROR_SERVICE_FAILED:
            return "Service failed";
        case NTRIP_ATLAS_ERROR_ALL_SERVICES_FAILED:
            return "All services failed";
        default:
            return "Unknown error";
    }
}
