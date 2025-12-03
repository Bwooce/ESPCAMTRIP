/**
 * NTRIP Atlas - Compact Failure Tracking Implementation
 *
 * Memory-optimized failure tracking for ESP32 and embedded systems.
 * Reduces failure storage from 80 bytes to 6 bytes per service (93% reduction).
 *
 * Licensed under MIT License
 */

#include "ntrip_atlas.h"
#include <string.h>
#include <time.h>

// Maximum number of services supported in compact mode
#define NTRIP_COMPACT_MAX_SERVICES 255
#define NTRIP_COMPACT_INVALID_INDEX 255

// Global state for compact failure tracking
static struct {
    ntrip_compact_failure_t failures[NTRIP_COMPACT_MAX_SERVICES];
    const ntrip_service_index_entry_t* service_mapping;
    size_t mapping_count;
    bool initialized;
} g_compact_failure_state = {0};

// Default exponential backoff intervals (seconds)
// 1h, 4h, 12h, 1d, 3d, 1w, 2w, 1month
static const uint32_t g_default_backoff_intervals[8] = {
    3600,     // 1 hour
    14400,    // 4 hours
    43200,    // 12 hours
    86400,    // 1 day
    259200,   // 3 days
    604800,   // 1 week
    1209600,  // 2 weeks
    2629746   // 1 month (30.44 days average)
};

/**
 * Get current time in hours since epoch
 */
static uint32_t get_current_time_hours(void) {
    // Get current time from platform (assuming seconds since epoch)
    uint32_t current_seconds = time(NULL);  // Fallback to standard time()
    return current_seconds / 3600;  // Convert to hours
}

/**
 * Initialize compact failure tracking with service index mapping
 */
ntrip_atlas_error_t ntrip_atlas_init_compact_failure_tracking(
    const ntrip_service_index_entry_t* service_mapping,
    size_t mapping_count
) {
    if (!service_mapping || mapping_count == 0 || mapping_count > NTRIP_COMPACT_MAX_SERVICES) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    // Store service mapping
    g_compact_failure_state.service_mapping = service_mapping;
    g_compact_failure_state.mapping_count = mapping_count;

    // Initialize all failure records to zero
    memset(g_compact_failure_state.failures, 0, sizeof(g_compact_failure_state.failures));

    g_compact_failure_state.initialized = true;

    return NTRIP_ATLAS_SUCCESS;
}

/**
 * Convert service ID string to compact index
 */
uint8_t ntrip_atlas_get_service_index(const char* service_id) {
    if (!g_compact_failure_state.initialized || !service_id) {
        return NTRIP_COMPACT_INVALID_INDEX;
    }

    // Linear search through service mapping
    for (size_t i = 0; i < g_compact_failure_state.mapping_count; i++) {
        if (strcmp(g_compact_failure_state.service_mapping[i].service_id, service_id) == 0) {
            return g_compact_failure_state.service_mapping[i].service_index;
        }
    }

    return NTRIP_COMPACT_INVALID_INDEX;
}

/**
 * Record a service failure using compact storage
 */
ntrip_atlas_error_t ntrip_atlas_record_compact_failure(uint8_t service_index) {
    if (!g_compact_failure_state.initialized || service_index >= NTRIP_COMPACT_MAX_SERVICES) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    ntrip_compact_failure_t* failure = &g_compact_failure_state.failures[service_index];

    // Increment failure count (saturates at 15)
    if (failure->failure_count < 15) {
        failure->failure_count++;
    }

    // Calculate new backoff level (max 8 for our default intervals)
    uint8_t new_backoff_level = failure->failure_count;
    if (new_backoff_level > 8) {
        new_backoff_level = 8;  // Max backoff level
    }
    failure->backoff_level = new_backoff_level;

    // Calculate retry time in hours
    uint32_t backoff_seconds = g_default_backoff_intervals[new_backoff_level - 1];
    uint32_t current_hours = get_current_time_hours();
    uint32_t backoff_hours = (backoff_seconds + 3599) / 3600;  // Round up to next hour
    failure->retry_time_hours = current_hours + backoff_hours;

    return NTRIP_ATLAS_SUCCESS;
}

/**
 * Record service success using compact storage (resets failure count)
 */
ntrip_atlas_error_t ntrip_atlas_record_compact_success(uint8_t service_index) {
    if (!g_compact_failure_state.initialized || service_index >= NTRIP_COMPACT_MAX_SERVICES) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    ntrip_compact_failure_t* failure = &g_compact_failure_state.failures[service_index];

    // Reset failure tracking on success
    failure->failure_count = 0;
    failure->backoff_level = 0;
    failure->retry_time_hours = 0;

    return NTRIP_ATLAS_SUCCESS;
}

/**
 * Check if service is blocked using compact storage
 */
bool ntrip_atlas_is_compact_service_blocked(uint8_t service_index) {
    if (!g_compact_failure_state.initialized || service_index >= NTRIP_COMPACT_MAX_SERVICES) {
        return false;  // If we can't check, assume not blocked
    }

    ntrip_compact_failure_t* failure = &g_compact_failure_state.failures[service_index];

    // No failures = not blocked
    if (failure->failure_count == 0) {
        return false;
    }

    // Check if retry time has passed
    uint32_t current_hours = get_current_time_hours();
    return current_hours < failure->retry_time_hours;
}

