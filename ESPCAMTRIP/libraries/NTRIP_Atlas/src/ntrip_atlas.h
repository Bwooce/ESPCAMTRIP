/**
 * NTRIP Atlas - Global NTRIP Service Discovery Library
 *
 * Copyright (c) 2024 NTRIP Atlas Contributors
 * Licensed under MIT License
 *
 * Platform-agnostic C library for automatic NTRIP service discovery,
 * intelligent base station selection, and secure credential management.
 */

#ifndef NTRIP_ATLAS_H
#define NTRIP_ATLAS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Library version
#define NTRIP_ATLAS_VERSION_MAJOR 1
#define NTRIP_ATLAS_VERSION_MINOR 1
#define NTRIP_ATLAS_VERSION_PATCH 0

// Configuration limits
#define NTRIP_ATLAS_MAX_URL_LEN     128
#define NTRIP_ATLAS_MAX_MOUNTPOINT  32
#define NTRIP_ATLAS_MAX_USERNAME    64
#define NTRIP_ATLAS_MAX_PASSWORD    64
#define NTRIP_ATLAS_MAX_PROVIDER    64
#define NTRIP_ATLAS_MAX_COUNTRY     4
#define NTRIP_ATLAS_MAX_FORMAT      32
#define NTRIP_ATLAS_MAX_DETAILS     128

// Memory allocation strategy
#ifdef NTRIP_ATLAS_STATIC_ALLOCATION
    #define NTRIP_ATLAS_MAX_SERVICES    64
    #define NTRIP_ATLAS_MAX_MOUNTPOINTS 256
#endif

/**
 * Error codes
 */
typedef enum {
    NTRIP_ATLAS_SUCCESS = 0,
    NTRIP_ATLAS_ERROR_INVALID_PARAM = -1,
    NTRIP_ATLAS_ERROR_NO_SERVICES = -2,
    NTRIP_ATLAS_ERROR_NO_NETWORK = -3,
    NTRIP_ATLAS_ERROR_AUTH_FAILED = -4,
    NTRIP_ATLAS_ERROR_INVALID_RESPONSE = -5,
    NTRIP_ATLAS_ERROR_DISTANCE_LIMIT = -6,
    NTRIP_ATLAS_ERROR_NO_MEMORY = -7,
    NTRIP_ATLAS_ERROR_TIMEOUT = -8,
    NTRIP_ATLAS_ERROR_PLATFORM = -9,
    NTRIP_ATLAS_ERROR_SERVICE_FAILED = -10,
    NTRIP_ATLAS_ERROR_ALL_SERVICES_FAILED = -11,
    NTRIP_ATLAS_ERROR_NOT_FOUND = -12,
    // Database versioning errors
    NTRIP_ATLAS_ERROR_INVALID_MAGIC = -13,       // Database magic number mismatch
    NTRIP_ATLAS_ERROR_VERSION_TOO_OLD = -14,     // Library version too old for database
    NTRIP_ATLAS_ERROR_INCOMPATIBLE_VERSION = -15, // Incompatible schema version
    NTRIP_ATLAS_ERROR_MISSING_FEATURE = -16,     // Required feature not supported
    // Tiered loading errors
    NTRIP_ATLAS_ERROR_NO_DISCOVERY_INDEX = -17,  // Discovery index not loaded
    NTRIP_ATLAS_ERROR_NO_ENDPOINTS = -18,        // Service endpoints not available
    NTRIP_ATLAS_ERROR_NO_METADATA = -19,         // Service metadata not available
    NTRIP_ATLAS_ERROR_LOAD_FAILED = -20,         // Data loading operation failed
    NTRIP_ATLAS_ERROR_SPATIAL_INDEX_FULL = -21,  // Maximum number of tiles reached
    NTRIP_ATLAS_ERROR_TILE_FULL = -22            // Maximum services per tile reached
} ntrip_atlas_error_t;

/**
 * Service network types
 */
typedef enum {
    NTRIP_NETWORK_GOVERNMENT = 0,
    NTRIP_NETWORK_COMMERCIAL = 1,
    NTRIP_NETWORK_COMMUNITY = 2,
    NTRIP_NETWORK_RESEARCH = 3
} ntrip_network_type_t;

/**
 * Payment priority configuration for service discovery
 */
typedef enum {
    NTRIP_PAYMENT_PRIORITY_FREE_FIRST = 0,  // Try free services first, paid services as fallback
    NTRIP_PAYMENT_PRIORITY_PAID_FIRST = 1   // Try paid services first, free services as fallback
} ntrip_payment_priority_t;

/**
 * Authentication methods
 */
typedef enum {
    NTRIP_AUTH_NONE = 0,
    NTRIP_AUTH_BASIC = 1,
    NTRIP_AUTH_DIGEST = 2
} ntrip_auth_method_t;

/**
 * Service configuration (compiled-in static data)
 */
typedef struct {
    char provider[NTRIP_ATLAS_MAX_PROVIDER];
    char country[NTRIP_ATLAS_MAX_COUNTRY];
    char base_url[NTRIP_ATLAS_MAX_URL_LEN];
    uint16_t port;
    uint8_t ssl;
    ntrip_network_type_t network_type;
    ntrip_auth_method_t auth_method;
    uint8_t requires_registration;
    uint8_t typical_free_access;
    uint8_t quality_rating;  // 1-5 stars

    // Geographic coverage
    double coverage_lat_min;
    double coverage_lat_max;
    double coverage_lon_min;
    double coverage_lon_max;
} ntrip_service_config_t;

