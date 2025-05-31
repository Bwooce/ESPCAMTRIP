#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class StorageManager {
public:
  // Initialization
  static bool init();
  static bool verifyCard();
  
  // File operations (thread-safe)
  static File openFile(const String& path, const char* mode);
  static bool closeFile(File& file);
  static bool exists(const String& path);
  static bool remove(const String& path);
  static bool mkdir(const String& path);
  static bool rmdir(const String& path);
  
  // Directory operations
  static std::vector<String> listDirectory(const String& path);
  static std::vector<String> getCaptureDirectories();
  static bool removeDirectoryRecursively(const String& path);
  
  // Space management
  static void getSpaceInfo(uint64_t& totalBytes, uint64_t& usedBytes);
  static bool ensureMinimumSpace();
  static void performCleanup();
  
  // Utility functions
  static time_t extractTimestampFromDirectory(const String& directory);
  static bool writeLogEntry(const String& filename, const String& entry);
  
private:
  static SemaphoreHandle_t sdMutex;
  static bool initialized;
  
  // Cleanup functions
  static void cleanupOldDirectories();
  static std::vector<String> getDirectoriesByAge(const std::vector<String>& directories);
  
  // Thread safety wrappers
  static bool takeMutex(uint32_t timeoutMs = portMAX_DELAY);
  static void giveMutex();
};

#endif // STORAGE_MANAGER_H