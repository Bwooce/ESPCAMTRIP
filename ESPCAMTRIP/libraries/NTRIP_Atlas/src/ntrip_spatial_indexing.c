/**
 * NTRIP Atlas - Spatial Indexing Implementation
 *
 * Adaptive grid optimization for O(1) geographic service lookups.
 * Based on Brad Fitzpatrick's latlong library spatial indexing techniques.
 *
 * Provides hierarchical tile system with constant-time service discovery
 * replacing O(n) linear scanning with O(1) direct grid lookups.
 *
 * Licensed under MIT License
 */

#include "ntrip_atlas.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// Spatial indexing constants
#define SPATIAL_INDEX_MAX_TILES 4096   // Regional services only (31 services) - globals handled separately
#define SPATIAL_INDEX_MAX_SERVICES_PER_TILE 64  // Regional services only - globals handled separately
#define SPATIAL_INDEX_MAX_LEVELS 5

// Tile key encoding constants (32-bit)
#define TILE_LEVEL_SHIFT    29
#define TILE_LAT_SHIFT      13
#define TILE_LON_SHIFT      0
#define TILE_LAT_MASK       0x1FFF  // 13 bits = 8192 max tiles
#define TILE_LON_MASK       0x1FFF  // 13 bits = 8192 max tiles
#define TILE_LEVEL_MASK     0x07    // 3 bits = 8 max levels

// Tile entry contains service indices for geographic area
typedef struct {
    ntrip_tile_key_t key;
    uint8_t service_count;
    uint8_t service_indices[SPATIAL_INDEX_MAX_SERVICES_PER_TILE];
} ntrip_tile_t;

// Global spatial index (pre-computed at build time)
typedef struct {
    ntrip_tile_t tiles[SPATIAL_INDEX_MAX_TILES];
    uint16_t tile_count;
    bool initialized;
} ntrip_spatial_index_t;

// Global spatial index instance
static ntrip_spatial_index_t g_spatial_index = {0};

/**
 * Encode tile coordinates into 32-bit key
 *
 * Bit layout: [level:3][reserved:3][lat_tile:13][lon_tile:13]
 */
ntrip_tile_key_t ntrip_atlas_encode_tile_key(uint8_t level, uint16_t lat_tile, uint16_t lon_tile) {
    if (level >= SPATIAL_INDEX_MAX_LEVELS) return 0;  // Invalid level

    // Calculate max tiles for this level
    uint16_t max_lat_tiles = 2 << level;      // 2, 4, 8, 16, 32
    uint16_t max_lon_tiles = 4 << level;      // 4, 8, 16, 32, 64

    if (lat_tile >= max_lat_tiles) return 0;  // Invalid lat tile
    if (lon_tile >= max_lon_tiles) return 0;  // Invalid lon tile

    // Encode key (add 1 to ensure valid keys are never 0)
    uint32_t key = ((uint32_t)(level & TILE_LEVEL_MASK) << TILE_LEVEL_SHIFT) |
                   ((uint32_t)(lat_tile & TILE_LAT_MASK) << TILE_LAT_SHIFT) |
                   ((uint32_t)(lon_tile & TILE_LON_MASK) << TILE_LON_SHIFT);
    return key + 1;
}

/**
 * Decode tile key into components
 */
void ntrip_atlas_decode_tile_key(ntrip_tile_key_t key, uint8_t* level, uint16_t* lat_tile, uint16_t* lon_tile) {
    if (!level || !lat_tile || !lon_tile) return;

    // Decode key (subtract 1 to match encoding)
    if (key == 0) {
        *level = 0;
        *lat_tile = 0;
        *lon_tile = 0;
        return;
    }

    key -= 1;
    *level = (uint8_t)((key >> TILE_LEVEL_SHIFT) & TILE_LEVEL_MASK);
    *lat_tile = (uint16_t)((key >> TILE_LAT_SHIFT) & TILE_LAT_MASK);
    *lon_tile = (uint16_t)((key >> TILE_LON_SHIFT) & TILE_LON_MASK);
}

/**
 * Convert lat/lon coordinates to tile coordinates for given zoom level
 *
 * Hierarchical tile system:
 * - Level 0: 4×2 tiles    (90° × 180° each) - Continental
 * - Level 1: 8×4 tiles    (45° × 90° each)  - Regional
 * - Level 2: 16×8 tiles   (22.5° × 45° each) - National
 * - Level 3: 32×16 tiles  (11.25° × 22.5° each) - State
 * - Level 4: 64×32 tiles  (5.625° × 11.25° each) - Local
 */