/**
 * Mountpoint information (discovered at runtime)
 */
typedef struct {
    char mountpoint[NTRIP_ATLAS_MAX_MOUNTPOINT];
    char identifier[64];     // Human-readable location name
    double latitude;
    double longitude;
    char format[NTRIP_ATLAS_MAX_FORMAT];
    char format_details[NTRIP_ATLAS_MAX_DETAILS];
    char nav_system[32];     // GPS+GLONASS+Galileo
    char receiver_type[64];  // Hardware generating data
    uint16_t bitrate;
    uint8_t nmea_required;
    ntrip_auth_method_t authentication;
    uint8_t fee_required;

    // Runtime calculated fields
    double distance_km;
    uint8_t suitability_score;
    const ntrip_service_config_t* service;
} ntrip_mountpoint_t;

/**
 * Best service selection result
 * NOTE: All essential mountpoint data is copied inline to avoid pointer lifetime issues.
 *       Pointers to config/mountpoint structs are only valid during discovery.
 */
typedef struct {
    char server[NTRIP_ATLAS_MAX_URL_LEN];
    uint16_t port;
    uint8_t ssl;
    char mountpoint[NTRIP_ATLAS_MAX_MOUNTPOINT];
    char username[NTRIP_ATLAS_MAX_USERNAME];
    char password[NTRIP_ATLAS_MAX_PASSWORD];
    double distance_km;
    uint8_t quality_score;

    // Essential mountpoint information copied inline (fixes pointer lifetime bug)
    double mountpoint_latitude;
    double mountpoint_longitude;
    char format[NTRIP_ATLAS_MAX_FORMAT];
    uint8_t nmea_required;

    const ntrip_service_config_t* service_info;  // Still available for reference
} ntrip_best_service_t;

/**
 * Selection criteria for filtering services
 */
typedef struct {
    // Technical requirements
    char required_formats[64];   // "RTCM 3.2" or "RTCM 3.2,RTCM 3.1"
    char required_systems[32];   // "GPS" or "GPS+GLONASS"
    uint16_t min_bitrate;        // Minimum data rate
    ntrip_auth_method_t max_auth; // Maximum auth complexity
    uint8_t free_only;           // Exclude paid services

    // Geographic constraints
    double max_distance_km;      // Maximum distance from user

    // Quality requirements
    uint8_t min_quality_rating;  // Minimum 1-5 stars
    ntrip_network_type_t preferred_network; // Prefer govt/commercial/community
} ntrip_selection_criteria_t;

/**
 * Service failure tracking for exponential backoff
 */
typedef struct {
    char service_id[64];         // Unique service identifier
    uint32_t failure_count;      // Number of consecutive failures
    uint32_t first_failure_time; // Timestamp of first failure (seconds since epoch)
    uint32_t next_retry_time;    // When service can be retried again
    uint32_t backoff_seconds;    // Current backoff period
} ntrip_service_failure_t;

/**
 * Failure tracking configuration
 */
typedef struct {
    // Exponential backoff intervals (seconds): 1h, 4h, 12h, 1d, 3d, 1w, 2w, 1m
    uint32_t backoff_intervals[8];
    uint8_t max_backoff_level;   // Max index in backoff_intervals
    uint8_t failure_tracking_enabled; // Enable/disable failure tracking
} ntrip_failure_config_t;

/**
 * Compact service structure for runtime discovery
 * Enhanced with polygon coverage support for precise geographic boundaries
 * Optimized for ESP32 flash storage with memory-mapped access
 */
typedef struct __attribute__((packed)) {
    // Connection info (35 bytes)
    char hostname[32];          // Most NTRIP hostnames are short
    uint16_t port;              // TCP port
    uint8_t flags;              // Packed boolean flags (see NTRIP_FLAG_* below)

    // Geographic coverage - bounding box (8 bytes - for fast O(1) filtering)
    int16_t lat_min_deg100;     // Coverage bounds: -90.00 to 90.00 × 100
    int16_t lat_max_deg100;
    int16_t lon_min_deg100;     // Coverage bounds: -180.00 to 180.00 × 100
    int16_t lon_max_deg100;

    // Hierarchical coverage reference (2 bytes - for bitmap lookup)
    uint8_t coverage_levels;    // Bitmask: which levels this service covers (bits 0-4 = levels 0-4)
    uint8_t reserved;           // Padding for alignment

    // Service metadata (3 bytes)
    uint8_t provider_index;     // Index into shared provider string table
    uint8_t network_type;       // government/commercial/community
    uint8_t quality_rating;     // 1-5 star rating
} ntrip_service_compact_t;      // Total: 48 bytes (was 46, briefly 52 with polygons)

// Compact service flag definitions
#define NTRIP_FLAG_SSL              (1 << 0)  // HTTPS/SSL connection
#define NTRIP_FLAG_AUTH_BASIC       (1 << 1)  // Basic authentication
#define NTRIP_FLAG_AUTH_DIGEST      (1 << 2)  // Digest authentication
#define NTRIP_FLAG_REQUIRES_REG     (1 << 3)  // Requires registration
#define NTRIP_FLAG_FREE_ACCESS      (1 << 4)  // Free/community access
#define NTRIP_FLAG_GLOBAL_SERVICE   (1 << 5)  // Global coverage service - skip spatial indexing
#define NTRIP_FLAG_PAID_SERVICE     (1 << 6)  // Commercial paid service - check credentials

/**
 * Geographic blacklisting structures for avoiding repeated queries
 * to services outside their coverage areas
 */

