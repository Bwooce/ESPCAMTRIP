#ifndef PSRAM_MANAGER_H
#define PSRAM_MANAGER_H

#include <Arduino.h>

/**
 * PSRAM Manager for ESP32-S3 Camera Operations
 *
 * The XIAO ESP32S3 Sense has 8MB external PSRAM that's critical for:
 * - Camera frame buffers (UXGA = ~200KB per frame)
 * - AprilTag image processing buffers
 * - EXIF processing temporary buffers
 * - Large data structures
 *
 * Without PSRAM, camera operations fail at high resolutions
 */

class PSRAMManager {
private:
    static bool psram_available;
    static size_t psram_size;
    static size_t psram_free;

public:
    /**
     * Initialize and check PSRAM availability
     * Call during system startup
     */
    static bool init();

    /**
     * Check if PSRAM is available and properly initialized
     */
    static bool isAvailable() { return psram_available; }

    /**
     * Get PSRAM memory information
     */
    static size_t getTotalSize() { return psram_size; }
    static size_t getFreeSize();
    static size_t getUsedSize();
    static float getUsagePercent();

    /**
     * PSRAM-aware memory allocation
     * Automatically uses PSRAM for large allocations
     */
    static void* allocate(size_t size, bool force_psram = false);
    static void* reallocate(void* ptr, size_t size);
    static void deallocate(void* ptr);

    /**
     * Camera-specific PSRAM functions
     */
    static void* allocateCameraBuffer(size_t size);
    static void* allocateImageBuffer(size_t width, size_t height, size_t bytes_per_pixel = 1);

    /**
     * Memory debugging and monitoring
     */
    static void printMemoryStatus();
    static void printDetailedStatus();

    /**
     * Memory optimization functions
     */
    static void defragment();
    static bool checkMemoryHealth();

    /**
     * Emergency memory management
     */
    static void emergencyCleanup();
    static bool hasLowMemory();
};

// Memory allocation macros for PSRAM
#define PSRAM_MALLOC(size) PSRAMManager::allocate(size, true)
#define PSRAM_CALLOC(num, size) PSRAMManager::allocate((num) * (size), true)
#define PSRAM_FREE(ptr) PSRAMManager::deallocate(ptr)

// Camera buffer allocation
#define CAMERA_BUFFER_MALLOC(size) PSRAMManager::allocateCameraBuffer(size)

#endif // PSRAM_MANAGER_H