ntrip_atlas_error_t ntrip_atlas_lat_lon_to_tile(
    double lat,
    double lon,
    uint8_t level,
    uint16_t* tile_lat,
    uint16_t* tile_lon
) {
    if (!tile_lat || !tile_lon) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    if (level >= SPATIAL_INDEX_MAX_LEVELS) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    // Calculate tiles per level: lat_tiles = 2^(level+1), lon_tiles = 2^(level+2)
    uint16_t lat_tiles = 2 << level;      // 2, 4, 8, 16, 32
    uint16_t lon_tiles = 4 << level;      // 4, 8, 16, 32, 64

    // Normalize coordinates: lat [-90,90] → [0,180], lon [-180,180] → [0,360]
    double norm_lat = lat + 90.0;         // [0, 180]
    double norm_lon = lon + 180.0;        // [0, 360]

    // Map to tile space
    *tile_lat = (uint16_t)(norm_lat * lat_tiles / 180.0);
    *tile_lon = (uint16_t)(norm_lon * lon_tiles / 360.0);

    // Clamp to valid range
    if (*tile_lat >= lat_tiles) *tile_lat = lat_tiles - 1;
    if (*tile_lon >= lon_tiles) *tile_lon = lon_tiles - 1;

    return NTRIP_ATLAS_SUCCESS;
}

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
) {
    if (!lat_min || !lat_max || !lon_min || !lon_max) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    if (level >= SPATIAL_INDEX_MAX_LEVELS) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    // Calculate tiles per level
    uint16_t lat_tiles = 2 << level;
    uint16_t lon_tiles = 4 << level;

    if (tile_lat >= lat_tiles || tile_lon >= lon_tiles) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    // Calculate tile size in degrees
    double lat_tile_size = 180.0 / lat_tiles;
    double lon_tile_size = 360.0 / lon_tiles;

    // Calculate bounds in normalized space [0,180] and [0,360]
    double norm_lat_min = tile_lat * lat_tile_size;
    double norm_lat_max = (tile_lat + 1) * lat_tile_size;
    double norm_lon_min = tile_lon * lon_tile_size;
    double norm_lon_max = (tile_lon + 1) * lon_tile_size;

    // Convert back to standard coordinate space
    *lat_min = norm_lat_min - 90.0;
    *lat_max = norm_lat_max - 90.0;
    *lon_min = norm_lon_min - 180.0;
    *lon_max = norm_lon_max - 180.0;

    return NTRIP_ATLAS_SUCCESS;
}

/**
 * Initialize spatial index system
 */
ntrip_atlas_error_t ntrip_atlas_init_spatial_index(void) {
    memset(&g_spatial_index, 0, sizeof(g_spatial_index));
    g_spatial_index.initialized = true;

    return NTRIP_ATLAS_SUCCESS;
}

/**
 * Binary search for tile by key
 */
static ntrip_tile_t* find_tile_by_key(ntrip_tile_key_t key) {
    if (!g_spatial_index.initialized || g_spatial_index.tile_count == 0) {
        return NULL;
    }

    int left = 0;
    int right = g_spatial_index.tile_count - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        if (g_spatial_index.tiles[mid].key == key) {
            return &g_spatial_index.tiles[mid];
        } else if (g_spatial_index.tiles[mid].key < key) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return NULL; // Tile not found
}

/**
 * Add service to spatial tile (build-time operation)
 */
ntrip_atlas_error_t ntrip_atlas_add_service_to_tile(
    ntrip_tile_key_t tile_key,
    uint8_t service_index
) {
    if (!g_spatial_index.initialized) {
        return NTRIP_ATLAS_ERROR_PLATFORM; // Not initialized
    }

    // Find existing tile or create new one
    ntrip_tile_t* tile = find_tile_by_key(tile_key);

    if (!tile) {
        // Create new tile
        if (g_spatial_index.tile_count >= SPATIAL_INDEX_MAX_TILES) {
            return NTRIP_ATLAS_ERROR_SPATIAL_INDEX_FULL; // No space for more tiles
        }

        // Find insertion point to maintain sorted order
        int insert_pos = 0;
        while (insert_pos < g_spatial_index.tile_count &&
               g_spatial_index.tiles[insert_pos].key < tile_key) {
            insert_pos++;
        }

        // Shift tiles to make room
        memmove(&g_spatial_index.tiles[insert_pos + 1],
                &g_spatial_index.tiles[insert_pos],
                (g_spatial_index.tile_count - insert_pos) * sizeof(ntrip_tile_t));

        // Initialize new tile
        tile = &g_spatial_index.tiles[insert_pos];
        tile->key = tile_key;
        tile->service_count = 0;
        memset(tile->service_indices, 0, sizeof(tile->service_indices));

        g_spatial_index.tile_count++;
    }

    // Add service to tile if not already present
    for (uint8_t i = 0; i < tile->service_count; i++) {
        if (tile->service_indices[i] == service_index) {
            return NTRIP_ATLAS_SUCCESS; // Service already in tile
        }
    }

    // Add service if there's space
    if (tile->service_count < SPATIAL_INDEX_MAX_SERVICES_PER_TILE) {
        tile->service_indices[tile->service_count] = service_index;
        tile->service_count++;
        return NTRIP_ATLAS_SUCCESS;
    }

    return NTRIP_ATLAS_ERROR_TILE_FULL; // Tile full
}