// Maximum services that can have geographic blacklists
#define NTRIP_MAX_SERVICES 32

// Geographic blacklist entry for a specific region
typedef struct {
    int16_t grid_lat;           // Grid latitude (1-degree resolution)
    int16_t grid_lon;           // Grid longitude (1-degree resolution)
    time_t blacklisted_time;    // When this region was blacklisted
    char reason[64];            // Error reason (e.g., "No coverage in this area")
} ntrip_geo_blacklist_entry_t;

// Geographic blacklist statistics
typedef struct {
    uint16_t services_with_blacklists;
    uint16_t total_blacklisted_regions;
    uint8_t max_entries_per_service;
    double grid_size_degrees;
} ntrip_geo_blacklist_stats_t;

/**
 * Geographic filtering statistics for coverage analysis
 */
typedef struct {
    uint16_t total_services;                 // Total number of services analyzed
    uint16_t services_with_coverage;         // Services covering user location
    double coverage_percentage;              // Percentage of services with coverage
    double nearest_service_distance_km;      // Distance to nearest service center
    double farthest_service_distance_km;     // Distance to farthest service center
} ntrip_geo_filtering_stats_t;

/**
 * Compact failure storage for memory-constrained systems (6 bytes vs 80 bytes)
 * Provides 93% memory reduction for ESP32 deployments
 */
typedef struct __attribute__((packed)) {
    uint8_t service_index;       // 1 byte - index into service table (0-255)
    uint8_t backoff_level : 4;   // 4 bits - exponential backoff level (0-15)
    uint8_t failure_count : 4;   // 4 bits - failure count (0-15, saturates at 15)
    uint32_t retry_time_hours;   // 4 bytes - hours since epoch when retry allowed
} ntrip_compact_failure_t;       // 6 bytes total

/**
 * Service index mapping for compact failure storage
 */
typedef struct {
    char service_id[32];         // Service identifier (shortened)
    uint8_t service_index;       // Compact index (0-255)
} ntrip_service_index_entry_t;

/**
 * Streaming callback context
 */
typedef struct {
    void* user_data;           // User-provided context
    void* internal_state;      // Internal parser state
} ntrip_stream_context_t;

/**
 * HTTP streaming callback function
 *
 * Called repeatedly with chunks of HTTP response data as they arrive.
 * Enables processing of large sourcetables without buffering entire response.
 *
 * @param chunk      Pointer to data chunk (not null-terminated)
 * @param len        Length of data chunk in bytes
 * @param context    User context pointer passed to http_stream
 * @return          0 to continue receiving, non-zero to stop streaming
 */
typedef int (*ntrip_stream_callback_t)(
    const char* chunk,
    size_t len,
    void* context
);

/**
 * Platform abstraction interface (v2.0 - streaming support)
 */
typedef struct {
    uint16_t interface_version;  // Set to 2 for streaming support

    // Streaming HTTP/HTTPS for sourcetable discovery
    // Replaces old http_request buffer-based approach
    int (*http_stream)(
        const char* host,
        uint16_t port,
        uint8_t ssl,
        const char* path,
        ntrip_stream_callback_t on_data,
        void* user_context,
        uint32_t timeout_ms
    );

    // NMEA sentence sending for VRS networks
    // Called after successful mountpoint connection
    int (*send_nmea)(
        void* connection,
        const char* nmea_sentence
    );

    // Secure credential storage
    int (*store_credential)(const char* key, const char* value);
    int (*load_credential)(const char* key, char* value, size_t max_len);

    // Failure tracking persistence (optional, can be NULL)
    int (*store_failure_data)(const char* service_id, const ntrip_service_failure_t* failure);
    int (*load_failure_data)(const char* service_id, ntrip_service_failure_t* failure);
    int (*clear_failure_data)(const char* service_id);

    // Logging interface
    void (*log_message)(int level, const char* message);

    // Time functions
    uint32_t (*get_time_ms)(void);
    uint32_t (*get_time_seconds)(void);  // For failure tracking timestamps
} ntrip_platform_t;

/**
 * Core API Functions
 */

/**
 * Initialize NTRIP Atlas with platform-specific callbacks
 */
ntrip_atlas_error_t ntrip_atlas_init(const ntrip_platform_t* platform);

/**
 * Initialize NTRIP Atlas with specific feature set
 */
ntrip_atlas_error_t ntrip_atlas_init_features(uint8_t feature_flags);

/**
 * Find best NTRIP service for given coordinates
 */
ntrip_atlas_error_t ntrip_atlas_find_best(ntrip_best_service_t* result,
                                         double latitude, double longitude);

/**
 * Find best service with custom selection criteria
 */
ntrip_atlas_error_t ntrip_atlas_find_best_filtered(ntrip_best_service_t* result,
                                                  double latitude, double longitude,
                                                  const ntrip_selection_criteria_t* criteria);

/**
 * Find best service with fallback option
 */
ntrip_atlas_error_t ntrip_atlas_find_best_with_fallback(ntrip_best_service_t* primary,
                                                       ntrip_best_service_t* fallback,
                                                       double latitude, double longitude);

/**
 * Store credentials for a specific service provider
 */
ntrip_atlas_error_t ntrip_atlas_set_credentials(const char* service_id,
                                               const char* username,
                                               const char* password);

/**
 * Credential store for managing multiple service credentials
 */
