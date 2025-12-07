#include "camera_manager.h"
#include "config.h"
#include "system_state.h"
#include "storage_manager.h"
#include "gps_manager.h"
#include "exif_gps_static.h"
#include "psram_manager.h"
#include <esp_camera.h>
#include <ArduinoJson.h>

// Static member definitions
bool CameraManager::initialized = false;
bool CameraManager::capturing = false;
int CameraManager::photoCount = 0;
String CameraManager::currentDirectory = "";
bool CameraManager::geotaggingEnabled = false;

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

// GPS Geotagging Functions

bool CameraManager::enableGeotagging(bool enable) {
  geotaggingEnabled = enable && Config::gps.enable_geotagging;
  Serial.printf("GPS geotagging %s\n", geotaggingEnabled ? "enabled" : "disabled");
  return geotaggingEnabled;
}

bool CameraManager::isGeotaggingEnabled() {
  return geotaggingEnabled && GPSManager::hasValidFix();
}

bool CameraManager::captureGeotaggedPhoto() {
  if (!initialized || !capturing) {
    return false;
  }

  // Get camera frame
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return false;
  }

  // Generate filename (with GPS coordinates if available)
  String filename = isGeotaggingEnabled() ? generateGeotaggedFilename() : generateFilename();
  File file = StorageManager::openFile(filename, "w");

  if (!file) {
    Serial.println("Failed to create file: " + filename);
    esp_camera_fb_return(fb);
    return false;
  }

  // Embed EXIF GPS data if geotagging is enabled (static implementation)
  size_t finalDataSize = fb->len;
  uint8_t* finalData = fb->buf;
  uint8_t* exifBuffer = nullptr;

  if (isGeotaggingEnabled()) {
    // Update static EXIF header with current GPS data
    GPSPosition gpsPos = GPSManager::getPosition();
    StaticEXIFGPS::updateGPS(gpsPos.latitude, gpsPos.longitude, gpsPos.altitude,
                            gpsPos.valid ? time(nullptr) : 0, GPSManager::getFixQuality());

    // Allocate buffer for JPEG + EXIF (using PSRAM for large allocations)
    size_t bufferSize = fb->len + StaticEXIFGPS::getHeaderSize();
    exifBuffer = (uint8_t*)PSRAMManager::allocate(bufferSize);

    if (exifBuffer) {
      // Copy original JPEG data
      memcpy(exifBuffer, fb->buf, fb->len);

      // Embed static EXIF data
      size_t newSize = StaticEXIFGPS::embedIntoJPEG(exifBuffer, fb->len, bufferSize);
      if (newSize > 0) {
        finalData = exifBuffer;
        finalDataSize = newSize;
        Serial.printf("Static EXIF GPS embedded (%u bytes added)\n", newSize - fb->len);
      } else {
        Serial.println("Failed to embed static EXIF GPS, using original JPEG");
        PSRAMManager::deallocate(exifBuffer);
        exifBuffer = nullptr;
      }
    } else {
      Serial.println("Failed to allocate EXIF buffer from PSRAM");
    }
  }

  // Write final image data
  size_t written = file.write(finalData, finalDataSize);
  StorageManager::closeFile(file);

  // Clean up EXIF buffer if allocated
  if (exifBuffer) {
    PSRAMManager::deallocate(exifBuffer);
  }

  // Return frame buffer
  esp_camera_fb_return(fb);

  // Check write success
  if (written == finalDataSize) {
    photoCount++;
    SystemState::incrementPhotoCount();

    // Save GPS metadata if geotagging is enabled
    if (isGeotaggingEnabled()) {
      saveGPSMetadata(filename);
      Serial.printf("Geotagged photo %04d saved: %u bytes (GPS: %.6f, %.6f)\n",
                    photoCount - 1, (unsigned)fb->len,
                    GPSManager::getPosition().latitude,
                    GPSManager::getPosition().longitude);
    } else {
      Serial.printf("Photo %04d saved: %u bytes\n", photoCount - 1, (unsigned)fb->len);
    }
    return true;
  }

  return false;
}