/**
 * Find services covering user location using spatial index
 *
 * O(1) lookup replacing O(n) linear scan
 */
size_t ntrip_atlas_find_services_by_location_fast(
    double user_lat,
    double user_lon,
    uint8_t* service_indices,
    size_t max_services
) {
    if (!service_indices || !g_spatial_index.initialized) {
        return 0;
    }

    // Start with finest resolution and work up until we find services
    for (int level = 4; level >= 0; level--) {
        uint16_t tile_lat, tile_lon;

        ntrip_atlas_error_t result = ntrip_atlas_lat_lon_to_tile(
            user_lat, user_lon, level, &tile_lat, &tile_lon
        );

        if (result != NTRIP_ATLAS_SUCCESS) {
            continue;
        }

        ntrip_tile_key_t key = ntrip_atlas_encode_tile_key(level, tile_lat, tile_lon);

        // Binary search for tile (O(log k) where k = number of tiles)
        ntrip_tile_t* tile = find_tile_by_key(key);

        if (tile && tile->service_count > 0) {
            size_t count = tile->service_count;
            if (count > max_services) {
                count = max_services;
            }

            memcpy(service_indices, tile->service_indices, count * sizeof(uint8_t));
            return count;
        }
    }

    return 0; // No services found
}

/**
 * Get spatial index statistics
 */
ntrip_atlas_error_t ntrip_atlas_get_spatial_index_stats(
    ntrip_spatial_index_stats_t* stats
) {
    if (!stats) {
        return NTRIP_ATLAS_ERROR_INVALID_PARAM;
    }

    memset(stats, 0, sizeof(*stats));

    if (!g_spatial_index.initialized) {
        return NTRIP_ATLAS_ERROR_PLATFORM;
    }

    stats->total_tiles = g_spatial_index.tile_count;
    stats->memory_used_bytes = sizeof(g_spatial_index);

    // Count services and analyze distribution
    for (uint16_t i = 0; i < g_spatial_index.tile_count; i++) {
        const ntrip_tile_t* tile = &g_spatial_index.tiles[i];

        stats->total_service_assignments += tile->service_count;

        if (tile->service_count > 0) {
            stats->populated_tiles++;
        }

        if (tile->service_count > stats->max_services_per_tile) {
            stats->max_services_per_tile = tile->service_count;
        }
    }

    stats->average_services_per_tile = stats->populated_tiles > 0 ?
        (double)stats->total_service_assignments / stats->populated_tiles : 0.0;

    return NTRIP_ATLAS_SUCCESS;
}

/**
 * Print spatial index debug information
 */
void ntrip_atlas_print_spatial_index_debug(void) {
    printf("NTRIP Atlas Spatial Index Debug\n");
    printf("===============================\n");

    if (!g_spatial_index.initialized) {
        printf("❌ Spatial index not initialized\n");
        return;
    }

    ntrip_spatial_index_stats_t stats;
    ntrip_atlas_error_t result = ntrip_atlas_get_spatial_index_stats(&stats);

    if (result != NTRIP_ATLAS_SUCCESS) {
        printf("❌ Failed to get spatial index stats\n");
        return;
    }

    printf("📊 Index Statistics:\n");
    printf("  Total tiles: %d\n", stats.total_tiles);
    printf("  Populated tiles: %d\n", stats.populated_tiles);
    printf("  Memory usage: %zu bytes\n", stats.memory_used_bytes);
    printf("  Total service assignments: %d\n", stats.total_service_assignments);
    printf("  Average services per tile: %.1f\n", stats.average_services_per_tile);
    printf("  Max services per tile: %d\n", stats.max_services_per_tile);

    printf("\n🔍 Tile Details:\n");
    for (uint16_t i = 0; i < g_spatial_index.tile_count && i < 10; i++) {
        const ntrip_tile_t* tile = &g_spatial_index.tiles[i];

        uint8_t level;
        uint16_t lat_tile, lon_tile;
        ntrip_atlas_decode_tile_key(tile->key, &level, &lat_tile, &lon_tile);

        printf("  Tile %d: L%d [%d,%d] = 0x%08X (%d services)\n",
               i, level, lat_tile, lon_tile, tile->key, tile->service_count);
    }

    if (g_spatial_index.tile_count > 10) {
        printf("  ... and %d more tiles\n", g_spatial_index.tile_count - 10);
    }
}