typedef struct {
    uint8_t count;
    struct {
        char service_id[32];
        char username[NTRIP_ATLAS_MAX_USERNAME];
        char password[NTRIP_ATLAS_MAX_PASSWORD];
    } credentials[16];  // Support up to 16 different services
} ntrip_credential_store_t;

/**
 * Initialize credential store
 */
void ntrip_atlas_init_credential_store(ntrip_credential_store_t* store);

/**
 * Add credentials to store
 */
ntrip_atlas_error_t ntrip_atlas_add_credential(ntrip_credential_store_t* store,
                                              const char* service_id,
                                              const char* username,
                                              const char* password);

/**
 * Check if credentials exist for a service
 */
bool ntrip_atlas_has_credentials(const ntrip_credential_store_t* store,
                                const char* service_id);

/**
 * Get credentials for a service
 */
ntrip_atlas_error_t ntrip_atlas_get_credentials(const ntrip_credential_store_t* store,
                                               const char* service_id,
                                               char* username, size_t username_len,
                                               char* password, size_t password_len);

/**
 * Check if a service is accessible with available credentials
 * Uses provider name as service identifier
 */
bool ntrip_atlas_is_service_accessible(const ntrip_service_config_t* service,
                                      const ntrip_credential_store_t* store);

/**
 * Payment Priority Configuration Functions
 */

/**
 * Set global payment priority configuration
 * @param priority Payment priority mode (free-first vs paid-first)
 * @return Success/error status
 */
ntrip_atlas_error_t ntrip_atlas_set_payment_priority(ntrip_payment_priority_t priority);

/**
 * Get current payment priority configuration
 * @return Current payment priority mode
 */
ntrip_payment_priority_t ntrip_atlas_get_payment_priority(void);

/**
 * Check if a service requires credentials and if we have them
 * @param service Service to check
 * @param store Credential store to check against
 * @return true if service is accessible (free or has credentials)
 */
bool ntrip_atlas_is_service_usable(const ntrip_service_compact_t* service,
                                  const ntrip_credential_store_t* store);

/**
 * Filter services by payment priority and credential availability
 * @param services Input service array
 * @param service_count Number of input services
 * @param store Credential store for paid service validation
 * @param priority Payment priority mode
 * @param filtered_services Output filtered service array
 * @param max_filtered Maximum size of output array
 * @return Number of usable services in priority order
 */
size_t ntrip_atlas_filter_services_by_payment_priority(
    const ntrip_service_compact_t* services,
    size_t service_count,
    const ntrip_credential_store_t* store,
    ntrip_payment_priority_t priority,
    ntrip_service_compact_t* filtered_services,
    size_t max_filtered
);

/**
 * Compact Service Runtime Optimization Functions
 */

/**
 * Convert full service config to compact format (81.5% memory reduction)
 */
ntrip_atlas_error_t ntrip_atlas_compress_service(
    const ntrip_service_config_t* full,
    ntrip_service_compact_t* compact
);

/**
 * Convert compact service back to full format (for compatibility)
 */
ntrip_atlas_error_t ntrip_atlas_expand_service(
    const ntrip_service_compact_t* compact,
    ntrip_service_config_t* full
);

/**
 * Get memory usage statistics for compact vs full services
 */
ntrip_atlas_error_t ntrip_atlas_get_compact_memory_stats(
    size_t service_count,
    size_t* full_bytes,
    size_t* compact_bytes,
    size_t* savings_bytes
);

/**
 * Geographic Blacklisting Functions
 * Avoid repeated queries to services outside their coverage areas
 */

/**
 * Initialize geographic blacklist system
 */
ntrip_atlas_error_t ntrip_atlas_init_geographic_blacklist(void);

/**
 * Add a geographic region to service blacklist (when service reports no coverage)
 */
ntrip_atlas_error_t ntrip_atlas_blacklist_service_region(
    const char* provider,
    double latitude,
    double longitude,
    const char* error_reason
);

/**
 * Check if a service is blacklisted for a geographic region
 */
bool ntrip_atlas_is_service_geographically_blacklisted(
    const char* provider,
    double latitude,
    double longitude
);

/**
 * Remove geographic blacklist entry for a service
 */
ntrip_atlas_error_t ntrip_atlas_remove_geographic_blacklist(
    const char* provider,
    double latitude,
    double longitude
);

/**
 * Clear all geographic blacklist entries for a service
 */
ntrip_atlas_error_t ntrip_atlas_clear_service_geographic_blacklist(const char* provider);

/**
 * Clear all geographic blacklist entries (for testing/reset)
 */
ntrip_atlas_error_t ntrip_atlas_clear_all_geographic_blacklists(void);

/**
 * Get blacklist statistics for monitoring
 */
ntrip_atlas_error_t ntrip_atlas_get_geographic_blacklist_stats(
    ntrip_geo_blacklist_stats_t* stats
);

/**
 * Filter service list to remove geographically blacklisted services
 */
size_t ntrip_atlas_filter_geographically_blacklisted_services(
    const ntrip_service_compact_t* services,
    size_t service_count,
    double latitude,
    double longitude,
    ntrip_service_compact_t* filtered_services,
    size_t max_filtered
);

/**
 * Geographic Filtering Functions
 * Provides distance-based filtering using service coverage bounding boxes
 */

/**
 * Check if user location is within service coverage bounds
 */
bool ntrip_atlas_is_location_within_service_coverage(
    const ntrip_service_compact_t* service,
    double user_latitude,
    double user_longitude
);

/**
 * Calculate distance from user to service coverage center
 */
double ntrip_atlas_calculate_distance_to_service_center(
    const ntrip_service_compact_t* service,
    double user_latitude,
    double user_longitude
);

