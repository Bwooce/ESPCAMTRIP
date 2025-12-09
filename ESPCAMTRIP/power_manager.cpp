#include "power_manager.h"
#include "config.h"
#include "system_state.h"
#include "camera_manager.h"
#include <esp_pm.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
// Bluetooth headers removed - not needed since BT is disabled by default
#include <esp_wifi.h>
#include <esp_err.h>
#include <driver/adc.h>

// Static member definitions
bool PowerManager::initialized = false;
uint32_t PowerManager::currentCpuFreq = 240;
unsigned long PowerManager::lastPowerCheck = 0;

// Battery monitoring configuration
// Define ENABLE_BATTERY_MONITORING to enable battery monitoring
// #define ENABLE_BATTERY_MONITORING

#ifdef ENABLE_BATTERY_MONITORING
  // Battery monitoring pins
  #if defined(ARDUINO_XIAO_ESP32S3)
    // Xiao ESP32S3 Sense: Check GPIO 9 (BAT voltage divider) or GPIO 4
    #define BATTERY_ADC_PIN 9     // Xiao ESP32S3: GPIO 9 (ADC1_CH8) - Battery voltage
    #define BATTERY_ADC_CHANNEL ADC1_CHANNEL_8
  #elif defined(CONFIG_IDF_TARGET_ESP32S3)
    #define BATTERY_ADC_PIN 4     // Generic ESP32-S3: GPIO 4 (ADC1_CH3)
    #define BATTERY_ADC_CHANNEL ADC1_CHANNEL_3
  #else
    #define BATTERY_ADC_PIN 34    // ESP32: GPIO 34 (ADC1_CH6)
    #define BATTERY_ADC_CHANNEL ADC1_CHANNEL_6
  #endif
  #define BATTERY_VOLTAGE_DIVIDER_RATIO 2.0  // Adjust based on your voltage divider
#endif

void PowerManager::init() {
  Serial.println("Initializing power management...");
  
  // Configure wake-up sources
  configureWakeupSources();
  
  // Disable unnecessary peripherals
  disableUnusedPeripherals();
  
  // Configure power management
  if (Config::power.ENABLE_OPTIMIZATION) {
    // Set initial CPU frequency
    setCpuFrequency(Config::power.CPU_FREQ_NORMAL);
    
    // Configure dynamic frequency scaling
    // Note: ESP32-S3 with PSRAM has limitations on power management
    // - Light sleep disabled (incompatible with PSRAM)
    // - Min frequency may need to be >= 80MHz for PSRAM stability
    esp_pm_config_t pm_config = {
      .max_freq_mhz = static_cast<int>(Config::power.CPU_FREQ_CAPTURE),
      .min_freq_mhz = 80,  // PSRAM requires >= 80MHz minimum
      .light_sleep_enable = false  // Disabled for PSRAM compatibility
    };

    esp_err_t ret = esp_pm_configure(&pm_config);
    if (ret == ESP_OK) {
      Serial.println("Dynamic frequency scaling enabled");
    } else {
      Serial.printf("Failed to configure power management: %s\n", esp_err_to_name(ret));
      Serial.println("Continuing with manual frequency control only");
      // Continue - manual frequency control will still work
    }
  }
  
#ifdef ENABLE_BATTERY_MONITORING
  // Initialize battery monitoring if available
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten((adc1_channel_t)BATTERY_ADC_CHANNEL, ADC_ATTEN_DB_12);
  Serial.printf("Battery monitoring enabled on GPIO %d\n", BATTERY_ADC_PIN);
#else
  Serial.println("Battery monitoring disabled (define ENABLE_BATTERY_MONITORING to enable)");
#endif
  
  initialized = true;
  lastPowerCheck = millis();
  
  Serial.println("Power management initialized");
}

void PowerManager::coordinatePowerManagement() {
  if (!initialized || !Config::power.ENABLE_OPTIMIZATION) {
    return;
  }
  
  // Only check every second to avoid overhead
  if (millis() - lastPowerCheck < 1000) {
    return;
  }
  lastPowerCheck = millis();
  
  // Update power state based on system activity
  updatePowerState();
  
#ifdef ENABLE_BATTERY_MONITORING
  // Check battery status if monitoring is enabled
  if (isLowBattery()) {
    Serial.println("WARNING: Low battery detected!");
    // Could trigger emergency upload or shutdown
  }
#endif
}

