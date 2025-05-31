#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

class SystemState {
public:
  // System initialization
  static void init();
  
  // Camera state
  static bool isCapturing();
  static void setCapturing(bool state);
  static bool isCameraInUse();
  static void setCameraInUse(bool inUse);
  
  // Directory management
  static String getCurrentDirectory();
  static void setCurrentDirectory(const String& dir);
  
  // Timing
  static unsigned long getLastCaptureTime();
  static void setLastCaptureTime(unsigned long time);
  static unsigned long getLastActivityTime();
  static void updateActivity();
  
  // Photo counting
  static int getPhotoCount();
  static void incrementPhotoCount();
  static void resetPhotoCount();
  
  // NTRIP coordination
  static void updateNtripActivity();
  static bool isNtripIdle();
  static unsigned long getLastNtripActivity();
  
  // System status
  static bool isSystemIdle();
  static void printStatus();
  
private:
  // State variables
  static bool capturing;
  static bool cameraInUse;
  static String currentDirectory;
  static unsigned long lastCaptureTime;
  static unsigned long lastActivityTime;
  static int photoCount;
  
  // NTRIP coordination
  static volatile bool ntripServerActive;
  static volatile unsigned long lastNtripActivity;
  
  // Thread safety
  static portMUX_TYPE stateMux;
  
  // Helper functions
  static void enterCritical();
  static void exitCritical();
};

#endif // SYSTEM_STATE_H