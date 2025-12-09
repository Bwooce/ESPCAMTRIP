#include "camera_manager.h"
#include "config.h"
#include "system_state.h"
#include "storage_manager.h"
#include <esp_camera.h>

// Pin access macros to handle board profile differences
#if defined(ARDUINO_ESP32S3_CAM_LCD)
  // ESP32S3 CAM LCD board profile defines pins as macros
  #define GET_XCLK_PIN()      XCLK_GPIO_NUM
  #define GET_SIOD_PIN()      SIOD_GPIO_NUM
  #define GET_SIOC_PIN()      SIOC_GPIO_NUM
  #define GET_Y9_PIN()        Y9_GPIO_NUM
  #define GET_Y8_PIN()        Y8_GPIO_NUM
  #define GET_Y7_PIN()        Y7_GPIO_NUM
  #define GET_Y6_PIN()        Y6_GPIO_NUM
  #define GET_Y5_PIN()        Y5_GPIO_NUM
  #define GET_Y4_PIN()        Y4_GPIO_NUM
  #define GET_Y3_PIN()        Y3_GPIO_NUM
  #define GET_Y2_PIN()        Y2_GPIO_NUM
  #define GET_VSYNC_PIN()     VSYNC_GPIO_NUM
  #define GET_HREF_PIN()      HREF_GPIO_NUM
  #define GET_PCLK_PIN()      PCLK_GPIO_NUM
  #define GET_PWDN_PIN()      PWDN_GPIO_NUM
  #define GET_RESET_PIN()     RESET_GPIO_NUM
#else
  // Other board profiles use Config struct members
  #define GET_XCLK_PIN()      Config::cameraPins.XCLK_GPIO_NUM
  #define GET_SIOD_PIN()      Config::cameraPins.SIOD_GPIO_NUM
  #define GET_SIOC_PIN()      Config::cameraPins.SIOC_GPIO_NUM
  #define GET_Y9_PIN()        Config::cameraPins.Y9_GPIO_NUM
  #define GET_Y8_PIN()        Config::cameraPins.Y8_GPIO_NUM
  #define GET_Y7_PIN()        Config::cameraPins.Y7_GPIO_NUM
  #define GET_Y6_PIN()        Config::cameraPins.Y6_GPIO_NUM
  #define GET_Y5_PIN()        Config::cameraPins.Y5_GPIO_NUM
  #define GET_Y4_PIN()        Config::cameraPins.Y4_GPIO_NUM
  #define GET_Y3_PIN()        Config::cameraPins.Y3_GPIO_NUM
  #define GET_Y2_PIN()        Config::cameraPins.Y2_GPIO_NUM
  #define GET_VSYNC_PIN()     Config::cameraPins.VSYNC_GPIO_NUM
  #define GET_HREF_PIN()      Config::cameraPins.HREF_GPIO_NUM
  #define GET_PCLK_PIN()      Config::cameraPins.PCLK_GPIO_NUM
  #define GET_PWDN_PIN()      Config::cameraPins.PWDN_GPIO_NUM
  #define GET_RESET_PIN()     Config::cameraPins.RESET_GPIO_NUM
#endif

// Static member definitions
bool CameraManager::initialized = false;
bool CameraManager::capturing = false;
int CameraManager::photoCount = 0;
String CameraManager::currentDirectory = "";

bool CameraManager::init() {
  Serial.println("Initializing camera...");

  // Validate camera pins before attempting initialization
  if (!validateCameraPins()) {
    Serial.println("ERROR: Camera pin validation failed!");
    return false;
  }

  // Note: Xiao ESP32S3 Sense supports OV2640 (older) and OV3660 (newer revisions)
  Serial.println("Configuring for Xiao ESP32S3 Sense camera...");

  camera_config_t config = getCameraConfig();

  // Print debug information
  Serial.printf("Camera config: XCLK=%d, SIOD=%d, SIOC=%d\n",
                config.pin_xclk, config.pin_sccb_sda, config.pin_sccb_scl);

  // Initialize camera - ESP32 camera library will auto-detect OV2640/OV3660
  Serial.println("Initializing camera (OV2640/OV3660 auto-detection)...");
  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK) {
    Serial.printf("Camera initialization failed with error 0x%x\n", err);
    Serial.println("Camera initialization failed - system will continue without camera");
    return false;
  }

  // Detect which camera sensor was found
  sensor_t * s = esp_camera_sensor_get();
  if (s != NULL) {
    Serial.printf("Camera initialized successfully - Sensor: %s\n",
                  s->id.PID == OV2640_PID ? "OV2640" :
                  s->id.PID == OV3660_PID ? "OV3660" : "Unknown");
  } else {
    Serial.println("Camera hardware initialized successfully");
  }

  // Apply additional settings with error handling
  if (!applyCameraSettingsSafe()) {
    Serial.println("WARNING: Camera settings configuration failed, using defaults");
  }

  initialized = true;
  Serial.println("Camera initialized successfully");
  return true;
}

