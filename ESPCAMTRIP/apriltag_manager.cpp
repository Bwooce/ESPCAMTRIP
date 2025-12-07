#include "apriltag_manager.h"
#include "config.h"
#include <esp_camera.h>

// AprilTag library enabled - included from main sketch
#ifndef APRILTAG_ENABLED
#define APRILTAG_ENABLED  // Enable AprilTag functionality
#endif

// Conditional compilation for AprilTag library
#ifdef APRILTAG_ENABLED
    // Include AprilTag headers when library is available
    extern "C" {
        #include "apriltag.h"
        #include "tag36h11.h"
        #include "apriltag_pose.h"
    }
#else
    // Stub types for compilation without AprilTag library
    typedef struct apriltag_detector apriltag_detector_t;
    typedef struct apriltag_family apriltag_family_t;
    typedef struct apriltag_detection apriltag_detection_t;
    typedef struct image_u8 image_u8_t;
#endif

// Static member definitions
bool AprilTagManager::initialized = false;
bool AprilTagManager::enabled = false;
apriltag_detector* AprilTagManager::detector = nullptr;
apriltag_family* AprilTagManager::tag_family = nullptr;
float AprilTagManager::tag_size_m = 0.165f; // Default: 16.5cm tag
float AprilTagManager::camera_fx = 460.0f;  // Default focal length for VGA
float AprilTagManager::camera_fy = 460.0f;
float AprilTagManager::camera_cx = 320.0f;  // VGA center
float AprilTagManager::camera_cy = 240.0f;
unsigned long AprilTagManager::last_detection_time = 0;
AprilTagDetection AprilTagManager::last_detection = {};
bool AprilTagManager::has_active_detection = false;
AprilTagStatistics AprilTagManager::stats = {};

bool AprilTagManager::init() {
    Serial.println("Initializing AprilTag Manager...");

#ifdef APRILTAG_ENABLED
    if (initialized) {
        Serial.println("AprilTag Manager already initialized");
        return true;
    }

    // Check if AprilTag detection is enabled in config
    if (!Config::apriltag.enabled) {
        Serial.println("AprilTag detection disabled in configuration");
        return false;
    }

    // Initialize AprilTag detector
    if (!initializeDetector()) {
        Serial.println("Failed to initialize AprilTag detector");
        return false;
    }

    // Reset statistics
    stats.reset();
    last_detection_time = 0;
    has_active_detection = false;

    initialized = true;
    enabled = true;

    Serial.println("AprilTag Manager initialized successfully");
    Serial.printf("Tag family: 36h11, Tag size: %.3f m\n", tag_size_m);
    Serial.printf("Camera intrinsics: fx=%.1f fy=%.1f cx=%.1f cy=%.1f\n",
                  camera_fx, camera_fy, camera_cx, camera_cy);

    return true;
#else
    Serial.println("AprilTag library not available - using stub implementation");
    Serial.println("To enable AprilTag detection:");
    Serial.println("1. Install raspiduino/apriltag-esp32 library");
    Serial.println("2. Define APRILTAG_ENABLED in apriltag_manager.cpp");
    Serial.println("3. Recompile project");

    initialized = false;
    enabled = false;
    return false;
#endif
}

void AprilTagManager::deinit() {
    if (initialized) {
#ifdef APRILTAG_ENABLED
        deinitializeDetector();
#endif
        initialized = false;
        enabled = false;
        has_active_detection = false;
        Serial.println("AprilTag Manager deinitialized");
    }
}

bool AprilTagManager::initializeDetector() {
#ifdef APRILTAG_ENABLED
    // Create detector
    detector = apriltag_detector_create();
    if (!detector) {
        Serial.println("Failed to create AprilTag detector");
        return false;
    }

    // Configure detector for optimal performance on ESP32
    // These settings balance detection accuracy with processing speed
    detector->quad_decimate = 2.0f;      // Reduce resolution for speed
    detector->quad_sigma = 0.0f;         // Minimal blur for clean images
    detector->refine_edges = 1;          // Improve accuracy
    detector->decode_sharpening = 0.25f; // Modest sharpening

    // Create tag family (36h11 is robust for outdoor use)
    tag_family = tag36h11_create();
    if (!tag_family) {
        Serial.println("Failed to create tag family");
        apriltag_detector_destroy(detector);
        detector = nullptr;
        return false;
    }

    // Add family to detector
    apriltag_detector_add_family(detector, tag_family);

    Serial.println("AprilTag detector configured for ESP32");
    Serial.printf("Decimation: %.1f, Sigma: %.1f, Refine edges: %s\n",
                  detector->quad_decimate, detector->quad_sigma,
                  detector->refine_edges ? "Yes" : "No");

    return true;
#else
    return false;
#endif
}