void PowerManager::enterLightSleep(uint32_t durationMs) {
  if (!initialized || durationMs == 0) {
    return;
  }
  
  // Configure timer wake-up
  esp_sleep_enable_timer_wakeup(durationMs * 1000ULL); // Convert to microseconds, ensure ULL for large values
  
  // Enter light sleep
  esp_light_sleep_start();
  
  // System resumes here after wake-up
  handleWakeup();
}

void PowerManager::setCpuFrequency(uint32_t freqMhz) {
  if (currentCpuFreq == freqMhz) {
    return;
  }
  
  // Validate frequency
  if (freqMhz != 240 && freqMhz != 160 && freqMhz != 80) {
    Serial.printf("Invalid CPU frequency: %lu MHz\n", (unsigned long)freqMhz);
    return;
  }
  
  // Set CPU frequency
  setCpuFrequencyMhz(freqMhz);
  currentCpuFreq = freqMhz;
  
  Serial.printf("CPU frequency set to %lu MHz\n", (unsigned long)freqMhz);
}

void PowerManager::disableUnusedPeripherals() {
  Serial.println("Disabling unused peripherals...");
  
  // Bluetooth is not enabled by default in ESP32 Arduino - no need to disable
  
  // Configure unused GPIO pins
  configureUnusedPins();
  
  // Disable unused peripherals
  // Note: Be careful not to disable peripherals in use!
  
  // Reduce WiFi power if not actively transmitting
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
  
  Serial.println("Peripherals configured for low power");
}

// Bluetooth function removed - not needed since BT is disabled by default

void PowerManager::configureUnusedPins() {
  // DISABLED: GPIO configuration causing ESP32-S3 strapping pin conflicts
  // The previous implementation tried to configure GPIO 0, 45, 46 which are
  // strapping pins on ESP32-S3 and cannot be reconfigured after boot.
  // This caused "GPIO_PIN mask error" and task watchdog resets.

  Serial.println("GPIO pin configuration disabled to prevent strapping pin conflicts");

  // TODO: Implement safe GPIO configuration that excludes strapping pins:
  // ESP32-S3 strapping pins to avoid: GPIO 0, 45, 46
  // ESP32-S3 camera pins in use: GPIO 3, 11, 12, 13, 14, 17, 18, 21, 38, 39, 40, 41, 42, 47
  // ESP32-S3 system pins to avoid: GPIO 19, 20 (USB), GPIO 26-32 (flash/PSRAM)
}

void PowerManager::configureWakeupSources() {
  // Configure GPIO wake-up sources
  gpio_num_t capturePin = (gpio_num_t)Config::pins.CAPTURE_TRIGGER_PIN;
  gpio_num_t uploadPin = (gpio_num_t)Config::pins.UPLOAD_TRIGGER_PIN;
  
  // Configure EXT0 wake-up (single pin)
  esp_sleep_enable_ext0_wakeup(capturePin, 0); // Wake on LOW
  
  // Configure EXT1 wake-up (multiple pins)
  uint64_t pin_mask = (1ULL << capturePin) | (1ULL << uploadPin);
  esp_sleep_enable_ext1_wakeup(pin_mask, ESP_EXT1_WAKEUP_ALL_LOW);
  
  // Configure timer wake-up (will be set when entering sleep)
  // esp_sleep_enable_timer_wakeup() - called when needed
  
  Serial.println("Wake-up sources configured");
}

void PowerManager::handleWakeup() {
  // Determine wake-up cause
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Wakeup by EXT0 (capture button)");
      SystemState::updateActivity();
      break;
      
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("Wakeup by EXT1 (GPIO pins)");
      SystemState::updateActivity();
      break;
      
    case ESP_SLEEP_WAKEUP_TIMER:
      // Timer wakeup - normal operation
      break;
      
    default:
      Serial.printf("Wakeup reason: %d\n", wakeup_reason);
      break;
  }
}

