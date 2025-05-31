#include "system_state.h"
#include "config.h"

// Static member definitions
bool SystemState::capturing = false;
bool SystemState::cameraInUse = false;
String SystemState::currentDirectory = "";
unsigned long SystemState::lastCaptureTime = 0;
unsigned long SystemState::lastActivityTime = 0;
int SystemState::photoCount = 0;
volatile bool SystemState::ntripServerActive = true;
volatile unsigned long SystemState::lastNtripActivity = 0;
portMUX_TYPE SystemState::stateMux = portMUX_INITIALIZER_UNLOCKED;

void SystemState::init() {
  // Initialize all state variables
  capturing = false;
  cameraInUse = false;
  currentDirectory = "";
  lastCaptureTime = 0;
  lastActivityTime = millis();
  photoCount = 0;
  ntripServerActive = true;
  lastNtripActivity = millis();
  
  Serial.println("System state initialized");
}

bool SystemState::isCapturing() {
  bool result;
  enterCritical();
  result = capturing;
  exitCritical();
  return result;
}

void SystemState::setCapturing(bool state) {
  enterCritical();
  capturing = state;
  exitCritical();
}

bool SystemState::isCameraInUse() {
  bool result;
  enterCritical();
  result = cameraInUse;
  exitCritical();
  return result;
}

void SystemState::setCameraInUse(bool inUse) {
  enterCritical();
  cameraInUse = inUse;
  exitCritical();
}

String SystemState::getCurrentDirectory() {
  String result;
  enterCritical();
  result = currentDirectory;
  exitCritical();
  return result;
}

void SystemState::setCurrentDirectory(const String& dir) {
  enterCritical();
  currentDirectory = dir;
  exitCritical();
}

unsigned long SystemState::getLastCaptureTime() {
  unsigned long result;
  enterCritical();
  result = lastCaptureTime;
  exitCritical();
  return result;
}

void SystemState::setLastCaptureTime(unsigned long time) {
  enterCritical();
  lastCaptureTime = time;
  exitCritical();
}

unsigned long SystemState::getLastActivityTime() {
  unsigned long result;
  enterCritical();
  result = lastActivityTime;
  exitCritical();
  return result;
}

void SystemState::updateActivity() {
  enterCritical();
  lastActivityTime = millis();
  exitCritical();
}

int SystemState::getPhotoCount() {
  int result;
  enterCritical();
  result = photoCount;
  exitCritical();
  return result;
}

void SystemState::incrementPhotoCount() {
  enterCritical();
  photoCount++;
  exitCritical();
}

void SystemState::resetPhotoCount() {
  enterCritical();
  photoCount = 0;
  exitCritical();
}

void SystemState::updateNtripActivity() {
  enterCritical();
  lastNtripActivity = millis();
  exitCritical();
}

bool SystemState::isNtripIdle() {
  unsigned long lastActivity;
  enterCritical();
  lastActivity = lastNtripActivity;
  exitCritical();
  
  return (millis() - lastActivity > 5000); // 5 second idle threshold
}

unsigned long SystemState::getLastNtripActivity() {
  unsigned long result;
  enterCritical();
  result = lastNtripActivity;
  exitCritical();
  return result;
}

bool SystemState::isSystemIdle() {
  unsigned long lastActivity = getLastActivityTime();
  bool capturing = isCapturing();
  
  // System is idle if not capturing and no activity for 30 seconds
  return !capturing && (millis() - lastActivity > 30000);
}

void SystemState::printStatus() {
  Serial.println("\n=== System Status ===");
  Serial.printf("Capturing: %s\n", isCapturing() ? "Yes" : "No");
  Serial.printf("Camera in use: %s\n", isCameraInUse() ? "Yes" : "No");
  Serial.printf("Current directory: %s\n", getCurrentDirectory().c_str());
  Serial.printf("Photo count: %d\n", getPhotoCount());
  Serial.printf("Last activity: %lu seconds ago\n", (millis() - getLastActivityTime()) / 1000);
  Serial.printf("NTRIP idle: %s\n", isNtripIdle() ? "Yes" : "No");
  Serial.printf("System idle: %s\n", isSystemIdle() ? "Yes" : "No");
  Serial.println("====================\n");
}

void SystemState::enterCritical() {
  portENTER_CRITICAL(&stateMux);
}

void SystemState::exitCritical() {
  portEXIT_CRITICAL(&stateMux);
}