/**
 * Calculate distance to nearest edge of service coverage area
 */
double ntrip_atlas_calculate_distance_to_coverage_edge(
    const ntrip_service_compact_t* service,
    double user_latitude,
    double user_longitude
);

/**
 * Filter services by geographic coverage
 */
size_t ntrip_atlas_filter_services_by_coverage(
    const ntrip_service_compact_t* services,
    size_t service_count,
    double user_latitude,
    double user_longitude,
    double max_distance_km,
    ntrip_service_compact_t* filtered_services,
    size_t max_filtered
);

/**
 * Filter and sort services by geographic proximity
 */
size_t ntrip_atlas_filter_and_sort_services_by_location(
    ntrip_service_compact_t* services,
    size_t service_count,
    double user_latitude,
    double user_longitude,
    double max_distance_km
);

/**
 * Get geographic coverage statistics
 */
ntrip_atlas_error_t ntrip_atlas_get_geographic_filtering_stats(
    const ntrip_service_compact_t* services,
    size_t service_count,
    double user_latitude,
    double user_longitude,
    ntrip_geo_filtering_stats_t* stats
);

/**
 * Test connectivity to a specific service
 */
ntrip_atlas_error_t ntrip_atlas_test_service(const ntrip_best_service_t* service);

/**
 * Get all available services in a region
 */
ntrip_atlas_error_t ntrip_atlas_list_services_in_region(double lat_min, double lat_max,
                                                       double lon_min, double lon_max,
                                                       ntrip_service_config_t* services,
                                                       size_t* count, size_t max_count);

/**
 * Get detailed information about a specific service
 */
ntrip_atlas_error_t ntrip_atlas_get_service_info(const char* service_id,
                                                ntrip_service_config_t* info);

/**
 * Failure Tracking Functions
 */

/**
 * Initialize failure tracking with custom backoff configuration
 */
ntrip_atlas_error_t ntrip_atlas_init_failure_tracking(const ntrip_failure_config_t* config);

/**
 * Record a service failure and update backoff state
 */
ntrip_atlas_error_t ntrip_atlas_record_service_failure(const char* service_id);

/**
 * Record a successful service connection (resets failure count)
 */
ntrip_atlas_error_t ntrip_atlas_record_service_success(const char* service_id);

/**
 * Check if a service is currently blocked due to failures
 */
bool ntrip_atlas_is_service_blocked(const char* service_id);

/**
 * Get time until service can be retried (0 if available now)
 */
uint32_t ntrip_atlas_get_service_retry_time(const char* service_id);

/**
 * Compact Failure Tracking API (Memory-optimized for ESP32)
 * Reduces memory usage from 80 bytes to 6 bytes per service (93% reduction)
 */

/**
 * Initialize compact failure tracking with service index mapping
 * @param service_mapping Array of service ID to index mappings
 * @param mapping_count Number of entries in service_mapping array
 */
ntrip_atlas_error_t ntrip_atlas_init_compact_failure_tracking(
    const ntrip_service_index_entry_t* service_mapping,
    size_t mapping_count
);

/**
 * Convert service ID string to compact index
 * @param service_id Service identifier string
 * @return Service index (0-255) or 255 if not found
 */
uint8_t ntrip_atlas_get_service_index(const char* service_id);

/**
 * Record a service failure using compact storage
 * @param service_index Service index (from ntrip_atlas_get_service_index)
 */
ntrip_atlas_error_t ntrip_atlas_record_compact_failure(uint8_t service_index);

/**
 * Record service success using compact storage (resets failure count)
 * @param service_index Service index
 */
ntrip_atlas_error_t ntrip_atlas_record_compact_success(uint8_t service_index);

/**
 * Check if service is blocked using compact storage
 * @param service_index Service index
 * @return true if service is currently in backoff
 */
bool ntrip_atlas_is_compact_service_blocked(uint8_t service_index);

/**
 * Get retry time for compact service (hours until retry allowed)
 * @param service_index Service index
 * @return Hours until retry allowed (0 if available now)
 */
uint32_t ntrip_atlas_get_compact_retry_time_hours(uint8_t service_index);

/**
 * Convert compact failure to full failure structure (for debugging/analysis)
 * @param compact Compact failure structure
 * @param full Output full failure structure
 */
ntrip_atlas_error_t ntrip_atlas_expand_compact_failure(
    const ntrip_compact_failure_t* compact,
    ntrip_service_failure_t* full
);

/**
 * Get backoff seconds from backoff level (utility function)
 * @param backoff_level Backoff level (0-15)
 * @return Backoff duration in seconds
 */
uint32_t ntrip_atlas_get_backoff_seconds_from_level(uint8_t backoff_level);

/**
 * Filter service list to exclude blocked services during discovery
 * This ensures users get immediate NTRIP data from next-best service
 * instead of waiting for blocked services to become available
 *
 * @param services Input array of service candidates
 * @param service_count Number of services in input array
 * @param filtered_services Output array of non-blocked services
 * @param max_filtered Maximum size of filtered_services array
 * @return Number of non-blocked services in filtered array
 */
size_t ntrip_atlas_filter_blocked_services(
    const ntrip_service_config_t* services,
    size_t service_count,
    ntrip_service_config_t* filtered_services,
    size_t max_filtered
);

/**
 * Check if a service should be skipped during discovery due to failure backoff
 * Used internally by discovery algorithms to prioritize available services
 *
 * @param service_id Service identifier to check
 * @return true if service should be skipped (in backoff), false if available
 */
