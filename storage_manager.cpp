#include "storage_manager.h"
#include "config.h"
#include <algorithm>

// Static member definitions
SemaphoreHandle_t StorageManager::sdMutex = NULL;
bool StorageManager::initialized = false;

bool StorageManager::init() {
  Serial.println("Initializing storage manager...");
  
  // Create mutex for thread safety
  if (sdMutex == NULL) {
    sdMutex = xSemaphoreCreateMutex();
    if (sdMutex == NULL) {
      Serial.println("Failed to create SD mutex!");
      return false;
    }
  }
  
  // Initialize SD card
  if (!takeMutex()) {
    return false;
  }
  
  // Try 1-bit mode first (more compatible)
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("SD Card mount failed in 1-bit mode, trying 4-bit...");
    
    // Try 4-bit mode
    if (!SD_MMC.begin()) {
      Serial.println("SD Card mount failed!");
      giveMutex();
      return false;
    }
  }
  
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    giveMutex();
    return false;
  }
  
  Serial.print("SD Card Type: ");
  switch (cardType) {
    case CARD_MMC:  Serial.println("MMC"); break;
    case CARD_SD:   Serial.println("SDSC"); break;
    case CARD_SDHC: Serial.println("SDHC"); break;
    default:        Serial.println("UNKNOWN"); break;
  }
  
  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  uint64_t totalBytes = SD_MMC.totalBytes();
  uint64_t usedBytes = SD_MMC.usedBytes();
  
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  Serial.printf("Total space: %.2f GB\n", totalBytes / 1024.0 / 1024.0 / 1024.0);
  Serial.printf("Used space: %.2f GB\n", usedBytes / 1024.0 / 1024.0 / 1024.0);
  Serial.printf("Free space: %.2f GB\n", (totalBytes - usedBytes) / 1024.0 / 1024.0 / 1024.0);
  
  giveMutex();
  
  initialized = verifyCard();
  
  if (initialized) {
    Serial.println("Storage manager initialized successfully");
  }
  
  return initialized;
}

bool StorageManager::verifyCard() {
  const char* testFile = "/test_write.tmp";
  
  if (!takeMutex()) {
    return false;
  }
  
  // Test write
  File file = SD_MMC.open(testFile, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create test file");
    giveMutex();
    return false;
  }
  
  const char* testData = "SD card test";
  size_t written = file.print(testData);
  file.close();
  
  if (written != strlen(testData)) {
    Serial.println("Failed to write test data");
    SD_MMC.remove(testFile);
    giveMutex();
    return false;
  }
  
  // Test read
  file = SD_MMC.open(testFile, FILE_READ);
  if (!file) {
    Serial.println("Failed to read test file");
    SD_MMC.remove(testFile);
    giveMutex();
    return false;
  }
  
  String readData = file.readString();
  file.close();
  
  // Clean up
  SD_MMC.remove(testFile);
  giveMutex();
  
  if (readData != testData) {
    Serial.println("Read data doesn't match written data");
    return false;
  }
  
  Serial.println("SD card verified successfully");
  return true;
}

File StorageManager::openFile(const String& path, const char* mode) {
  if (!initialized || !takeMutex(1000)) {
    return File();
  }
  
  File file = SD_MMC.open(path.c_str(), mode);
  giveMutex();
  
  return file;
}

bool StorageManager::closeFile(File& file) {
  if (file) {
    file.close();
    return true;
  }
  return false;
}

bool StorageManager::exists(const String& path) {
  if (!initialized || !takeMutex(1000)) {
    return false;
  }
  
  bool result = SD_MMC.exists(path.c_str());
  giveMutex();
  
  return result;
}

bool StorageManager::remove(const String& path) {
  if (!initialized || !takeMutex(1000)) {
    return false;
  }
  
  bool result = SD_MMC.remove(path.c_str());
  giveMutex();
  
  return result;
}

bool StorageManager::mkdir(const String& path) {
  if (!initialized || !takeMutex(1000)) {
    return false;
  }
  
  bool result = SD_MMC.mkdir(path.c_str());
  giveMutex();
  
  return result;
}

