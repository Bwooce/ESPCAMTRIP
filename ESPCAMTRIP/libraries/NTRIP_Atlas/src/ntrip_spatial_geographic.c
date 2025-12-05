/**
 * NTRIP Atlas Spatial-Geographic Integration
 *
 * Combines O(1) spatial indexing with precise geographic bounds checking
 * to solve the "German problem" - ensuring services only appear in areas
 * where they actually provide coverage.
 *
 * This addresses the issue where spatial indexing tiles are larger than
 * individual service coverage areas, causing services to appear in regions
 * outside their actual coverage boundaries.
 */

#include "ntrip_atlas.h"
#include <string.h>
#include <stdbool.h>

/**
 * Find services using spatial indexing with geographic bounds validation
 *
 * This function combines:
 * 1. Fast O(1) spatial indexing to get candidate services
 * 2. Precise geographic bounds checking to verify actual coverage
 *
 * @param user_lat User latitude
 * @param user_lon User longitude
 * @param services Service database array
 * @param service_count Total number of services in database
 * @param found_services Output array for service indices
 * @param max_services Maximum services to return
 * @return Number of services found with actual coverage
 */
size_t ntrip_atlas_find_services_spatial_geographic(
    double user_lat,
    double user_lon,
    const ntrip_service_compact_t* services,
    size_t service_count,
    uint8_t* found_services,
    size_t max_services
) {
    if (!found_services || !services || service_count == 0) {
        return 0;
    }

    // Step 1: Get candidate services from spatial indexing (O(1) fast lookup)
    uint8_t spatial_candidates[16];  // Reasonable buffer for tile services
    size_t spatial_count = ntrip_atlas_find_services_by_location_fast(
        user_lat, user_lon, spatial_candidates, 16
    );

    if (spatial_count == 0) {
        return 0; // No spatial candidates found
    }

    // Step 2: Filter candidates using geographic bounds checking
    size_t verified_count = 0;

    for (size_t i = 0; i < spatial_count && verified_count < max_services; i++) {
        uint8_t service_idx = spatial_candidates[i];

        // Validate service index is within bounds
        if (service_idx >= service_count) {
            continue; // Invalid service index
        }

        // Check if user location is within this service's actual coverage
        bool within_coverage = ntrip_atlas_is_location_within_service_coverage(
            &services[service_idx], user_lat, user_lon
        );

        if (within_coverage) {
            found_services[verified_count++] = service_idx;
        }
    }

    return verified_count;
}

/**
 * Find best service using spatial-geographic lookup
 *
 * This is a higher-level convenience function that finds the best service
 * from those with verified coverage at the user's location.
 *
 * @param user_lat User latitude
 * @param user_lon User longitude
 * @param services Service database array
 * @param service_count Total number of services
 * @param best_service Output for best service
 * @return Error code
 */
ntrip_atlas_error_t ntrip_atlas_find_best_service_spatial_geographic(
    double user_lat,
    double user_lon,
    const ntrip_service_compact_t* services,
    size_t service_count,
    ntrip_service_compact_t* best_service
) {
    if (!best_service || !services) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    uint8_t found_services[8];
    size_t found_count = ntrip_atlas_find_services_spatial_geographic(
        user_lat, user_lon, services, service_count, found_services, 8
    );

    if (found_count == 0) {
        return NTRIP_ATLAS_ERROR_NO_SERVICES;
    }

    // Find best service based on quality rating and distance
    double best_score = -1.0;
    uint8_t best_index = 0;

    for (size_t i = 0; i < found_count; i++) {
        uint8_t service_idx = found_services[i];
        const ntrip_service_compact_t* service = &services[service_idx];

        // Calculate distance to service center
        double distance = ntrip_atlas_calculate_distance_to_service_center(
            service, user_lat, user_lon
        );

        // Calculate composite score (higher is better)
        // Quality rating: 1-5, Distance penalty: 0-100km normalized
        double quality_score = service->quality_rating * 20.0;  // Max 100 points
        double distance_penalty = (distance > 100.0) ? 100.0 : distance;  // Cap at 100km
        double score = quality_score - distance_penalty;

        if (i == 0 || score > best_score) {
            best_score = score;
            best_index = service_idx;
        }
    }

    // Copy best service to output
    *best_service = services[best_index];
    return NTRIP_ATLAS_SUCCESS;
}

/**
 * Get statistics about spatial-geographic lookup performance
 *
 * This function helps analyze the effectiveness of the integrated approach
 * by comparing spatial candidates vs verified services.
 *
 * @param user_lat User latitude
 * @param user_lon User longitude
 * @param services Service database
 * @param service_count Total number of services
 * @param spatial_candidates Output: services found by spatial indexing
 * @param verified_services Output: services with verified coverage
 * @return Error code
 */
ntrip_atlas_error_t ntrip_atlas_get_spatial_geographic_stats(
    double user_lat,
    double user_lon,
    const ntrip_service_compact_t* services,
    size_t service_count,
    size_t* spatial_candidates,
    size_t* verified_services
) {
    if (!spatial_candidates || !verified_services || !services) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    // Get spatial indexing candidates
    uint8_t candidates[16];
    *spatial_candidates = ntrip_atlas_find_services_by_location_fast(
        user_lat, user_lon, candidates, 16
    );

    // Count how many have verified coverage
    *verified_services = 0;
    for (size_t i = 0; i < *spatial_candidates; i++) {
        uint8_t service_idx = candidates[i];

        if (service_idx < service_count) {
            bool within_coverage = ntrip_atlas_is_location_within_service_coverage(
                &services[service_idx], user_lat, user_lon
            );

            if (within_coverage) {
                (*verified_services)++;
            }
        }
    }

    return NTRIP_ATLAS_SUCCESS;
}