void AprilTagManager::deinitializeDetector() {
#ifdef APRILTAG_ENABLED
    if (detector) {
        apriltag_detector_destroy(detector);
        detector = nullptr;
    }

    if (tag_family) {
        tag36h11_destroy(tag_family);
        tag_family = nullptr;
    }
#endif
}

int AprilTagManager::processFrame(camera_fb_t* frame_buffer) {
    if (!initialized || !enabled || !frame_buffer) {
        return 0;
    }

#ifdef APRILTAG_ENABLED
    // Validate frame format
    if (frame_buffer->format != PIXFORMAT_GRAYSCALE) {
        Serial.println("AprilTag requires grayscale image format");
        stats.detection_failures++;
        return 0;
    }

    return processFrameData(frame_buffer->buf, frame_buffer->width, frame_buffer->height);
#else
    // Stub implementation for compilation
    Serial.println("AprilTag processing disabled - library not available");
    return 0;
#endif
}

int AprilTagManager::processFrameData(uint8_t* frame_data, int width, int height) {
    if (!initialized || !enabled || !frame_data) {
        return 0;
    }

#ifdef APRILTAG_ENABLED
    unsigned long start_time = millis();

    // Create image_u8 structure for AprilTag library
    image_u8_t image = {
        .width = width,
        .height = height,
        .stride = width,
        .buf = frame_data
    };

    // Detect tags
    zarray_t* detections = apriltag_detector_detect(detector, &image);
    int num_detections = zarray_size(detections);

    // Process detections
    if (num_detections > 0) {
        // Get the first (best) detection for now
        // In a more sophisticated implementation, you might choose based on size or confidence
        apriltag_detection_t* detection;
        zarray_get(detections, 0, &detection);

        // Convert to our internal format
        last_detection.id = detection->id;
        last_detection.center_x = detection->c[0];
        last_detection.center_y = detection->c[1];
        last_detection.decision_margin = detection->decision_margin;

        // Copy corner coordinates
        for (int i = 0; i < 8; i++) {
            last_detection.corners[i] = detection->p[i/2][i%2];
        }

        // Estimate pose if camera intrinsics are available
        last_detection.pose_valid = estimatePose(detection, &last_detection);

        // Update detection state
        last_detection_time = millis();
        has_active_detection = true;

        // Calculate tag size in pixels for quality assessment
        float corner_distance = sqrt(pow(detection->p[0][0] - detection->p[2][0], 2) +
                                   pow(detection->p[0][1] - detection->p[2][1], 2));
        last_detection.tag_size_pixels = corner_distance;

        Serial.printf("AprilTag detected: ID=%d, center=(%.1f,%.1f), margin=%.2f\n",
                      last_detection.id, last_detection.center_x, last_detection.center_y,
                      last_detection.decision_margin);

        if (last_detection.pose_valid) {
            Serial.printf("Pose: X=%.3f Y=%.3f Z=%.3f m\n",
                          last_detection.translation[0],
                          last_detection.translation[1],
                          last_detection.translation[2]);
        }
    } else {
        has_active_detection = false;
    }

    // Cleanup detections
    apriltag_detections_destroy(detections);

    // Update statistics
    unsigned long process_time = millis() - start_time;
    updateStatistics(process_time, num_detections);

    return num_detections;
#else
    // Stub implementation
    static int call_count = 0;
    if (++call_count % 100 == 0) {
        Serial.printf("AprilTag stub: processed %d frames (%dx%d)\n", call_count, width, height);
    }
    return 0;
#endif
}

bool AprilTagManager::estimatePose(apriltag_detection* detection, AprilTagDetection* result) {
#ifdef APRILTAG_ENABLED
    if (!isPoseEstimationReady() || !detection || !result) {
        return false;
    }

    // Setup camera info for pose estimation
    apriltag_detection_info_t info = {
        .det = detection,
        .tagsize = tag_size_m,
        .fx = camera_fx,
        .fy = camera_fy,
        .cx = camera_cx,
        .cy = camera_cy
    };

    // Estimate pose
    apriltag_pose_t pose;
    estimate_tag_pose(&info, &pose);

    // Convert to our format
    for (int i = 0; i < 3; i++) {
        result->translation[i] = pose.t->data[i];
        for (int j = 0; j < 3; j++) {
            result->rotation[i][j] = pose.R->data[i*3 + j];
        }
    }

    stats.pose_estimates++;
    return true;
#else
    return false;
#endif
}