bool ntrip_atlas_should_skip_service(const char* service_id);

/**
 * Clear all failure history (for testing or reset purposes)
 */
ntrip_atlas_error_t ntrip_atlas_clear_failure_history(void);

/**
 * Database Versioning and Forward Compatibility
 * Ensures smooth upgrades and clear compatibility handling
 */

// Database file magic numbers for format verification
#define NTRIP_ATLAS_DB_MAGIC_V1     0x4E545250  // "NTRP" in ASCII
#define NTRIP_ATLAS_SCHEMA_MAGIC    0x53434845  // "SCHE" in ASCII

// Current library schema version
#define NTRIP_ATLAS_SCHEMA_MAJOR    1
#define NTRIP_ATLAS_SCHEMA_MINOR    1

// Database feature flags (bitwise)
#define NTRIP_DB_FEATURE_COMPACT_FAILURES  0x01  // Compact failure tracking
#define NTRIP_DB_FEATURE_GEOGRAPHIC_INDEX   0x02  // Geographic indexing
#define NTRIP_DB_FEATURE_TIERED_LOADING     0x04  // Tiered data loading
#define NTRIP_DB_FEATURE_EXTENDED_AUTH      0x08  // Extended auth methods
#define NTRIP_DB_FEATURE_RESERVED_1         0x10  // Future use
#define NTRIP_DB_FEATURE_RESERVED_2         0x20  // Future use
#define NTRIP_DB_FEATURE_RESERVED_3         0x40  // Future use
#define NTRIP_DB_FEATURE_EXPERIMENTAL       0x80  // Experimental features

// Feature initialization modes
#define NTRIP_ATLAS_FEATURE_CORE    (NTRIP_DB_FEATURE_COMPACT_FAILURES)
#define NTRIP_ATLAS_FEATURE_ALL     (NTRIP_DB_FEATURE_COMPACT_FAILURES | \
                                     NTRIP_DB_FEATURE_GEOGRAPHIC_INDEX | \
                                     NTRIP_DB_FEATURE_TIERED_LOADING)

/**
 * Database header for version verification and compatibility
 */
typedef struct __attribute__((packed)) {
    uint32_t magic_number;      // 4 bytes - NTRP magic
    uint16_t schema_major;      // 2 bytes - Major schema version
    uint16_t schema_minor;      // 2 bytes - Minor schema version
    uint32_t database_version;  // 4 bytes - YYYYMMDD date
    uint8_t sequence_number;    // 1 byte - Daily sequence (01-99)
    uint8_t feature_flags;      // 1 byte - Feature compatibility flags
    uint16_t service_count;     // 2 bytes - Total services in database
} ntrip_db_header_t;           // 16 bytes total

/**
 * Version compatibility levels
 */
typedef enum {
    NTRIP_COMPAT_COMPATIBLE,      // Fully compatible
    NTRIP_COMPAT_BACKWARD_ONLY,   // Can read, may miss new features
    NTRIP_COMPAT_UPGRADE_NEEDED,  // Library needs upgrade
    NTRIP_COMPAT_INCOMPATIBLE     // Complete incompatibility
} ntrip_compatibility_t;

/**
 * Library and database version information
 */
typedef struct {
    uint16_t library_schema_major;
    uint16_t library_schema_minor;
    uint32_t database_version;
    uint8_t supported_features;
    bool compact_failure_support;
    bool geographic_index_support;
    bool tiered_loading_support;
} ntrip_version_info_t;

/**
 * Check database compatibility with current library
 * @param db_header Database header to check
 * @param compatibility Output compatibility level
 * @return Success/error status
 */
ntrip_atlas_error_t ntrip_atlas_check_database_compatibility(
    const ntrip_db_header_t* db_header,
    ntrip_compatibility_t* compatibility
);

/**
 * Get current library version information
 * @param version_info Output version information
 * @return Success/error status
 */
ntrip_atlas_error_t ntrip_atlas_get_version_info(
    ntrip_version_info_t* version_info
);

/**
 * Create database header with specified parameters
 * @param header Output header structure
 * @param database_version Database version (YYYYMMDD format)
 * @param sequence_number Daily sequence number (01-99)
 * @param service_count Total number of services
 * @return Success/error status
 */
ntrip_atlas_error_t ntrip_atlas_create_database_header(
    ntrip_db_header_t* header,
    uint32_t database_version,
    uint8_t sequence_number,
    uint16_t service_count
);

/**
 * Validate database header format and values
 * @param header Database header to validate
 * @return Success/error status
 */
ntrip_atlas_error_t ntrip_atlas_validate_database_header(
    const ntrip_db_header_t* header
);

/**
 * Check if library supports specific feature
 * @param feature_flag Feature flag to check
 * @return true if feature is supported
 */
bool ntrip_atlas_supports_feature(uint8_t feature_flag);

/**
 * Get human-readable compatibility message
 * @param compatibility Compatibility level
 * @return Descriptive message string
 */
const char* ntrip_atlas_get_compatibility_message(ntrip_compatibility_t compatibility);

/**
 * Print version information to stdout (for debugging)
 */
void ntrip_atlas_print_version_info(void);

/**
 * Initialize with version checking and graceful degradation
 * @param db_header Database header (NULL to skip version check)
 * @return Success/error status with version-specific errors
 */
ntrip_atlas_error_t ntrip_atlas_init_with_version_check(
    const ntrip_db_header_t* db_header
);

