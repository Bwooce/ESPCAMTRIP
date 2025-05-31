#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_pm.h>

class PowerManager {
public:
  // Initialization
  static void init();
  
  // Power management
  static void coordinatePowerManagement();
  static void enterLightSleep(uint32_t durationMs);
  static void setCpuFrequency(uint32_t freqMhz);
  
  // Peripheral management
  static void disableUnusedPeripherals();
  static void disableBluetooth();
  static void configureUnusedPins();
  
  // Wake configuration
  static void configureWakeupSources();
  static void handleWakeup();
  
  // Battery monitoring (if supported)
  static float getBatteryVoltage();
  static uint8_t getBatteryPercentage();
  static bool isLowBattery();
  
private:
  static bool initialized;
  static uint32_t currentCpuFreq;
  static unsigned long lastPowerCheck;
  
  // Internal power states
  static bool isCpuScalingEnabled();
  static void updatePowerState();
  
  // Pin management helpers
  static bool isSystemPin(int pin);
  static bool isPowerExemptPin(int pin);
};

#endif // POWER_MANAGER_H
