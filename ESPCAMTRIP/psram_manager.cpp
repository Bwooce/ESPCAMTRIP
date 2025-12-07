#include "psram_manager.h"
#include "esp_heap_caps.h"
#include "esp_psram.h"

// Static member definitions
bool PSRAMManager::psram_available = false;
size_t PSRAMManager::psram_size = 0;
size_t PSRAMManager::psram_free = 0;

bool PSRAMManager::init() {
    Serial.println("Initializing PSRAM Manager...");

    // Check if PSRAM is available
    if (ESP.getPsramSize() == 0) {
        Serial.println("ERROR: PSRAM not detected!");
        Serial.println("Solution:");
        Serial.println("1. Ensure XIAO ESP32S3 Sense (with PSRAM) is used");
        Serial.println("2. In Arduino IDE: Tools > PSRAM > 'OPI PSRAM'");
        Serial.println("3. Recompile and upload");
        psram_available = false;
        return false;
    }

    psram_size = ESP.getPsramSize();
    psram_free = ESP.getFreePsram();
    psram_available = true;

    Serial.printf("PSRAM detected: %.2f MB total, %.2f MB free\n",
                  psram_size / 1024.0 / 1024.0,
                  psram_free / 1024.0 / 1024.0);

    // Test PSRAM allocation
    void* test_ptr = heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);
    if (test_ptr) {
        heap_caps_free(test_ptr);
        Serial.println("PSRAM allocation test: PASSED");
    } else {
        Serial.println("WARNING: PSRAM allocation test FAILED");
        return false;
    }

    // Configure heap allocation preferences
    // Prefer PSRAM for large allocations (>1KB)
    heap_caps_malloc_extmem_enable(1024);

    Serial.println("PSRAM Manager initialized successfully");
    printMemoryStatus();

    return true;
}

size_t PSRAMManager::getFreeSize() {
    return psram_available ? ESP.getFreePsram() : 0;
}

size_t PSRAMManager::getUsedSize() {
    return psram_available ? (psram_size - ESP.getFreePsram()) : 0;
}

float PSRAMManager::getUsagePercent() {
    if (!psram_available || psram_size == 0) return 0.0f;
    return (float)getUsedSize() / psram_size * 100.0f;
}

void* PSRAMManager::allocate(size_t size, bool force_psram) {
    if (!psram_available) {
        return malloc(size);
    }

    void* ptr = nullptr;

    if (force_psram || size >= 1024) {
        // Use PSRAM for large allocations or when forced
        ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
        if (ptr) {
            Serial.printf("PSRAM allocated: %u bytes\n", size);
        }
    }

    // Fallback to regular heap if PSRAM allocation failed
    if (!ptr) {
        ptr = malloc(size);
        if (ptr && size >= 1024) {
            Serial.printf("WARNING: Large allocation (%u bytes) using internal RAM\n", size);
        }
    }

    return ptr;
}

void* PSRAMManager::reallocate(void* ptr, size_t size) {
    if (!psram_available) {
        return realloc(ptr, size);
    }

    // Check if original pointer is in PSRAM
    if (heap_caps_get_allocated_size(ptr) > 0) {
        // Reallocate in PSRAM
        return heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM);
    } else {
        // Original was in internal RAM
        if (size >= 1024) {
            // Try to move to PSRAM for large size
            void* new_ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
            if (new_ptr && ptr) {
                size_t old_size = heap_caps_get_allocated_size(ptr);
                memcpy(new_ptr, ptr, min(old_size, size));
                free(ptr);
                return new_ptr;
            }
        }
        return realloc(ptr, size);
    }
}

void PSRAMManager::deallocate(void* ptr) {
    if (ptr) {
        // Use heap_caps_free which automatically handles PSRAM vs internal RAM
        heap_caps_free(ptr);
    }
}

void* PSRAMManager::allocateCameraBuffer(size_t size) {
    if (!psram_available) {
        Serial.printf("WARNING: Camera buffer (%u bytes) using internal RAM - may cause instability\n", size);
        return malloc(size);
    }

    void* ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    if (ptr) {
        Serial.printf("Camera buffer allocated in PSRAM: %u bytes\n", size);
    } else {
        Serial.printf("CRITICAL: Failed to allocate camera buffer (%u bytes) in PSRAM\n", size);
        // Try internal RAM as last resort
        ptr = malloc(size);
    }

    return ptr;
}

