#include "upload_manager.h"
#include "config.h"
#include "storage_manager.h"
#include "wifi_manager.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <mbedtls/md.h>
#include <mbedtls/base64.h>

// Static member definitions
std::vector<String> UploadManager::uploadedDirectories;
uint32_t UploadManager::totalUploaded = 0;
uint32_t UploadManager::totalFailed = 0;

void UploadManager::loadTracking() {
  Serial.println("Loading upload tracking...");
  
  uploadedDirectories.clear();
  
  if (!StorageManager::exists(Config::storage.UPLOAD_TRACKING_FILE)) {
    Serial.println("No upload tracking file found");
    return;
  }
  
  File file = StorageManager::openFile(Config::storage.UPLOAD_TRACKING_FILE, "r");
  if (!file) {
    Serial.println("Failed to open upload tracking file");
    return;
  }
  
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      uploadedDirectories.push_back(line);
    }
  }
  
  StorageManager::closeFile(file);
  
  Serial.printf("Loaded %u uploaded directories\n", uploadedDirectories.size()); // Changed %d to %u
}

void UploadManager::saveTracking() {
  File file = StorageManager::openFile(Config::storage.UPLOAD_TRACKING_FILE, "w");
  if (!file) {
    Serial.println("Failed to save upload tracking");
    return;
  }
  
  for (const String& dir : uploadedDirectories) {
    file.println(dir);
  }
  
  StorageManager::closeFile(file);
}

bool UploadManager::isDirectoryUploaded(const String& directory) {
  for (const String& uploaded : uploadedDirectories) {
    if (uploaded == directory) {
      return true;
    }
  }
  return false;
}

void UploadManager::markDirectoryAsUploaded(const String& directory) {
  if (!isDirectoryUploaded(directory)) {
    uploadedDirectories.push_back(directory);
    saveTracking();
    totalUploaded++;
  }
}

void UploadManager::removeFromTracking(const String& directory) {
  uploadedDirectories.erase(
    std::remove(uploadedDirectories.begin(), uploadedDirectories.end(), directory),
    uploadedDirectories.end()
  );
  saveTracking();
}

void UploadManager::uploadPendingDirectories() {
  Serial.println("\n=== UPLOAD MANAGER ===");
  
  // Check WiFi connection
  if (!WiFiManager::isConnected()) {
    Serial.println("No WiFi connection - cannot upload");
    return;
  }
  
  // Get all capture directories
  std::vector<String> allDirs = StorageManager::getCaptureDirectories();
  std::vector<String> pendingDirs;
  
  // Find directories not yet uploaded
  for (const String& dir : allDirs) {
    if (!isDirectoryUploaded(dir)) {
      pendingDirs.push_back(dir);
    }
  }
  
  if (pendingDirs.empty()) {
    Serial.println("No pending uploads");
    return;
  }
  
  Serial.printf("Found %d directories to upload\n", pendingDirs.size());
  
  // Upload each directory
  int successCount = 0;
  for (const String& dir : pendingDirs) {
    Serial.println("\nUploading: " + dir);
    
    bool success = false;
    for (size_t retry = 0; retry < Config::s3.MAX_UPLOAD_RETRIES; retry++) {
      if (uploadDirectory(dir)) {
        markDirectoryAsUploaded(dir);
        successCount++;
        success = true;
        Serial.println("✓ Upload successful");
        break;
      } else {
        Serial.printf("✗ Upload failed (attempt %d/%d)\n", 
                      retry + 1, Config::s3.MAX_UPLOAD_RETRIES);
        if (retry < Config::s3.MAX_UPLOAD_RETRIES - 1) {
          // Exponential backoff: 2^retry * 1000ms (1s, 2s, 4s, 8s...)
          uint32_t backoffDelay = (1 << retry) * 1000;
          // Cap at 30 seconds
          backoffDelay = min(backoffDelay, (uint32_t)30000U);
          Serial.printf("Waiting %lu ms before retry...\n", (unsigned long)backoffDelay);
          delay(backoffDelay);
        }
      }
    }
    
    if (!success) {
      totalFailed++;
    }
  }
  
  Serial.printf("\n=== Upload complete: %d/%d successful ===\n", 
                successCount, pendingDirs.size());
}

bool UploadManager::uploadDirectory(const String& directoryPath) {
  std::vector<String> files = StorageManager::listDirectory(directoryPath);
  
  int uploadCount = 0;
  int successCount = 0;
  
  for (const String& filename : files) {
    if (filename.endsWith(".jpg")) {
      uploadCount++;
      
      File file = StorageManager::openFile(filename, "r");
      if (!file) {
        Serial.println("Failed to open: " + filename);
        continue;
      }
      
      Serial.printf("  Uploading %s...", filename.c_str());
      
      if (uploadFile(file, directoryPath)) {
        successCount++;
        Serial.println(" ✓");
      } else {
        Serial.println(" ✗");
      }
      
      StorageManager::closeFile(file);
    }
  }
  
  return (successCount == uploadCount && uploadCount > 0);
}

