#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

class WiFiManager {
public:
  // WiFi management
  static bool connectWiFi();
  static void disconnectWiFi();
  static bool isConnected();
  static int getRSSI();
  static String getIPAddress();
  
  // Time synchronization
  static bool initializeTime();
  static bool isTimeSynchronized();
  static String getCurrentTimestamp();
  
  // Network monitoring
  static void checkConnection();
  static uint32_t getConnectionUptime();
  static uint32_t getDisconnectCount();
  
private:
  static bool connected;
  static unsigned long connectionStartTime;
  static uint32_t disconnectCount;
  static bool timeSynchronized;
  
  // Internal functions
  static void handleDisconnect();
  static bool waitForTimeSync(uint32_t timeoutMs);
};

#endif // WIFI_MANAGER_H