void CameraManager::deinit() {
  if (initialized) {
    esp_camera_deinit();
    initialized = false;
    Serial.println("Camera deinitialized");
  }
}

bool CameraManager::isInitialized() {
  return initialized;
}

void CameraManager::powerUp() {
  if (!initialized) {
    init();
  }
}

void CameraManager::powerDown() {
  if (initialized) {
    deinit();
  }
}

bool CameraManager::startCapture() {
  if (!initialized) {
    if (!init()) {
      return false;
    }
  }
  
  // Check storage space
  if (!StorageManager::ensureMinimumSpace()) {
    Serial.println("ERROR: Insufficient storage space!");
    return false;
  }
  
  // Create capture directory
  if (!createCaptureDirectory()) {
    return false;
  }
  
  capturing = true;
  photoCount = 0;
  SystemState::setCapturing(true);
  SystemState::setCameraInUse(true);
  
  Serial.println("Capture started in: " + currentDirectory);
  return true;
}

void CameraManager::stopCapture() {
  capturing = false;
  SystemState::setCapturing(false);
  SystemState::setCameraInUse(false);
  
  Serial.printf("Capture stopped. Total photos: %d\n", photoCount);
}

bool CameraManager::capturePhoto() {
  if (!initialized || !capturing) {
    return false;
  }
  
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return false;
  }
  
  String filename = generateFilename();
  File file = StorageManager::openFile(filename, "w");
  
  if (!file) {
    Serial.println("Failed to create file: " + filename);
    esp_camera_fb_return(fb);
    return false;
  }
  
  size_t written = file.write(fb->buf, fb->len);
  StorageManager::closeFile(file);
  
  esp_camera_fb_return(fb);
  
  if (written == fb->len) {
    photoCount++;
    SystemState::incrementPhotoCount();
    Serial.printf("Photo %04d saved: %u bytes\n", photoCount - 1, (unsigned)fb->len);
    return true;
  }
  
  return false;
}

bool CameraManager::setQuality(int quality) {
  if (!initialized) return false;
  
  sensor_t * s = esp_camera_sensor_get();
  if (!s) return false;
  
  s->set_quality(s, quality);
  return true;
}

bool CameraManager::setFrameSize(framesize_t size) {
  if (!initialized) return false;
  
  sensor_t * s = esp_camera_sensor_get();
  if (!s) return false;
  
  s->set_framesize(s, size);
  return true;
}

bool CameraManager::setBrightness(int brightness) {
  if (!initialized) return false;
  
  sensor_t * s = esp_camera_sensor_get();
  if (!s) return false;
  
  s->set_brightness(s, brightness);
  return true;
}

bool CameraManager::setContrast(int contrast) {
  if (!initialized) return false;
  
  sensor_t * s = esp_camera_sensor_get();
  if (!s) return false;
  
  s->set_contrast(s, contrast);
  return true;
}

bool CameraManager::setSaturation(int saturation) {
  if (!initialized) return false;
  
  sensor_t * s = esp_camera_sensor_get();
  if (!s) return false;
  
  s->set_saturation(s, saturation);
  return true;
}

int CameraManager::getPhotoCount() {
  return photoCount;
}

String CameraManager::getCurrentDirectory() {
  return currentDirectory;
}