/**
 * Tiered Data Loading System
 * Optimizes memory usage by loading only essential data for discovery,
 * deferring detailed information until actually needed.
 */

/**
 * Tier 1: Discovery Index (Essential data for service selection)
 * Memory usage: 16 bytes per service (95% reduction from full data)
 */
typedef struct __attribute__((packed)) {
    uint8_t service_index;       // 1 byte - Index into full service table
    int16_t lat_center_deg100;   // 2 bytes - Center latitude * 100
    int16_t lon_center_deg100;   // 2 bytes - Center longitude * 100
    uint8_t radius_km;           // 1 byte - Coverage radius (0-255km)
    uint8_t quality_rating;      // 1 byte - Quality rating (1-5 stars)
    uint8_t network_type;        // 1 byte - Government/Commercial/Community
    uint8_t auth_method;         // 1 byte - None/Basic/Digest
    uint8_t requires_registration; // 1 byte - Boolean
    uint8_t ssl_available;       // 1 byte - Boolean
    char provider_short[4];      // 4 bytes - Short provider name
} ntrip_service_index_t;         // 16 bytes total

/**
 * Tier 2: Service Endpoints (Loaded when service selected for connection)
 */
typedef struct __attribute__((packed)) {
    char hostname[48];           // 48 bytes - Full hostname
    uint16_t port;               // 2 bytes - Port number
    uint16_t ssl_port;           // 2 bytes - SSL port (0 if none)
    char base_path[32];          // 32 bytes - URL path
    char user_agent[32];         // 32 bytes - Required user agent
    uint8_t connection_flags;    // 1 byte - Various connection options
} ntrip_service_endpoints_t;     // 117 bytes

/**
 * Tier 3: Administrative Metadata (Loaded only when requested for UI/details)
 */
typedef struct {
    char provider_full[64];      // Full provider name
    char country[32];            // Country name
    char description[128];       // Service description
    char website[64];            // Provider website
    char contact_email[64];      // Support contact
    char registration_url[128];  // Where to register
    uint32_t last_updated;       // When service info updated
    char coverage_notes[256];    // Coverage area notes
} ntrip_service_metadata_t;      // ~800 bytes

/**
 * Data loading modes for backward compatibility
 */
typedef enum {
    NTRIP_LOADING_FULL,          // Load all data (current behavior)
    NTRIP_LOADING_TIERED         // Use tiered loading (memory optimized)
} ntrip_loading_mode_t;

/**
 * Platform-specific data loading interface for tiered system
 */
typedef struct {
    // Load discovery index (Tier 1) - called once at startup
    ntrip_atlas_error_t (*load_discovery_index)(
        ntrip_service_index_t** index,
        size_t* count,
        void* platform_data
    );

    // Load service endpoints (Tier 2) - called per selected service
    ntrip_atlas_error_t (*load_service_endpoints)(
        uint8_t service_index,
        ntrip_service_endpoints_t* endpoints,
        void* platform_data
    );

    // Load service metadata (Tier 3) - called for UI/details only
    ntrip_atlas_error_t (*load_service_metadata)(
        uint8_t service_index,
        ntrip_service_metadata_t* metadata,
        void* platform_data
    );

    void* platform_data;        // Platform-specific context
} ntrip_tiered_platform_t;

/**
 * Initialize with tiered data loading for memory optimization
 * @param mode Loading mode (full vs tiered)
 * @param tiered_platform Tiered loading callbacks (NULL for full mode)
 * @return Success/error status
 */
ntrip_atlas_error_t ntrip_atlas_init_with_loading_mode(
    ntrip_loading_mode_t mode,
    const ntrip_tiered_platform_t* tiered_platform
);

/**
 * Fast discovery using only Tier 1 data (discovery index)
 * Uses 95% less memory than traditional approach
 * @param result Output best service result
 * @param user_lat User latitude
 * @param user_lon User longitude
 * @return Success/error status
 */
ntrip_atlas_error_t ntrip_atlas_find_best_tiered(
    ntrip_best_service_t* result,
    double user_lat,
    double user_lon
);

/**
 * Load service endpoints on demand (Tier 2)
 * @param service_index Service index from discovery
 * @param endpoints Output endpoints structure
 * @return Success/error status
 */
ntrip_atlas_error_t ntrip_atlas_load_service_endpoints(
    uint8_t service_index,
    ntrip_service_endpoints_t* endpoints
);

/**
 * Load service metadata on demand (Tier 3)
 * @param service_index Service index from discovery
 * @param metadata Output metadata structure
 * @return Success/error status
 */
ntrip_atlas_error_t ntrip_atlas_load_service_metadata(
    uint8_t service_index,
    ntrip_service_metadata_t* metadata
);

/**
 * Get memory usage statistics for tiered loading
 * @param tier1_bytes Output Tier 1 memory usage
 * @param tier2_bytes Output Tier 2 memory usage (cached)
 * @param tier3_bytes Output Tier 3 memory usage (cached)
 * @return Success/error status
 */
ntrip_atlas_error_t ntrip_atlas_get_tiered_memory_stats(
    size_t* tier1_bytes,
    size_t* tier2_bytes,
    size_t* tier3_bytes
);

/**
 * Trim caches under memory pressure (ESP32 optimization)
 * Frees Tier 2/3 cached data while preserving Tier 1 discovery index
 */
void ntrip_atlas_trim_caches(void);

/**
 * Get default exponential backoff configuration
 * Returns: 1h, 4h, 12h, 1d, 3d, 1w, 2w, 1month progression
 */
