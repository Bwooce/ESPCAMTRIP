#ifndef VISUAL_CODE_RECOGNITION_H
#define VISUAL_CODE_RECOGNITION_H

#include <Arduino.h>
#include "esp_camera.h"

#ifdef VISUAL_CODE_TEST_MODE

// Visual code recognition results
struct CodeRecognitionResult {
  bool found;                     // True if a code was detected
  String type;                   // "QR", "Barcode", "DataMatrix", etc.
  String data;                   // Decoded text/data
  float confidence;              // Recognition confidence (0.0-1.0)
  uint16_t x, y;                // Center position of detected code
  uint16_t width, height;        // Bounding box dimensions
  unsigned long timestamp;       // millis() when detected
};

// Pattern detection parameters
struct DetectionParams {
  uint8_t minContrastThreshold = 30;    // Minimum contrast for edge detection
  uint8_t maxNoiseLevel = 15;           // Maximum noise tolerance
  uint16_t minCodeSize = 20;            // Minimum code size in pixels
  uint16_t maxCodeSize = 400;           // Maximum code size in pixels
  float aspectRatioTolerance = 0.3f;    // Aspect ratio tolerance for squares
  bool enableVerboseLogging = true;     // Enable detailed logging
};

class VisualCodeRecognition {
public:
  // Initialization and configuration
  static bool init();
  static void setDetectionParams(const DetectionParams& params);
  static DetectionParams getDetectionParams();

  // Code detection and recognition
  static CodeRecognitionResult scanForCodes(camera_fb_t* frameBuffer);
  static CodeRecognitionResult scanCurrentFrame();
  static bool startContinuousScanning(uint32_t intervalMs = 1000);
  static void stopContinuousScanning();

  // Pattern analysis functions
  static bool detectQRFinderPatterns(camera_fb_t* fb, uint16_t& x, uint16_t& y, uint16_t& size);
  static bool detectBarcodePattern(camera_fb_t* fb, uint16_t& x, uint16_t& y, uint16_t& width, uint16_t& height);
  static String extractQRData(camera_fb_t* fb, uint16_t x, uint16_t y, uint16_t size);

  // Image processing utilities
  static void convertToGrayscale(camera_fb_t* fb, uint8_t* grayBuffer);
  static void applyGaussianBlur(uint8_t* buffer, uint16_t width, uint16_t height);
  static void detectEdges(uint8_t* buffer, uint16_t width, uint16_t height, uint8_t threshold);
  static uint8_t getPixelIntensity(camera_fb_t* fb, uint16_t x, uint16_t y);

  // Debug and testing functions
  static void enableTestMode();
  static void disableTestMode();
  static void printFrameInfo(camera_fb_t* fb);
  static void generateTestPatternReport();
  static void saveDebugFrame(camera_fb_t* fb, const String& filename);

  // Statistics and monitoring
  static uint32_t getFramesProcessed();
  static uint32_t getCodesDetected();
  static uint32_t getSuccessfulScans();
  static float getDetectionRate();
  static void resetStatistics();

private:
  // Internal state
  static bool initialized;
  static bool continuousMode;
  static bool testMode;
  static DetectionParams params;
  static unsigned long lastScanTime;
  static uint32_t scanInterval;

  // Statistics
  static uint32_t framesProcessed;
  static uint32_t codesDetected;
  static uint32_t successfulScans;

  // Working buffers for image processing
  static uint8_t* grayBuffer;
  static uint8_t* edgeBuffer;
  static size_t bufferSize;

  // Pattern detection helpers
  static bool findSquarePattern(uint8_t* buffer, uint16_t width, uint16_t height,
                              uint16_t& x, uint16_t& y, uint16_t& size);
  static bool validateFinderPattern(uint8_t* buffer, uint16_t width, uint16_t height,
                                  uint16_t x, uint16_t y, uint16_t size);
  static int calculatePatternRatio(uint8_t* buffer, uint16_t width, uint16_t startX,
                                 uint16_t y, uint16_t length);

  // Data extraction helpers
  static String decodeBinaryPattern(uint8_t* buffer, uint16_t width, uint16_t height,
                                  uint16_t x, uint16_t y, uint16_t patternWidth, uint16_t patternHeight);
  static bool validateDataPattern(const String& pattern);

  // Memory management
  static bool allocateBuffers(size_t size);
  static void deallocateBuffers();

  // Continuous scanning task
  static void continuousScanTask(void* parameter);
  static TaskHandle_t scanTaskHandle;

  // Test patterns for validation
  static void generateSyntheticQR(uint8_t* buffer, uint16_t width, uint16_t height,
                                const String& data, uint16_t x, uint16_t y, uint16_t size);
  static void generateSyntheticBarcode(uint8_t* buffer, uint16_t width, uint16_t height,
                                     const String& data, uint16_t x, uint16_t y);
};

// Test pattern generator for development
namespace CodeTestPatterns {
  // Generate test images with known patterns
  void generateQRTestImage(camera_fb_t* fb, const String& testData);
  void generateBarcodeTestImage(camera_fb_t* fb, const String& testData);
  void addNoiseToImage(camera_fb_t* fb, uint8_t noiseLevel);
  void adjustImageContrast(camera_fb_t* fb, float contrastFactor);

  // Validation helpers
  bool validateDetectionAccuracy();
  void runDetectionBenchmark();
  void printTestResults();
}

#endif // VISUAL_CODE_TEST_MODE

#endif // VISUAL_CODE_RECOGNITION_H