#ifdef ENABLE_BATTERY_MONITORING
float PowerManager::getBatteryVoltage() {
  // Read ADC value
  int adc_reading = adc1_get_raw((adc1_channel_t)BATTERY_ADC_CHANNEL);

  // Convert to voltage (assuming 12-bit ADC, 3.3V reference)
  float voltage = (adc_reading / 4095.0) * 3.3;

  // Apply voltage divider ratio
  voltage *= BATTERY_VOLTAGE_DIVIDER_RATIO;

  return voltage;
}

uint8_t PowerManager::getBatteryPercentage() {
  float voltage = getBatteryVoltage();

  // Battery percentage calculation (adjust for your battery type)
  // Example for single-cell LiPo: 4.2V = 100%, 3.0V = 0%
  const float MAX_VOLTAGE = 4.2;
  const float MIN_VOLTAGE = 3.0;

  if (voltage >= MAX_VOLTAGE) return 100;
  if (voltage <= MIN_VOLTAGE) return 0;

  return (uint8_t)((voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE) * 100);
}

bool PowerManager::isLowBattery() {
  // Consider battery low if below 20%
  return getBatteryPercentage() < 20;
}
#else
// Stub functions when battery monitoring is disabled
float PowerManager::getBatteryVoltage() {
  return 5.0f; // Return USB voltage as fallback
}

uint8_t PowerManager::getBatteryPercentage() {
  return 100; // Return 100% when USB powered
}

bool PowerManager::isLowBattery() {
  return false; // Never low when USB powered
}
#endif

void PowerManager::updatePowerState() {
  bool systemIdle = SystemState::isSystemIdle();
  bool cameraActive = SystemState::isCameraInUse();
  bool ntripIdle = SystemState::isNtripIdle();
  
  // Determine target CPU frequency based on activity
  uint32_t targetFreq;
  
  if (cameraActive || SystemState::isCapturing()) {
    // Maximum performance for camera operations
    targetFreq = Config::power.CPU_FREQ_CAPTURE;
  } else if (!systemIdle || !ntripIdle) {
    // Normal performance for active operations
    targetFreq = Config::power.CPU_FREQ_NORMAL;
  } else {
    // Low power when idle
    targetFreq = Config::power.CPU_FREQ_IDLE;
  }
  
  // Apply CPU frequency change
  setCpuFrequency(targetFreq);
  
  // Enter light sleep if completely idle
  if (systemIdle && ntripIdle && Config::power.SLEEP_BETWEEN_CAPTURES) {
    // Only sleep for short periods to maintain responsiveness
    enterLightSleep(100); // 100ms sleep
  }
}

bool PowerManager::isCpuScalingEnabled() {
  return initialized && Config::power.ENABLE_OPTIMIZATION;
}

bool PowerManager::isSystemPin(int pin) {
  // System pins that should not be reconfigured
  const int systemPins[] = {
    Config::pins.CAPTURE_TRIGGER_PIN,
    Config::pins.UPLOAD_TRIGGER_PIN,
    Config::pins.LED_STATUS_PIN,
    Config::pins.RTCM_UART_TX,
    // Add other system pins here
  };
  
  for (size_t i = 0; i < sizeof(systemPins) / sizeof(systemPins[0]); i++) {
    if (systemPins[i] == pin && systemPins[i] >= 0) {
      return true;
    }
  }
  
  // Check camera pins
  return CameraManager::isCameraPin(pin);
}

bool PowerManager::isPowerExemptPin(int pin) {
  // Pins that should not be configured for power management
  // These include: SD card pins, UART pins, etc.
  const int exemptPins[] = {
    1, 3,    // UART0 (programming/debug)
    19, 20,  // USB (if applicable)
    // SD card pins (varies by board)
    2, 4, 12, 13, 14, 15,
    // Add other exempt pins
  };
  
  for (size_t i = 0; i < sizeof(exemptPins) / sizeof(exemptPins[0]); i++) {
    if (exemptPins[i] == pin) {
      return true;
    }
  }
  
  return false;
}