void* PSRAMManager::allocateImageBuffer(size_t width, size_t height, size_t bytes_per_pixel) {
    size_t size = width * height * bytes_per_pixel;
    return allocateCameraBuffer(size);
}

void PSRAMManager::printMemoryStatus() {
    Serial.println("\n--- Memory Status ---");

    // Internal RAM
    Serial.printf("Internal RAM: %u bytes free\n", ESP.getFreeHeap());

    // PSRAM
    if (psram_available) {
        Serial.printf("PSRAM: %.2f/%.2f MB (%.1f%% used)\n",
                      getUsedSize() / 1024.0 / 1024.0,
                      psram_size / 1024.0 / 1024.0,
                      getUsagePercent());
    } else {
        Serial.println("PSRAM: Not available");
    }

    Serial.println("--------------------\n");
}

void PSRAMManager::printDetailedStatus() {
    Serial.println("\n--- Detailed Memory Analysis ---");

    // Heap capabilities info
    multi_heap_info_t internal_info, spiram_info;

    heap_caps_get_info(&internal_info, MALLOC_CAP_INTERNAL);
    Serial.printf("Internal RAM:\n");
    Serial.printf("  Total: %u bytes\n", internal_info.total_free_bytes + internal_info.total_allocated_bytes);
    Serial.printf("  Free: %u bytes\n", internal_info.total_free_bytes);
    Serial.printf("  Allocated: %u bytes\n", internal_info.total_allocated_bytes);
    Serial.printf("  Largest free block: %u bytes\n", internal_info.largest_free_block);

    if (psram_available) {
        heap_caps_get_info(&spiram_info, MALLOC_CAP_SPIRAM);
        Serial.printf("PSRAM:\n");
        Serial.printf("  Total: %u bytes (%.2f MB)\n",
                      spiram_info.total_free_bytes + spiram_info.total_allocated_bytes,
                      (spiram_info.total_free_bytes + spiram_info.total_allocated_bytes) / 1024.0 / 1024.0);
        Serial.printf("  Free: %u bytes (%.2f MB)\n",
                      spiram_info.total_free_bytes,
                      spiram_info.total_free_bytes / 1024.0 / 1024.0);
        Serial.printf("  Allocated: %u bytes (%.2f MB)\n",
                      spiram_info.total_allocated_bytes,
                      spiram_info.total_allocated_bytes / 1024.0 / 1024.0);
        Serial.printf("  Largest free block: %u bytes\n", spiram_info.largest_free_block);
    }

    Serial.println("-------------------------------\n");
}

void PSRAMManager::defragment() {
    // ESP32 doesn't have explicit defragmentation, but we can check heap integrity
    if (psram_available) {
        bool integrity_ok = heap_caps_check_integrity_all(true);
        Serial.printf("Heap integrity check: %s\n", integrity_ok ? "PASSED" : "FAILED");
    }
}

bool PSRAMManager::checkMemoryHealth() {
    if (!psram_available) {
        return ESP.getFreeHeap() > 50000; // At least 50KB internal RAM
    }

    // Check both internal and PSRAM health
    bool internal_ok = ESP.getFreeHeap() > 30000; // At least 30KB internal RAM
    bool psram_ok = getFreeSize() > 1024 * 1024; // At least 1MB PSRAM

    return internal_ok && psram_ok;
}

void PSRAMManager::emergencyCleanup() {
    Serial.println("EMERGENCY: Performing memory cleanup...");

    // Force garbage collection if available
    // ESP32 doesn't have explicit GC, but we can check for memory leaks

    printDetailedStatus();

    if (!checkMemoryHealth()) {
        Serial.println("CRITICAL: Memory health check failed after cleanup!");
        Serial.println("System may need restart...");
    }
}

bool PSRAMManager::hasLowMemory() {
    if (!psram_available) {
        return ESP.getFreeHeap() < 100000; // Less than 100KB internal RAM
    }

    // Consider low memory if either is critically low
    bool internal_low = ESP.getFreeHeap() < 50000; // Less than 50KB internal
    bool psram_low = getFreeSize() < 512 * 1024;   // Less than 512KB PSRAM

    return internal_low || psram_low;
}