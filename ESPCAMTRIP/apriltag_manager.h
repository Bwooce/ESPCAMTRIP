#ifndef APRILTAG_MANAGER_H
#define APRILTAG_MANAGER_H

#include <Arduino.h>
#include <esp_camera.h>

/**
 * AprilTag Detection Manager for ESP32-S3 Camera
 *
 * Handles AprilTag detection during LANDING mode for precision landing.
 * Optimized for real-time processing at VGA resolution with grayscale images.
 *
 * Features:
 * - Tag36h11 family detection (robust for outdoor use)
 * - Pose estimation with camera intrinsics
 * - Performance monitoring and statistics
 * - Integration with camera mode state machine
 */

// Forward declarations for AprilTag library types
// These will be included in the .cpp file to avoid header dependencies
struct apriltag_detector;
struct apriltag_family;
struct apriltag_detection;
struct image_u8;

struct AprilTagDetection {
    int id;                    // Tag ID
    float center_x, center_y;  // Tag center in image coordinates
    float corners[8];          // 4 corners (x,y pairs)

    // Pose estimation (relative to camera)
    bool pose_valid;
    float translation[3];      // X, Y, Z in meters
    float rotation[3][3];      // Rotation matrix

    // Quality metrics
    float decision_margin;     // Detection confidence
    float tag_size_pixels;     // Apparent tag size in image
};

struct AprilTagStatistics {
    uint32_t frames_processed;
    uint32_t total_detections;
    uint32_t unique_tags_seen;
    uint32_t pose_estimates;

    unsigned long avg_process_time_ms;
    unsigned long max_process_time_ms;
    unsigned long total_process_time;

    // Performance counters
    uint32_t detection_failures;
    uint32_t memory_errors;

    void reset() {
        frames_processed = 0;
        total_detections = 0;
        unique_tags_seen = 0;
        pose_estimates = 0;
        avg_process_time_ms = 0;
        max_process_time_ms = 0;
        total_process_time = 0;
        detection_failures = 0;
        memory_errors = 0;
    }
};

class AprilTagManager {
private:
    static bool initialized;
    static bool enabled;

    // AprilTag library components
    static apriltag_detector* detector;
    static apriltag_family* tag_family;

    // Detection configuration
    static float tag_size_m;           // Physical tag size in meters
    static float camera_fx, camera_fy; // Camera focal lengths
    static float camera_cx, camera_cy; // Camera optical center

    // Processing state
    static unsigned long last_detection_time;
    static AprilTagDetection last_detection;
    static bool has_active_detection;

    // Performance tracking
    static AprilTagStatistics stats;

    // Internal processing functions
    static bool initializeDetector();
    static void deinitializeDetector();
    static bool processFrameBuffer(uint8_t* frame_data, int width, int height);
    static bool estimatePose(apriltag_detection* detection, AprilTagDetection* result);
    static void updateStatistics(unsigned long process_time, int detection_count);

public:
    /**
     * Initialize AprilTag detection system
     * Configures detector with appropriate settings for landing mode
     */
    static bool init();

    /**
     * Deinitialize and cleanup AprilTag resources
     */
    static void deinit();

    /**
     * Check if AprilTag detection is enabled and initialized
     */
    static bool isInitialized() { return initialized; }

    /**
     * Enable/disable AprilTag processing
     */
    static void setEnabled(bool enable) { enabled = enable; }
    static bool isEnabled() { return enabled; }

    /**
     * Process camera frame for AprilTag detection
     * Frame should be VGA grayscale format as captured in LANDING mode
     *
     * @param frame_buffer ESP32 camera frame buffer
     * @return Number of tags detected in this frame
     */
    static int processFrame(camera_fb_t* frame_buffer);

    /**
     * Process raw frame data for AprilTag detection
     * Alternative interface for custom frame processing
     *
     * @param frame_data Grayscale image data
     * @param width Image width in pixels
     * @param height Image height in pixels
     * @return Number of tags detected
     */
    static int processFrameData(uint8_t* frame_data, int width, int height);

    /**
     * Get the most recent detection result
     * Valid only if hasActiveDetection() returns true
     */
    static AprilTagDetection getLastDetection() { return last_detection; }

    /**
     * Check if there's an active (recent) detection
     * Detections expire after a configurable timeout
     */
    static bool hasActiveDetection();

    /**
     * Get detection age in milliseconds since last detection
     */
    static unsigned long getDetectionAge();

    /**
     * Configuration functions
     */
    static void setTagSize(float size_meters) { tag_size_m = size_meters; }
    static void setCameraIntrinsics(float fx, float fy, float cx, float cy);

    /**
     * Get processing statistics
     */
    static AprilTagStatistics getStatistics() { return stats; }
    static void resetStatistics() { stats.reset(); }
    static void printStatistics();

    /**
     * Check if system is ready for pose estimation
     * Requires valid camera intrinsics and tag size
     */
    static bool isPoseEstimationReady();

    /**
     * Convert detection to MAVLink LANDING_TARGET coordinates
     * Returns target position relative to camera frame
     *
     * @param detection AprilTag detection result
     * @param angle_x Output: horizontal angle (radians)
     * @param angle_y Output: vertical angle (radians)
     * @param distance Output: distance to target (meters)
     * @return True if conversion successful
     */
    static bool convertToMAVLinkTarget(const AprilTagDetection& detection,
                                     float* angle_x, float* angle_y, float* distance);

    /**
     * Performance tuning functions
     */
    static void setProcessingParameters(float quad_decimate, float quad_sigma, bool refine_edges);
    static void optimizeForPerformance();
    static void optimizeForAccuracy();
};

#endif // APRILTAG_MANAGER_H