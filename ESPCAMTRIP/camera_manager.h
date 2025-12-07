#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <Arduino.h>
#include "esp_camera.h"

class CameraManager {
public:
  // Initialization and control
  static bool init();
  static void deinit();
  static bool isInitialized();
  
  // Power management
  static void powerUp();
  static void powerDown();
  
  // Capture control
  static bool startCapture();
  static void stopCapture();
  static bool capturePhoto();
  
  // Settings
  static bool setQuality(int quality);
  static bool setFrameSize(framesize_t size);
  static bool setBrightness(int brightness);
  static bool setContrast(int contrast);
  static bool setSaturation(int saturation);
  
  // Status
  static int getPhotoCount();
  static String getCurrentDirectory();
  static bool isCameraPin(int pin); // Moved to public

  // GPS Geotagging
  static bool enableGeotagging(bool enable);
  static bool isGeotaggingEnabled();
  static bool captureGeotaggedPhoto();

private:
  static bool initialized;
  static bool capturing;
  static int photoCount;
  static String currentDirectory;
  static bool geotaggingEnabled;

  // Camera configuration
  static camera_config_t getCameraConfig();
  static void applyCameraSettings();
  static bool createCaptureDirectory();
  static String generateFilename();
  static String generateGeotaggedFilename();
  static bool saveGPSMetadata(const String& photoFilename);
  
  // Pin validation
  // static bool isCameraPin(int pin); // Removed from private
};

#endif // CAMERA_MANAGER_H