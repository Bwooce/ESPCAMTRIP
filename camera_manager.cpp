#include "camera_manager.h"
#include "config.h"
#include "system_state.h"
#include "storage_manager.h"
#include <esp_camera.h>

// Static member definitions
bool CameraManager::initialized = false;
bool CameraManager::capturing = false;
int CameraManager::photoCount = 0;
String CameraManager::currentDirectory = "";

bool CameraManager::init() {
  Serial.println("Initializing camera...");
  
  camera_config_t config = getCameraConfig();
  
  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }
  
  // Apply additional settings
  applyCameraSettings();
  
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
    Serial.printf("Photo %04d saved: %d bytes\n", photoCount - 1, fb->len);
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
  config.pin_sscb_sda = Config::cameraPins.SIOD_GPIO_NUM;
  config.pin_sscb_scl = Config::cameraPins.SIOC_GPIO_NUM;
  config.pin_pwdn = Config::cameraPins.PWDN_GPIO_NUM;
  config.pin_reset = Config::cameraPins.RESET_GPIO_NUM;
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

bool CameraManager::createCaptureDirectory() {
  struct tm timeinfo;
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
    Config::cameraPins.XCLK_GPIO_NUM,
    Config::cameraPins.SIOD_GPIO_NUM,
    Config::cameraPins.SIOC_GPIO_NUM,
    Config::cameraPins.Y9_GPIO_NUM,
    Config::cameraPins.Y8_GPIO_NUM,
    Config::cameraPins.Y7_GPIO_NUM,
    Config::cameraPins.Y6_GPIO_NUM,
    Config::cameraPins.Y5_GPIO_NUM,
    Config::cameraPins.Y4_GPIO_NUM,
    Config::cameraPins.Y3_GPIO_NUM,
    Config::cameraPins.Y2_GPIO_NUM,
    Config::cameraPins.VSYNC_GPIO_NUM,
    Config::cameraPins.HREF_GPIO_NUM,
    Config::cameraPins.PCLK_GPIO_NUM
  };
  
  for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
    if (pins[i] == pin && pins[i] >= 0) {
      return true;
    }
  }
  
  return false;
}