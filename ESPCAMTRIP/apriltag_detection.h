#ifndef APRILTAG_DETECTION_H
#define APRILTAG_DETECTION_H

#include <Arduino.h>
#include "esp_camera.h"

#ifdef VISUAL_CODE_TEST_MODE

// AprilTag detection results
struct AprilTagDetection {
  bool found;                     // True if an AprilTag was detected
  uint16_t tag_id;                // Decoded tag ID
  float confidence;               // Detection confidence (0.0-1.0)

  // Corner positions (clockwise from top-left)
  struct {
    uint16_t x, y;
  } corners[4];

  // Center position
  uint16_t center_x, center_y;

  // Pose estimation (if enabled)
  struct {
    float x, y, z;              // Position (mm from camera)
    float roll, pitch, yaw;     // Orientation (radians)
    bool valid;                 // True if pose was computed
  } pose;

  unsigned long timestamp;        // millis() when detected
};

// AprilTag family types
enum AprilTagFamily {
  TAG_16h5,     // 16x16 bits, Hamming distance 5 (30 tags)
  TAG_25h9,     // 25x25 bits, Hamming distance 9 (35 tags)
  TAG_36h11     // 36x36 bits, Hamming distance 11 (587 tags)
};

// Detection parameters
struct AprilTagParams {
  AprilTagFamily family = TAG_36h11;       // Tag family to detect
  uint8_t quad_decimate = 2;               // Decimate input image by this factor
  uint8_t quad_sigma = 0;                  // Gaussian blur parameter (0=none)
  float quad_threshold_min = 10.0f;        // Minimum edge threshold
  float quad_threshold_max = 100.0f;       // Maximum edge threshold
  uint16_t min_tag_size = 30;              // Minimum tag size in pixels
  uint16_t max_tag_size = 300;             // Maximum tag size in pixels
  bool refine_edges = true;                // Refine quad edge locations
  bool decode_sharpening = 0.25f;          // Sharpening for tag decoding
  bool enable_pose_estimation = false;     // Calculate 6DOF pose
  float tag_size_mm = 50.0f;               // Physical tag size in millimeters
  bool enable_verbose_logging = true;      // Enable detailed logging
};

// Camera intrinsic parameters (for pose estimation)
struct CameraIntrinsics {
  float fx, fy;      // Focal lengths in pixels
  float cx, cy;      // Principal point in pixels
  float k1, k2;      // Radial distortion coefficients
  bool valid;        // True if calibrated
};

class AprilTagDetector {
public:
  // Initialization and configuration
  static bool init();
  static void setParams(const AprilTagParams& params);
  static AprilTagParams getParams();
  static void setCameraIntrinsics(const CameraIntrinsics& intrinsics);

  // Tag detection
  static std::vector<AprilTagDetection> detectTags(camera_fb_t* frameBuffer);
  static AprilTagDetection detectSingleTag(camera_fb_t* frameBuffer);
  static bool startContinuousDetection(uint32_t intervalMs = 100);
  static void stopContinuousDetection();

  // Pose estimation utilities
  static bool estimateTagPose(AprilTagDetection& detection, float tag_size_mm);
  static float calculateTagDistance(const AprilTagDetection& detection);
  static float calculateTagAngle(const AprilTagDetection& detection);

  // Image processing
  static void convertToGrayscale(camera_fb_t* fb, uint8_t* grayBuffer);
  static void detectQuadrilaterals(uint8_t* buffer, uint16_t width, uint16_t height);
  static bool decodeTag(uint8_t* buffer, uint16_t width, uint16_t height,
                       uint16_t corners[4][2], AprilTagDetection& result);

  // Debug and testing functions
  static void enableTestMode();
  static void disableTestMode();
  static void printDetectionInfo(const AprilTagDetection& detection);
  static void drawDetectionOverlay(camera_fb_t* fb, const AprilTagDetection& detection);
  static void saveDebugFrame(camera_fb_t* fb, const String& filename);

