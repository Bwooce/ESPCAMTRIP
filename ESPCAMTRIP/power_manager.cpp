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

// Battery monitoring pins (ESP32-S3 compatible)
#ifdef CONFIG_IDF_TARGET_ESP32S3
  #define BATTERY_ADC_PIN 4     // ESP32-S3: GPIO 4 (ADC1_CH3)
  #define BATTERY_ADC_CHANNEL ADC1_CHANNEL_3
#else
  #define BATTERY_ADC_PIN 34    // ESP32: GPIO 34 (ADC1_CH6)
  #define BATTERY_ADC_CHANNEL ADC1_CHANNEL_6
#endif
#define BATTERY_VOLTAGE_DIVIDER_RATIO 2.0  // Adjust based on your voltage divider

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
    esp_pm_config_t pm_config = {
      .max_freq_mhz = static_cast<int>(Config::power.CPU_FREQ_CAPTURE),
      .min_freq_mhz = static_cast<int>(Config::power.CPU_FREQ_IDLE),
      .light_sleep_enable = true
    };
    
    esp_err_t ret = esp_pm_configure(&pm_config);
    if (ret == ESP_OK) {
      Serial.println("Dynamic frequency scaling enabled");
    } else {
      Serial.printf("Failed to configure power management: %s\n", esp_err_to_name(ret));
    }
  }
  
  // Initialize battery monitoring if available
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten((adc1_channel_t)BATTERY_ADC_CHANNEL, ADC_ATTEN_DB_12);
  
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
  
  // Check battery status if monitoring is enabled
  if (isLowBattery()) {
    Serial.println("WARNING: Low battery detected!");
    // Could trigger emergency upload or shutdown
  }
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
  // Configure unused pins as inputs with pull-down to prevent floating
  gpio_config_t io_conf = {};
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  
  // Configure GPIO pins based on ESP32 variant
#ifdef CONFIG_IDF_TARGET_ESP32S3
  // ESP32-S3 has GPIO 0-48
  const int max_gpio = 49;
#else
  // ESP32 has GPIO 0-39
  const int max_gpio = 40;
#endif

  for (int pin = 0; pin < max_gpio; pin++) {
    if (!isSystemPin(pin) && !isPowerExemptPin(pin)) {
      io_conf.pin_bit_mask = (1ULL << pin);
      gpio_config(&io_conf);
    }
  }
  
  // Special handling for pins that should be pulled up
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  
  // Example: Configure strapping pins
  io_conf.pin_bit_mask = (1ULL << 0) | (1ULL << 45) | (1ULL << 46);
  gpio_config(&io_conf);
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