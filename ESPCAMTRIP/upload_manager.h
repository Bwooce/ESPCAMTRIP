#ifndef UPLOAD_MANAGER_H
#define UPLOAD_MANAGER_H

#include <Arduino.h>
#include <FS.h> // Added FS.h for File type
#include <vector>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

class UploadManager {
public:
  // Upload tracking
  static void loadTracking();
  static void saveTracking();
  static bool isDirectoryUploaded(const String& directory);
  static void markDirectoryAsUploaded(const String& directory);
  static void removeFromTracking(const String& directory);
  
  // Upload operations
  static void uploadPendingDirectories();
  static bool uploadDirectory(const String& directoryPath);
  static bool uploadFile(File& file, const String& directoryPath);
  
  // Statistics
  static uint32_t getUploadedCount();
  static uint32_t getPendingCount();
  static void printStatistics();
  
private:
  static std::vector<String> uploadedDirectories;
  static uint32_t totalUploaded;
  static uint32_t totalFailed;
  
  // AWS S3 functions
  static String generateS3Url(const String& key);
  static String generateAWSSignature(const String& method, const String& uri, 
                                   const String& queryString, const String& payload, 
                                   const String& timestamp);
  static bool performS3Upload(const String& url, File& file, const String& contentType);
  
  // Utility functions
  static String extractDirectoryName(const String& path);
  static String extractFileName(const String& path);
  static String base64Encode(const String& input);
};

#endif // UPLOAD_MANAGER_H