bool StorageManager::rmdir(const String& path) {
  if (!initialized || !takeMutex(1000)) {
    return false;
  }
  
  bool result = SD_MMC.rmdir(path.c_str());
  giveMutex();
  
  return result;
}

std::vector<String> StorageManager::listDirectory(const String& path) {
  std::vector<String> files;
  
  if (!initialized || !takeMutex(1000)) {
    return files;
  }
  
  File root = SD_MMC.open(path.c_str());
  if (!root || !root.isDirectory()) {
    giveMutex();
    return files;
  }
  
  File file = root.openNextFile();
  while (file) {
    files.push_back(String(file.name()));
    file = root.openNextFile();
  }
  
  root.close();
  giveMutex();
  
  return files;
}

std::vector<String> StorageManager::getCaptureDirectories() {
  std::vector<String> directories;
  
  if (!initialized || !takeMutex(1000)) {
    return directories;
  }
  
  File root = SD_MMC.open("/");
  if (!root || !root.isDirectory()) {
    giveMutex();
    return directories;
  }
  
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      String dirName = file.name();
      if (dirName.startsWith("/capture_")) {
        directories.push_back(dirName);
      }
    }
    file = root.openNextFile();
  }
  
  root.close();
  giveMutex();
  
  // Sort by name (which includes timestamp)
  std::sort(directories.begin(), directories.end());
  
  return directories;
}

bool StorageManager::removeDirectoryRecursively(const String& path) {
  if (!initialized || !takeMutex(5000)) { // Longer timeout for recursive operation
    return false;
  }
  
  // First, collect all files in the directory
  std::vector<String> filesToDelete;
  
  File dir = SD_MMC.open(path.c_str());
  if (!dir || !dir.isDirectory()) {
    giveMutex();
    return false;
  }
  
  File file = dir.openNextFile();
  while (file) {
    filesToDelete.push_back(String(file.name()));
    file.close();
    file = dir.openNextFile();
  }
  dir.close();
  
  // Delete all files
  bool success = true;
  for (const String& filePath : filesToDelete) {
    if (!SD_MMC.remove(filePath.c_str())) {
      Serial.println("Failed to remove: " + filePath);
      success = false;
    }
  }
  
  // Remove the directory itself
  if (success) {
    success = SD_MMC.rmdir(path.c_str());
  }
  
  giveMutex();
  return success;
}

void StorageManager::getSpaceInfo(uint64_t& totalBytes, uint64_t& usedBytes) {
  if (!initialized || !takeMutex(1000)) {
    totalBytes = 0;
    usedBytes = 0;
    return;
  }
  
  totalBytes = SD_MMC.totalBytes();
  usedBytes = SD_MMC.usedBytes();
  
  giveMutex();
}

bool StorageManager::ensureMinimumSpace() {
  uint64_t totalBytes, usedBytes;
  getSpaceInfo(totalBytes, usedBytes);
  
  uint64_t freeBytes = totalBytes - usedBytes;
  uint64_t minFreeBytes = (uint64_t)Config::storage.MIN_FREE_SPACE_MB * 1024ULL * 1024ULL;
  
  Serial.printf("Storage check: %.1f GB free, %.1f GB required\n",
                freeBytes / 1024.0 / 1024.0 / 1024.0,
                minFreeBytes / 1024.0 / 1024.0 / 1024.0);
  
  if (freeBytes >= minFreeBytes) {
    return true;
  }
  
  Serial.println("Low space detected, cleaning up...");
  
  // Get all directories sorted by age
  std::vector<String> directories = getCaptureDirectories();
  std::vector<String> sortedDirs = getDirectoriesByAge(directories);
  
  // Remove oldest directories until we have enough space
  int removed = 0;
  for (const String& dir : sortedDirs) {
    if (removeDirectoryRecursively(dir)) {
      removed++;
      Serial.println("Removed: " + dir);
      
      // Check if we have enough space now
      getSpaceInfo(totalBytes, usedBytes);
      freeBytes = totalBytes - usedBytes;
      
      if (freeBytes >= minFreeBytes) {
        Serial.printf("Space recovered after removing %d directories\n", removed);
        return true;
      }
    }
  }
  
  Serial.printf("Cleanup complete. Removed %d directories\n", removed);
  return freeBytes >= minFreeBytes;
}