camera_config_t CameraManager::getCameraConfig() {
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  #if defined(ARDUINO_ESP32S3_CAM_LCD)
    // For ESP32-S3-CAM-LCD, use board-defined pins directly
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
  #else
    // For other boards, use our Config::cameraPins values
    config.pin_d0 = Config::cameraPins.Y2_GPIO_NUM;
    config.pin_d1 = Config::cameraPins.Y3_GPIO_NUM;
    config.pin_d2 = Config::cameraPins.Y4_GPIO_NUM;
    config.pin_d3 = Config::cameraPins.Y5_GPIO_NUM;
    config.pin_d4 = Config::cameraPins.Y6_GPIO_NUM;
    config.pin_d5 = Config::cameraPins.Y7_GPIO_NUM;
    config.pin_d6 = Config::cameraPins.Y8_GPIO_NUM;
    config.pin_d7 = Config::cameraPins.Y9_GPIO_NUM;
    config.pin_xclk = Config::cameraPins.XCLK_GPIO_NUM;
    config.pin_pclk = Config::cameraPins.PCLK_GPIO_NUM;
    config.pin_vsync = Config::cameraPins.VSYNC_GPIO_NUM;
    config.pin_href = Config::cameraPins.HREF_GPIO_NUM;
    config.pin_sccb_sda = Config::cameraPins.SIOD_GPIO_NUM;
    config.pin_sccb_scl = Config::cameraPins.SIOC_GPIO_NUM;
    config.pin_pwdn = Config::cameraPins.PWDN_GPIO_NUM;
    config.pin_reset = Config::cameraPins.RESET_GPIO_NUM;
  #endif

  config.xclk_freq_hz = Config::camera.XCLK_FREQ;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = Config::camera.FRAME_SIZE;
  config.jpeg_quality = Config::camera.JPEG_QUALITY;
  config.fb_count = 1;

  return config;
}

void CameraManager::applyCameraSettings() {
  sensor_t * s = esp_camera_sensor_get();
  if (!s) return;

  // Apply optimal settings
  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, 0);       // -2 to 2
  s->set_saturation(s, 0);     // -2 to 2
  s->set_special_effect(s, 0); // 0 - No Effect
  s->set_whitebal(s, 1);       // 0 = disable, 1 = enable
  s->set_awb_gain(s, 1);       // 0 = disable, 1 = enable
  s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto)
  s->set_exposure_ctrl(s, 1);  // 0 = disable, 1 = enable
  s->set_aec2(s, 0);           // 0 = disable, 1 = enable
  s->set_ae_level(s, 0);       // -2 to 2
  s->set_aec_value(s, 300);    // 0 to 1200
  s->set_gain_ctrl(s, 1);      // 0 = disable, 1 = enable
  s->set_agc_gain(s, 0);       // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
  s->set_bpc(s, 0);            // 0 = disable, 1 = enable
  s->set_wpc(s, 1);            // 0 = disable, 1 = enable
  s->set_raw_gma(s, 1);        // 0 = disable, 1 = enable
  s->set_lenc(s, 1);           // 0 = disable, 1 = enable
  s->set_hmirror(s, 0);        // 0 = disable, 1 = enable
  s->set_vflip(s, 0);          // 0 = disable, 1 = enable
  s->set_dcw(s, 1);            // 0 = disable, 1 = enable
  s->set_colorbar(s, 0);       // 0 = disable, 1 = enable
}

bool CameraManager::applyCameraSettingsSafe() {
  sensor_t * s = esp_camera_sensor_get();
  if (!s) {
    Serial.println("WARNING: Could not get camera sensor for settings");
    return false;
  }

  // Apply optimal settings with error checking
  Serial.println("Applying camera sensor settings...");
  try {
    s->set_brightness(s, 0);
    s->set_contrast(s, 0);
    s->set_saturation(s, 0);
    s->set_special_effect(s, 0);
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_wb_mode(s, 0);
    s->set_exposure_ctrl(s, 1);
    s->set_aec2(s, 0);
    s->set_ae_level(s, 0);
    s->set_aec_value(s, 300);
    s->set_gain_ctrl(s, 1);
    s->set_agc_gain(s, 0);
    s->set_gainceiling(s, (gainceiling_t)0);
    s->set_bpc(s, 0);
    s->set_wpc(s, 1);
    s->set_raw_gma(s, 1);
    s->set_lenc(s, 1);
    s->set_hmirror(s, 0);
    s->set_vflip(s, 0);
    s->set_dcw(s, 1);
    s->set_colorbar(s, 0);

    Serial.println("Camera settings applied successfully");
    return true;
  } catch (...) {
    Serial.println("ERROR: Exception caught during camera settings configuration");
    return false;
  }
}