String CameraManager::generateGeotaggedFilename() {
  char filename[150];

  if (isGeotaggingEnabled()) {
    GPSPosition gpsPos = GPSManager::getPosition();
    // Format: photo_0001_N3752.1234_E14510.5678_RTK.jpg
    // Converts decimal degrees to DDMM.MMMM format for filename
    double lat = abs(gpsPos.latitude);
    double lon = abs(gpsPos.longitude);
    int latDeg = (int)lat;
    int lonDeg = (int)lon;
    double latMin = (lat - latDeg) * 60.0;
    double lonMin = (lon - lonDeg) * 60.0;

    char latHem = (gpsPos.latitude >= 0) ? 'N' : 'S';
    char lonHem = (gpsPos.longitude >= 0) ? 'E' : 'W';

    const char* fixType = GPSManager::hasRTKFix() ? "RTK" : "GPS";

    sprintf(filename, "%s/photo_%04d_%c%02d%06.3f_%c%03d%06.3f_%s.jpg",
            currentDirectory.c_str(), photoCount,
            latHem, latDeg, latMin,
            lonHem, lonDeg, lonMin,
            fixType);
  } else {
    // Fallback to regular filename if GPS not available
    sprintf(filename, "%s/photo_%04d.jpg", currentDirectory.c_str(), photoCount);
  }

  return String(filename);
}

bool CameraManager::saveGPSMetadata(const String& photoFilename) {
  if (!isGeotaggingEnabled()) {
    return false;
  }

  // Create metadata filename by replacing .jpg with .json
  String metadataFilename = photoFilename;
  metadataFilename.replace(".jpg", ".json");

  // Get current GPS position
  GPSPosition gpsPos = GPSManager::getPosition();
  GPSStatistics gpsStats = GPSManager::getStatistics();

  // Create JSON document
  DynamicJsonDocument doc(1024);

  // Basic photo info
  doc["photo_filename"] = photoFilename.substring(photoFilename.lastIndexOf('/') + 1);
  doc["photo_number"] = photoCount;
  doc["capture_time"] = millis();
  doc["unix_timestamp"] = gpsPos.timestamp;

  // GPS position data
  JsonObject gps = doc.createNestedObject("gps");
  gps["latitude"] = gpsPos.latitude;
  gps["longitude"] = gpsPos.longitude;
  gps["altitude_msl"] = gpsPos.altitude;
  gps["horizontal_accuracy"] = gpsPos.accuracy;
  gps["speed_ms"] = gpsPos.speed;
  gps["course_degrees"] = gpsPos.course;

  // GPS quality indicators
  gps["fix_quality"] = gpsPos.fixQuality;
  gps["fix_quality_text"] = GPSManager::getFixQualityString(gpsPos.fixQuality);
  gps["satellites_used"] = gpsPos.satellites;
  gps["hdop"] = gpsPos.hdop;
  gps["vdop"] = gpsPos.vdop;
  gps["age_of_diff"] = gpsPos.ageOfDiff;
  gps["valid"] = gpsPos.valid;

  // GPS statistics
  JsonObject stats = doc.createNestedObject("gps_stats");
  stats["messages_received"] = gpsStats.messagesReceived;
  stats["messages_processed"] = gpsStats.messagesProcessed;
  stats["parse_errors"] = gpsStats.parseErrors;
  stats["checksum_errors"] = gpsStats.checksumErrors;
  stats["message_rate_hz"] = gpsStats.messageRate;

  // Camera settings
  JsonObject camera = doc.createNestedObject("camera");
  camera["frame_size"] = Config::camera.FRAME_SIZE;
  camera["jpeg_quality"] = Config::camera.JPEG_QUALITY;
  camera["pixel_format"] = "JPEG";

  // Write JSON to file
  File metaFile = StorageManager::openFile(metadataFilename, "w");
  if (!metaFile) {
    Serial.println("Failed to create metadata file: " + metadataFilename);
    return false;
  }

  serializeJsonPretty(doc, metaFile);
  StorageManager::closeFile(metaFile);

  return true;
}