/**
 * Get retry time for compact service (hours until retry allowed)
 */
uint32_t ntrip_atlas_get_compact_retry_time_hours(uint8_t service_index) {
    if (!g_compact_failure_state.initialized || service_index >= NTRIP_COMPACT_MAX_SERVICES) {
        return 0;  // Available immediately if we can't check
    }

    ntrip_compact_failure_t* failure = &g_compact_failure_state.failures[service_index];

    if (failure->failure_count == 0) {
        return 0;  // No failures = available immediately
    }

    uint32_t current_hours = get_current_time_hours();
    if (current_hours >= failure->retry_time_hours) {
        return 0;  // Retry time has passed
    }

    return failure->retry_time_hours - current_hours;
}

/**
 * Convert compact failure to full failure structure (for debugging/analysis)
 */
ntrip_atlas_error_t ntrip_atlas_expand_compact_failure(
    const ntrip_compact_failure_t* compact,
    ntrip_service_failure_t* full
) {
    if (!compact || !full) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    // Find service ID from index
    const char* service_id = "unknown";
    if (g_compact_failure_state.initialized) {
        for (size_t i = 0; i < g_compact_failure_state.mapping_count; i++) {
            if (g_compact_failure_state.service_mapping[i].service_index == compact->service_index) {
                service_id = g_compact_failure_state.service_mapping[i].service_id;
                break;
            }
        }
    }

    // Copy/convert fields
    strncpy(full->service_id, service_id, sizeof(full->service_id) - 1);
    full->service_id[sizeof(full->service_id) - 1] = '\0';

    full->failure_count = compact->failure_count;

    // Convert hours back to seconds for compatibility
    full->next_retry_time = compact->retry_time_hours * 3600;

    // Calculate first failure time (estimate)
    if (compact->failure_count > 0 && compact->backoff_level > 0) {
        uint32_t backoff_seconds = g_default_backoff_intervals[compact->backoff_level - 1];
        full->first_failure_time = full->next_retry_time - backoff_seconds;
        full->backoff_seconds = backoff_seconds;
    } else {
        full->first_failure_time = 0;
        full->backoff_seconds = 0;
    }

    return NTRIP_ATLAS_SUCCESS;
}

/**
 * Get backoff seconds from backoff level (utility function)
 */
uint32_t ntrip_atlas_get_backoff_seconds_from_level(uint8_t backoff_level) {
    if (backoff_level == 0 || backoff_level > 8) {
        return 0;
    }

    return g_default_backoff_intervals[backoff_level - 1];
}

/**
 * Filter service list to exclude blocked services during discovery
 * This ensures users get immediate NTRIP data from next-best service
 * instead of waiting for blocked services to become available
 */
size_t ntrip_atlas_filter_blocked_services(
    const ntrip_service_config_t* services,
    size_t service_count,
    ntrip_service_config_t* filtered_services,
    size_t max_filtered
) {
    if (!services || !filtered_services || service_count == 0 || max_filtered == 0) {
        return 0;
    }

    size_t filtered_count = 0;

    for (size_t i = 0; i < service_count && filtered_count < max_filtered; i++) {
        // Check if this service should be skipped due to backoff
        if (!ntrip_atlas_should_skip_service(services[i].provider)) {
            // Copy non-blocked service to filtered array
            filtered_services[filtered_count] = services[i];
            filtered_count++;
        }
    }

    return filtered_count;
}

/**
 * Check if a service should be skipped during discovery due to failure backoff
 * Used internally by discovery algorithms to prioritize available services
 */
bool ntrip_atlas_should_skip_service(const char* service_id) {
    if (!service_id) {
        return false;  // Don't skip if we can't identify the service
    }

    // Get service index
    uint8_t service_index = ntrip_atlas_get_service_index(service_id);
    if (service_index == NTRIP_COMPACT_INVALID_INDEX) {
        // Unknown service - don't skip (might be a new service)
        return false;
    }

    // Check if service is currently blocked
    return ntrip_atlas_is_compact_service_blocked(service_index);
}

/**
 * Get compact failure statistics (debugging utility)
 */
void ntrip_atlas_get_compact_failure_stats(uint32_t* total_failures, uint32_t* blocked_services, uint32_t* memory_used) {
    if (!g_compact_failure_state.initialized) {
        if (total_failures) *total_failures = 0;
        if (blocked_services) *blocked_services = 0;
        if (memory_used) *memory_used = 0;
        return;
    }

    uint32_t failures = 0;
    uint32_t blocked = 0;

    for (size_t i = 0; i < NTRIP_COMPACT_MAX_SERVICES; i++) {
        if (g_compact_failure_state.failures[i].failure_count > 0) {
            failures++;
            if (ntrip_atlas_is_compact_service_blocked((uint8_t)i)) {
                blocked++;
            }
        }
    }

    if (total_failures) *total_failures = failures;
    if (blocked_services) *blocked_services = blocked;
    if (memory_used) *memory_used = sizeof(g_compact_failure_state.failures);
}