  // Statistics and monitoring
  static uint32_t getFramesProcessed();
  static uint32_t getTagsDetected();
  static uint32_t getValidDetections();
  static float getDetectionRate();
  static void resetStatistics();

private:
  // Internal state
  static bool initialized;
  static bool continuousMode;
  static bool testMode;
  static AprilTagParams params;
  static CameraIntrinsics camera_intrinsics;
  static unsigned long lastDetectionTime;
  static uint32_t detectionInterval;

  // Statistics
  static uint32_t framesProcessed;
  static uint32_t tagsDetected;
  static uint32_t validDetections;

  // Working buffers
  static uint8_t* grayBuffer;
  static uint8_t* edgeBuffer;
  static uint8_t* decimatedBuffer;
  static size_t bufferSize;

  // AprilTag detection pipeline
  static void preprocessImage(uint8_t* input, uint8_t* output, uint16_t width, uint16_t height);
  static void detectEdges(uint8_t* buffer, uint16_t width, uint16_t height);
  static std::vector<std::vector<uint16_t[2]>> findQuadrilaterals(uint8_t* buffer, uint16_t width, uint16_t height);
  static bool validateQuadrilateral(uint16_t corners[4][2], uint16_t width, uint16_t height);
  static bool decodeTagFamily(uint8_t* pattern, uint16_t size, AprilTagFamily family, uint16_t& tag_id);

  // AprilTag family decoders
  static bool decode16h5(uint8_t* pattern, uint16_t& tag_id);
  static bool decode25h9(uint8_t* pattern, uint16_t& tag_id);
  static bool decode36h11(uint8_t* pattern, uint16_t& tag_id);
  static uint16_t hammingDistance(uint64_t a, uint64_t b);

  // Pose estimation
  static bool solvePnP(uint16_t corners[4][2], float tag_size_mm,
                      float& x, float& y, float& z,
                      float& roll, float& pitch, float& yaw);

  // Memory management
  static bool allocateBuffers(size_t size);
  static void deallocateBuffers();

  // Continuous detection task
  static void continuousDetectionTask(void* parameter);
  static TaskHandle_t detectionTaskHandle;

  // Tag family lookup tables
  static const uint64_t TAG_16h5_CODES[];
  static const uint64_t TAG_25h9_CODES[];
  static const uint64_t TAG_36h11_CODES[];
  static const uint16_t TAG_16h5_COUNT;
  static const uint16_t TAG_25h9_COUNT;
  static const uint16_t TAG_36h11_COUNT;
};

// Test pattern generator for development
namespace AprilTagTestPatterns {
  // Generate test images with known tags
  void generateTag16h5(camera_fb_t* fb, uint16_t tag_id, uint16_t x, uint16_t y, uint16_t size);
  void generateTag25h9(camera_fb_t* fb, uint16_t tag_id, uint16_t x, uint16_t y, uint16_t size);
  void generateTag36h11(camera_fb_t* fb, uint16_t tag_id, uint16_t x, uint16_t y, uint16_t size);
  void addImageNoise(camera_fb_t* fb, uint8_t noiseLevel);
  void adjustImageLighting(camera_fb_t* fb, float brightness, float contrast);

  // Validation helpers
  bool validateDetectionAccuracy();
  void runDetectionBenchmark();
  void printTestResults();

  // Pose estimation validation
  bool validatePoseEstimation();
  void testPoseAtDistance(float distance_mm);
  void testPoseAtAngle(float angle_rad);
}

// Calibration utilities
namespace AprilTagCalibration {
  // Camera calibration using AprilTag grid
  bool calibrateCamera(std::vector<AprilTagDetection>& detections,
                      uint16_t grid_width, uint16_t grid_height,
                      float tag_size_mm, float tag_spacing_mm,
                      CameraIntrinsics& intrinsics);

  // Validate existing calibration
  bool validateCalibration(const CameraIntrinsics& intrinsics);
  float calculateReprojectionError(const std::vector<AprilTagDetection>& detections,
                                 const CameraIntrinsics& intrinsics);
}

#endif // VISUAL_CODE_TEST_MODE

#endif // APRILTAG_DETECTION_H