bool UploadManager::uploadFile(File& file, const String& directoryPath) {
  if (!file) return false;
  
  // Generate S3 key
  String dirName = extractDirectoryName(directoryPath);
  String fileName = extractFileName(file.name());
  String s3Key = dirName + "/" + fileName;
  
  // Generate URL
  String url = generateS3Url(s3Key);
  
  // Prepare HTTP client
  WiFiClientSecure client;
  client.setInsecure(); // For development - use proper certs in production
  
  HTTPClient http;
  http.begin(client, url);
  http.setTimeout(30000); // 30 second timeout
  
  // Set headers
  http.addHeader("Content-Type", "image/jpeg");
  http.addHeader("Content-Length", String(file.size()));
  
  // Generate timestamp
  time_t now;
  time(&now);
  char timestamp[20];
  strftime(timestamp, sizeof(timestamp), "%Y%m%dT%H%M%SZ", gmtime(&now));
  http.addHeader("x-amz-date", timestamp);
  
  // Add authorization (simplified - implement proper AWS Signature V4)
  String authHeader = generateAWSSignature("PUT", "/" + s3Key, "", "", timestamp);
  http.addHeader("Authorization", authHeader);
  
  // Read file into memory (for small files)
  // For larger files, implement streaming upload
  size_t fileSize = file.size();
  if (fileSize > 1024 * 1024) { // 1MB limit for this implementation
    Serial.println("File too large for simple upload");
    return false;
  }
  
  uint8_t* buffer = (uint8_t*)malloc(fileSize);
  if (!buffer) {
    Serial.println("Failed to allocate upload buffer");
    return false;
  }
  
  file.seek(0);
  size_t bytesRead = file.read(buffer, fileSize);
  
  bool success = false;
  if (bytesRead == fileSize) {
    int httpCode = http.PUT(buffer, fileSize);
    
    if (httpCode == 200 || httpCode == 201) {
      success = true;
    } else {
      Serial.printf("HTTP error: %d", httpCode);
    }
  }
  
  free(buffer);
  http.end();
  
  return success;
}

uint32_t UploadManager::getUploadedCount() {
  return uploadedDirectories.size();
}

uint32_t UploadManager::getPendingCount() {
  std::vector<String> allDirs = StorageManager::getCaptureDirectories();
  uint32_t pending = 0;
  
  for (const String& dir : allDirs) {
    if (!isDirectoryUploaded(dir)) {
      pending++;
    }
  }
  
  return pending;
}

void UploadManager::printStatistics() {
  Serial.println("\n=== Upload Statistics ===");
  Serial.printf("Total uploaded: %u directories\n", totalUploaded);
  Serial.printf("Total failed: %u uploads\n", totalFailed);
  Serial.printf("Currently tracked: %u directories\n", uploadedDirectories.size());
  Serial.printf("Pending uploads: %u directories\n", getPendingCount());
  Serial.println("========================\n");
}

String UploadManager::generateS3Url(const String& key) {
  return "https://" + String(Config::s3.bucket) + ".s3." + 
         String(Config::s3.region) + ".amazonaws.com/" + key;
}

String UploadManager::generateAWSSignature(const String& method, const String& uri,
                                         const String& queryString, const String& payload,
                                         const String& timestamp) {
  // This is a simplified implementation
  // For production, implement proper AWS Signature V4
  // See: https://docs.aws.amazon.com/general/latest/gr/sigv4_signing.html
  
  String auth = String(Config::s3.access_key) + ":" + String(Config::s3.secret_key);
  return "Basic " + base64Encode(auth);
}

String UploadManager::extractDirectoryName(const String& path) {
  int lastSlash = path.lastIndexOf('/');
  if (lastSlash >= 0) {
    return path.substring(lastSlash + 1);
  }
  return path;
}

String UploadManager::extractFileName(const String& path) {
  int lastSlash = path.lastIndexOf('/');
  if (lastSlash >= 0) {
    return path.substring(lastSlash + 1);
  }
  return path;
}

String UploadManager::base64Encode(const String& input) {
  size_t outputLen = 0;
  mbedtls_base64_encode(NULL, 0, &outputLen, 
                       (const unsigned char*)input.c_str(), input.length());
  
  unsigned char* output = (unsigned char*)malloc(outputLen);
  if (output) {
    mbedtls_base64_encode(output, outputLen, &outputLen,
                         (const unsigned char*)input.c_str(), input.length());
    String encoded = String((char*)output);
    free(output);
    return encoded;
  }
  
  return "";
}