ntrip_failure_config_t ntrip_atlas_get_default_failure_config(void);

/**
 * Utility Functions
 */

/**
 * Calculate distance between two coordinates (Haversine formula)
 */
double ntrip_atlas_calculate_distance(double lat1, double lon1, double lat2, double lon2);

/**
 * Format NMEA GGA sentence for VRS position updates
 *
 * Creates a properly formatted NMEA GGA sentence with checksum.
 * Required by many VRS networks to select appropriate virtual reference station.
 *
 * @param buffer        Output buffer for GGA sentence
 * @param max_len       Size of output buffer (recommend 128 bytes minimum)
 * @param latitude      Latitude in decimal degrees (-90 to +90)
 * @param longitude     Longitude in decimal degrees (-180 to +180)
 * @param altitude_m    Altitude in meters above WGS84 ellipsoid
 * @param fix_quality   Fix quality: 0=invalid, 1=GPS, 2=DGPS, 4=RTK fixed, 5=RTK float
 * @param satellites    Number of satellites in use (0-99)
 * @return             Length of formatted sentence, or negative on error
 */
int ntrip_atlas_format_gga(
    char* buffer,
    size_t max_len,
    double latitude,
    double longitude,
    double altitude_m,
    uint8_t fix_quality,
    uint8_t satellites
);

/**
 * Get library version string
 */
const char* ntrip_atlas_get_version(void);

/**
 * Spatial Indexing System (O(1) Service Discovery)
 * Based on Brad Fitzpatrick's latlong library adaptive grid optimization
 */

/**
 * 32-bit tile key for hierarchical spatial indexing
 * Bit layout: [level:3][reserved:3][lat_tile:13][lon_tile:13]
 */
typedef uint32_t ntrip_tile_key_t;

/**
 * Spatial index statistics
 */
typedef struct {
    uint16_t total_tiles;
    uint16_t populated_tiles;
    uint16_t total_service_assignments;
    uint16_t max_services_per_tile;
    double average_services_per_tile;
    size_t memory_used_bytes;
} ntrip_spatial_index_stats_t;

/**
 * Encode tile coordinates into 32-bit key
 */
ntrip_tile_key_t ntrip_atlas_encode_tile_key(uint8_t level, uint16_t lat_tile, uint16_t lon_tile);

/**
 * Decode tile key into components
 */
void ntrip_atlas_decode_tile_key(ntrip_tile_key_t key, uint8_t* level, uint16_t* lat_tile, uint16_t* lon_tile);

/**
 * Convert lat/lon coordinates to tile coordinates for given zoom level
 */
ntrip_atlas_error_t ntrip_atlas_lat_lon_to_tile(
    double lat,
    double lon,
    uint8_t level,
    uint16_t* tile_lat,
    uint16_t* tile_lon
);

/**
 * Convert tile coordinates back to lat/lon bounds
 */
ntrip_atlas_error_t ntrip_atlas_tile_to_lat_lon_bounds(
    uint8_t level,
    uint16_t tile_lat,
    uint16_t tile_lon,
    double* lat_min,
    double* lat_max,
    double* lon_min,
    double* lon_max
);

/**
 * Initialize spatial index system
 */
ntrip_atlas_error_t ntrip_atlas_init_spatial_index(void);

/**
 * Add service to spatial tile (build-time operation)
 */
ntrip_atlas_error_t ntrip_atlas_add_service_to_tile(
    ntrip_tile_key_t tile_key,
    uint8_t service_index
);

/**
 * Find services covering user location using spatial index
 * O(1) lookup replacing O(n) linear scan
 */
size_t ntrip_atlas_find_services_by_location_fast(
    double user_lat,
    double user_lon,
    uint8_t* service_indices,
    size_t max_services
);

/**
 * Get spatial index statistics
 */
ntrip_atlas_error_t ntrip_atlas_get_spatial_index_stats(
    ntrip_spatial_index_stats_t* stats
);

/**
 * Find services using spatial indexing with geographic bounds validation
 *
 * Combines O(1) spatial indexing with precise geographic coverage checking
 * to solve the "German problem" - ensures services only appear in areas
 * where they actually provide coverage.
 *
 * @param user_lat User latitude
 * @param user_lon User longitude
 * @param services Service database array
 * @param service_count Total number of services
 * @param found_services Output array for verified service indices
 * @param max_services Maximum services to return
 * @return Number of services found with verified coverage
 */
size_t ntrip_atlas_find_services_spatial_geographic(
    double user_lat,
    double user_lon,
    const ntrip_service_compact_t* services,
    size_t service_count,
    uint8_t* found_services,
    size_t max_services
);

/**
 * Find best service using spatial-geographic lookup
 *
 * Higher-level convenience function that finds the best service
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
);

/**
 * Get statistics about spatial-geographic lookup performance
 *
 * Analyzes effectiveness of integrated approach by comparing
 * spatial candidates vs verified services.
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
);

/**
 * Print spatial index debug information
 */
void ntrip_atlas_print_spatial_index_debug(void);

/**
 * Get error description string
 */
const char* ntrip_atlas_error_string(ntrip_atlas_error_t error);

/**
 * Platform Implementations (declared in separate headers)
 */

// ESP32/Arduino platform
extern const ntrip_platform_t ntrip_platform_esp32;

// Linux platform
extern const ntrip_platform_t ntrip_platform_linux;

// Windows platform
extern const ntrip_platform_t ntrip_platform_windows;

#ifdef __cplusplus
}
#endif

#endif // NTRIP_ATLAS_H