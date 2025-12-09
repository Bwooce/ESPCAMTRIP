#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#if defined(ARDUINO_XIAO_ESP32S3)
#include <SD.h>
#endif
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class StorageManager {
public:
  // Initialization
  static bool init();
  static bool verifyCard();
  
  // File operations (WARNING: openFile/closeFile has race condition)
  // Mutex is released before file handle returned - use atomic operations when possible
  static File openFile(const String& path, const char* mode);
  static bool closeFile(File& file);
  
  // Thread-safe atomic file operations (recommended)
  static bool writeFileAtomic(const String& path, const uint8_t* data, size_t size);
  static bool readFileAtomic(const String& path, std::vector<uint8_t>& data);
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