void StorageManager::performCleanup() {
  Serial.println("\n=== Storage Cleanup ===");
  
  // Remove old directories
  cleanupOldDirectories();
  
  // Ensure minimum space
  ensureMinimumSpace();
  
  // Show current space
  uint64_t totalBytes, usedBytes;
  getSpaceInfo(totalBytes, usedBytes);
  Serial.printf("Current usage: %.1f/%.1f GB\n",
                usedBytes / 1024.0 / 1024.0 / 1024.0,
                totalBytes / 1024.0 / 1024.0 / 1024.0);
  
  Serial.println("======================\n");
}

time_t StorageManager::extractTimestampFromDirectory(const String& directory) {
  // Parse directory name: /capture_YYYYMMDD_HHMMSS
  if (!directory.startsWith("/capture_") || directory.length() < 24) {
    return 0;
  }
  
  // Extract components
  int year = directory.substring(9, 13).toInt();
  int month = directory.substring(13, 15).toInt();
  int day = directory.substring(15, 17).toInt();
  int hour = directory.substring(18, 20).toInt();
  int minute = directory.substring(20, 22).toInt();
  int second = directory.substring(22, 24).toInt();
  
  // Validate ranges
  if (year < 2020 || year > 2099 || month < 1 || month > 12 ||
      day < 1 || day > 31 || hour > 23 || minute > 59 || second > 59) {
    return 0;
  }
  
  // Convert to time_t
  struct tm timeinfo = {0};
  timeinfo.tm_year = year - 1900;
  timeinfo.tm_mon = month - 1;
  timeinfo.tm_mday = day;
  timeinfo.tm_hour = hour;
  timeinfo.tm_min = minute;
  timeinfo.tm_sec = second;
  
  return mktime(&timeinfo);
}

bool StorageManager::writeLogEntry(const String& filename, const String& entry) {
  File file = openFile(filename, "a"); // Append mode
  if (!file) {
    return false;
  }
  
  // Add timestamp to entry
  time_t now;
  time(&now);
  
  file.print(ctime(&now));
  file.print(" - ");
  file.println(entry);
  
  closeFile(file);
  return true;
}

void StorageManager::cleanupOldDirectories() {
  Serial.println("Checking for old directories...");
  
  time_t now;
  time(&now);
  time_t cutoffTime = now - (Config::storage.DIRECTORY_RETENTION_DAYS * 24 * 60 * 60);
  
  std::vector<String> directories = getCaptureDirectories();
  int removed = 0;
  
  for (const String& dir : directories) {
    time_t dirTime = extractTimestampFromDirectory(dir);
    if (dirTime > 0 && dirTime < cutoffTime) {
      Serial.println("Removing old directory: " + dir);
      if (removeDirectoryRecursively(dir)) {
        removed++;
      }
    }
  }
  
  Serial.printf("Removed %d old directories\n", removed);
}

std::vector<String> StorageManager::getDirectoriesByAge(const std::vector<String>& directories) {
  std::vector<std::pair<time_t, String>> timestampedDirs;
  
  // Get timestamps for all directories
  for (const String& dir : directories) {
    time_t timestamp = extractTimestampFromDirectory(dir);
    if (timestamp > 0) {
      timestampedDirs.push_back(std::make_pair(timestamp, dir));
    }
  }
  
  // Sort by timestamp (oldest first)
  std::sort(timestampedDirs.begin(), timestampedDirs.end());
  
  // Extract sorted directory names
  std::vector<String> sortedDirs;
  for (const auto& pair : timestampedDirs) {
    sortedDirs.push_back(pair.second);
  }
  
  return sortedDirs;
}

bool StorageManager::takeMutex(uint32_t timeoutMs) {
  if (sdMutex == NULL) {
    return false;
  }
  
  return xSemaphoreTake(sdMutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void StorageManager::giveMutex() {
  if (sdMutex != NULL) {
    xSemaphoreGive(sdMutex);
  }
}