bool CameraManager::validateCameraPins() {
  Serial.println("Validating camera pin configuration...");

  #if defined(ARDUINO_ESP32S3_CAM_LCD)
    // For ESP32-S3-CAM-LCD, use board-defined pins directly
    Serial.printf("Camera pins: XCLK=%d, SIOD=%d, SIOC=%d, PCLK=%d\n",
                  XCLK_GPIO_NUM, SIOD_GPIO_NUM, SIOC_GPIO_NUM, PCLK_GPIO_NUM);
    Serial.printf("Data pins: Y9=%d, Y8=%d, Y7=%d, Y6=%d, Y5=%d, Y4=%d, Y3=%d, Y2=%d\n",
                  Y9_GPIO_NUM, Y8_GPIO_NUM, Y7_GPIO_NUM, Y6_GPIO_NUM,
                  Y5_GPIO_NUM, Y4_GPIO_NUM, Y3_GPIO_NUM, Y2_GPIO_NUM);
    Serial.printf("Control pins: VSYNC=%d, HREF=%d, PWDN=%d, RESET=%d\n",
                  VSYNC_GPIO_NUM, HREF_GPIO_NUM, PWDN_GPIO_NUM, RESET_GPIO_NUM);

    // Basic pin validation for ESP32-S3-CAM-LCD
    if (XCLK_GPIO_NUM < 0 || XCLK_GPIO_NUM > 48) {
      Serial.println("ERROR: Invalid XCLK pin");
      return false;
    }
  #else
    // For other boards, use our Config::cameraPins values
    Serial.printf("Camera pins: XCLK=%d, SIOD=%d, SIOC=%d, PCLK=%d\n",
                  Config::cameraPins.XCLK_GPIO_NUM,
                  Config::cameraPins.SIOD_GPIO_NUM,
                  Config::cameraPins.SIOC_GPIO_NUM,
                  Config::cameraPins.PCLK_GPIO_NUM);
    Serial.printf("Data pins: Y9=%d, Y8=%d, Y7=%d, Y6=%d, Y5=%d, Y4=%d, Y3=%d, Y2=%d\n",
                  Config::cameraPins.Y9_GPIO_NUM,
                  Config::cameraPins.Y8_GPIO_NUM,
                  Config::cameraPins.Y7_GPIO_NUM,
                  Config::cameraPins.Y6_GPIO_NUM,
                  Config::cameraPins.Y5_GPIO_NUM,
                  Config::cameraPins.Y4_GPIO_NUM,
                  Config::cameraPins.Y3_GPIO_NUM,
                  Config::cameraPins.Y2_GPIO_NUM);
    Serial.printf("Control pins: VSYNC=%d, HREF=%d, PWDN=%d, RESET=%d\n",
                  Config::cameraPins.VSYNC_GPIO_NUM,
                  Config::cameraPins.HREF_GPIO_NUM,
                  Config::cameraPins.PWDN_GPIO_NUM,
                  Config::cameraPins.RESET_GPIO_NUM);

    // Basic pin validation
    if (Config::cameraPins.XCLK_GPIO_NUM < 0 || Config::cameraPins.XCLK_GPIO_NUM > 48) {
      Serial.println("ERROR: Invalid XCLK pin");
      return false;
    }

    if (Config::cameraPins.SIOD_GPIO_NUM < 0 || Config::cameraPins.SIOD_GPIO_NUM > 48) {
      Serial.println("ERROR: Invalid SIOD pin");
      return false;
    }

    if (Config::cameraPins.SIOC_GPIO_NUM < 0 || Config::cameraPins.SIOC_GPIO_NUM > 48) {
      Serial.println("ERROR: Invalid SIOC pin");
      return false;
    }
  #endif

  Serial.println("Camera pin validation passed");
  return true;
}

bool CameraManager::createCaptureDirectory() {
  struct tm timeinfo = {0}; // Initialized
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to get time for directory name");
    return false;
  }
  
  char dirName[50];
  sprintf(dirName, "/capture_%04d%02d%02d_%02d%02d%02d", 
          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  
  currentDirectory = String(dirName);
  SystemState::setCurrentDirectory(currentDirectory);
  
  if (!StorageManager::mkdir(currentDirectory)) {
    Serial.println("Failed to create directory: " + currentDirectory);
    return false;
  }
  
  return true;
}

String CameraManager::generateFilename() {
  char filename[100];
  sprintf(filename, "%s/photo_%04d.jpg", currentDirectory.c_str(), photoCount);
  return String(filename);
}

bool CameraManager::isCameraPin(int pin) {
  const int pins[] = {
    GET_XCLK_PIN(),
    GET_SIOD_PIN(),
    GET_SIOC_PIN(),
    GET_Y9_PIN(),
    GET_Y8_PIN(),
    GET_Y7_PIN(),
    GET_Y6_PIN(),
    GET_Y5_PIN(),
    GET_Y4_PIN(),
    GET_Y3_PIN(),
    GET_Y2_PIN(),
    GET_VSYNC_PIN(),
    GET_HREF_PIN(),
    GET_PCLK_PIN()
  };

  for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
    if (pins[i] == pin && pins[i] >= 0) {
      return true;
    }
  }

  return false;
}