bool AprilTagManager::hasActiveDetection() {
    if (!has_active_detection) {
        return false;
    }

    // Check if detection has expired (500ms timeout for landing applications)
    return (millis() - last_detection_time) < 500;
}

unsigned long AprilTagManager::getDetectionAge() {
    return millis() - last_detection_time;
}

void AprilTagManager::setCameraIntrinsics(float fx, float fy, float cx, float cy) {
    camera_fx = fx;
    camera_fy = fy;
    camera_cx = cx;
    camera_cy = cy;

    Serial.printf("Camera intrinsics updated: fx=%.1f fy=%.1f cx=%.1f cy=%.1f\n",
                  fx, fy, cx, cy);
}

bool AprilTagManager::isPoseEstimationReady() {
    return (tag_size_m > 0.0f && camera_fx > 0.0f && camera_fy > 0.0f);
}

bool AprilTagManager::convertToMAVLinkTarget(const AprilTagDetection& detection,
                                           float* angle_x, float* angle_y, float* distance) {
    if (!angle_x || !angle_y || !distance || !detection.pose_valid) {
        return false;
    }

    // Convert image coordinates to angles
    // Horizontal angle (X-axis, positive to the right)
    *angle_x = atan2(detection.center_x - camera_cx, camera_fx);

    // Vertical angle (Y-axis, positive downward in image, but MAVLink expects positive up)
    *angle_y = atan2(camera_cy - detection.center_y, camera_fy);

    // Distance from pose estimation
    *distance = detection.translation[2]; // Z component is distance to camera

    return true;
}

void AprilTagManager::updateStatistics(unsigned long process_time, int detection_count) {
    stats.frames_processed++;
    stats.total_detections += detection_count;
    stats.total_process_time += process_time;

    if (process_time > stats.max_process_time_ms) {
        stats.max_process_time_ms = process_time;
    }

    if (stats.frames_processed > 0) {
        stats.avg_process_time_ms = stats.total_process_time / stats.frames_processed;
    }
}

void AprilTagManager::printStatistics() {
    Serial.println("\n--- AprilTag Statistics ---");
    Serial.printf("Status: %s\n", initialized ? (enabled ? "Enabled" : "Disabled") : "Not initialized");
    Serial.printf("Frames processed: %u\n", stats.frames_processed);
    Serial.printf("Total detections: %u\n", stats.total_detections);
    Serial.printf("Detection rate: %.1f%%\n",
                  stats.frames_processed > 0 ?
                  (float)stats.total_detections / stats.frames_processed * 100.0f : 0.0f);
    Serial.printf("Pose estimates: %u\n", stats.pose_estimates);
    Serial.printf("Avg/Max process time: %lu/%lu ms\n",
                  stats.avg_process_time_ms, stats.max_process_time_ms);
    Serial.printf("Detection failures: %u\n", stats.detection_failures);

    if (has_active_detection) {
        Serial.printf("Last detection: ID=%d, age=%lu ms\n",
                      last_detection.id, getDetectionAge());
    } else {
        Serial.println("No active detection");
    }
    Serial.println("---------------------------\n");
}

void AprilTagManager::setProcessingParameters(float quad_decimate, float quad_sigma, bool refine_edges) {
#ifdef APRILTAG_ENABLED
    if (detector) {
        detector->quad_decimate = quad_decimate;
        detector->quad_sigma = quad_sigma;
        detector->refine_edges = refine_edges ? 1 : 0;

        Serial.printf("AprilTag parameters updated: decimate=%.1f, sigma=%.1f, refine=%s\n",
                      quad_decimate, quad_sigma, refine_edges ? "Yes" : "No");
    }
#endif
}

void AprilTagManager::optimizeForPerformance() {
    setProcessingParameters(3.0f, 0.0f, false);
    Serial.println("AprilTag optimized for performance");
}

void AprilTagManager::optimizeForAccuracy() {
    setProcessingParameters(1.0f, 0.8f, true);
    Serial.println("AprilTag optimized for accuracy");
}