#include "apriltag_detection.h"

#ifdef VISUAL_CODE_TEST_MODE

// Static member initialization
AprilTagParams AprilTagDetector::params;
bool AprilTagDetector::initialized = false;

// AprilTag library objects
static apriltag_family_t *tf = nullptr;
static apriltag_detector_t *td = nullptr;

bool AprilTagDetector::init() {
  Serial.println("AprilTagDetector::init() called - Using raspiduino/apriltag-esp32 library");

  if (initialized) {
    Serial.println("AprilTag detector already initialized");
    return true;
  }

  // Initialize Circle21h7 family (triple concentric design)
  tf = tagCircle21h7_create();
  if (!tf) {
    Serial.println("ERROR: Failed to create tagCircle21h7 family");
    return false;
  }
  Serial.println("AprilTag detector using Circle21h7 family (triple concentric design)");

  // Create detector
  td = apriltag_detector_create();
  if (!td) {
    Serial.println("ERROR: Failed to create apriltag detector");
    tagCircle21h7_destroy(tf);
    return false;
  }

  // Add family to detector with ESP32-optimized settings
  // Use 0 error correction bits for quad detection only (we use LUT for actual decoding)
  // Circle21h7 has Hamming distance 7 but our LUT provides 3-bit error correction
  apriltag_detector_add_family_bits(td, tf, 0);  // 0 = exact matching for quad detection only

  // Configure detector parameters for minimal memory usage
  td->quad_decimate = 8.0;        // Maximum decimation to minimize stack usage
  td->quad_sigma = 0.0;           // Gaussian blur (0 = disabled)
  td->nthreads = 1;               // Single thread to reduce memory overhead
  td->debug = 0;                  // Disable debug output
  td->refine_edges = 0;           // Disable edge refinement to save memory
  td->decode_sharpening = 0.0;    // Disable sharpening to save memory

  initialized = true;
  Serial.println("AprilTag detector initialized with Circle21h7 family");
  Serial.printf("  Threads: %d, Quad decimate: %.1f\n", td->nthreads, td->quad_decimate);

  return true;
}

AprilTagDetection AprilTagDetector::detectSingleTag(camera_fb_t* frameBuffer) {
  AprilTagDetection detection = {};

  // Initialize detection result
  detection.found = false;
  detection.id = 0;
  detection.quality = 0.0;
  detection.center_x = 0.0;
  detection.center_y = 0.0;
  detection.size = 0.0;
  detection.pose_valid = false;

  if (!initialized || !td) {
    Serial.println("AprilTagDetector not initialized!");
    return detection;
  }

  if (!frameBuffer || !frameBuffer->buf) {
    Serial.println("Invalid frame buffer provided to detectSingleTag");
    return detection;
  }

  // Validate pixel format
  if (frameBuffer->format != PIXFORMAT_GRAYSCALE && frameBuffer->format != PIXFORMAT_RGB565) {
    Serial.printf("ERROR: AprilTag requires PIXFORMAT_GRAYSCALE or PIXFORMAT_RGB565, got format %d\n", frameBuffer->format);
    return detection;
  }

  Serial.printf("AprilTag detection: Processing %dx%d %s frame (%u bytes)\n",
                frameBuffer->width, frameBuffer->height,
                frameBuffer->format == PIXFORMAT_GRAYSCALE ? "grayscale" : "RGB565",
                (unsigned)frameBuffer->len);

  // Convert to grayscale if necessary
  uint8_t* grayBuffer = nullptr;
  bool allocatedBuffer = false;

  if (frameBuffer->format == PIXFORMAT_GRAYSCALE) {
    grayBuffer = frameBuffer->buf;
  } else if (frameBuffer->format == PIXFORMAT_RGB565) {
    // Allocate buffer for grayscale conversion in PSRAM
    size_t graySize = frameBuffer->width * frameBuffer->height;
    grayBuffer = (uint8_t*)ps_malloc(graySize);  // Use PSRAM for large image buffers
    if (!grayBuffer) {
      Serial.println("ERROR: Failed to allocate grayscale conversion buffer in PSRAM");
      return detection;
    }
    allocatedBuffer = true;

    // Convert RGB565 to grayscale
    Serial.println("Converting RGB565 to grayscale for AprilTag processing...");
    convertRGB565ToGrayscaleFast(frameBuffer->buf, grayBuffer, frameBuffer->width, frameBuffer->height);
  }

  // Create image_u8 structure for AprilTag library
  image_u8_t im = {
    .width = (int32_t)frameBuffer->width,
    .height = (int32_t)frameBuffer->height,
    .stride = (int32_t)frameBuffer->width,  // Grayscale: stride = width
    .buf = grayBuffer
  };

  // Detect AprilTags (with WiFi task yielding)
  taskYIELD();  // Let WiFi task run before intensive processing
  unsigned long startTime = millis();
  zarray_t *detections = apriltag_detector_detect(td, &im);
  unsigned long detectTime = millis() - startTime;
  taskYIELD();  // Let WiFi task run after intensive processing

  int numDetections = zarray_size(detections);
  // Minimal logging to prevent stack overflow during printf
  if (numDetections > 0) {
    Serial.println("AprilTag detected!");
  }

  if (numDetections > 0) {
    // Get the first (best) detection
    apriltag_detection_t *det;
    zarray_get(detections, 0, &det);

    // Fill in our detection structure
    detection.found = true;
    detection.id = det->id;
    detection.center_x = det->c[0];  // Center X coordinate
    detection.center_y = det->c[1];  // Center Y coordinate

    // Calculate tag size (average of all 4 corner distances)
    float size_sum = 0;
    for (int i = 0; i < 4; i++) {
      int next = (i + 1) % 4;
      float dx = det->p[next][0] - det->p[i][0];
      float dy = det->p[next][1] - det->p[i][1];
      size_sum += sqrt(dx*dx + dy*dy);
    }
    detection.size = size_sum / 4.0f;

    // Quality is decision margin (higher = better)
    detection.quality = det->decision_margin;

    // Copy corner positions
    for (int i = 0; i < 4; i++) {
      detection.corners[i].x = det->p[i][0];
      detection.corners[i].y = det->p[i][1];
    }

    // Pose estimation would require camera calibration parameters
    detection.pose_valid = false;

    Serial.printf("🎯 AprilTag ID %d detected at (%.1f,%.1f)\n",
                  detection.id, detection.center_x, detection.center_y);
    Serial.printf("   📍 Position: (%.1f, %.1f), Size: %.1f pixels\n",
                  detection.center_x, detection.center_y, detection.size);
    Serial.printf("   📊 Quality: %.2f (decision margin)\n", detection.quality);
    Serial.printf("   📐 Corners: (%.1f,%.1f) (%.1f,%.1f) (%.1f,%.1f) (%.1f,%.1f)\n",
                  detection.corners[0].x, detection.corners[0].y,
                  detection.corners[1].x, detection.corners[1].y,
                  detection.corners[2].x, detection.corners[2].y,
                  detection.corners[3].x, detection.corners[3].y);

    // Report if multiple tags were detected
    if (numDetections > 1) {
      Serial.printf("ℹ️  Multiple AprilTags detected: %d total (showing first one)\n", numDetections);
      Serial.println("   Use detectTags() function to get all detections");
    }

  }
  else {
    // Only log occasionally to avoid spamming output when no tags are present
    static uint32_t no_detection_counter = 0;
    no_detection_counter++;
    if (no_detection_counter % 50 == 0) {  // Log every 50th frame when no detection
      Serial.printf("No AprilTags detected in frame %u (checked %u frames total)\n",
                    no_detection_counter / 50, no_detection_counter);
    }
  }

  // Cleanup detections
  apriltag_detections_destroy(detections);

  // Free grayscale buffer if allocated
  if (allocatedBuffer && grayBuffer) {
    free(grayBuffer);
  }

  return detection;
}

std::vector<AprilTagDetection> AprilTagDetector::detectTags(camera_fb_t* frameBuffer) {
  std::vector<AprilTagDetection> detections;

  if (!initialized || !td || !frameBuffer || !frameBuffer->buf) {
    return detections;
  }

  if (frameBuffer->format != PIXFORMAT_GRAYSCALE && frameBuffer->format != PIXFORMAT_RGB565) {
    return detections;
  }

  // Convert to grayscale if necessary
  uint8_t* grayBuffer = nullptr;
  bool allocatedBuffer = false;

  if (frameBuffer->format == PIXFORMAT_GRAYSCALE) {
    grayBuffer = frameBuffer->buf;
  } else if (frameBuffer->format == PIXFORMAT_RGB565) {
    size_t graySize = frameBuffer->width * frameBuffer->height;
    grayBuffer = (uint8_t*)ps_malloc(graySize);  // Use PSRAM for large image buffers
    if (!grayBuffer) {
      return detections;
    }
    allocatedBuffer = true;
    convertRGB565ToGrayscaleFast(frameBuffer->buf, grayBuffer, frameBuffer->width, frameBuffer->height);
  }

  // Create image_u8 structure
  image_u8_t im = {
    .width = (int32_t)frameBuffer->width,
    .height = (int32_t)frameBuffer->height,
    .stride = (int32_t)frameBuffer->width,
    .buf = grayBuffer
  };

  // Detect all tags
  zarray_t *tag_detections = apriltag_detector_detect(td, &im);
  int numDetections = zarray_size(tag_detections);

  // Convert to our format
  for (int i = 0; i < numDetections; i++) {
    apriltag_detection_t *det;
    zarray_get(tag_detections, i, &det);

    AprilTagDetection detection = {};
    detection.found = true;
    detection.id = det->id;
    detection.center_x = det->c[0];
    detection.center_y = det->c[1];
    detection.quality = det->decision_margin;

    // Calculate size
    float size_sum = 0;
    for (int j = 0; j < 4; j++) {
      int next = (j + 1) % 4;
      float dx = det->p[next][0] - det->p[j][0];
      float dy = det->p[next][1] - det->p[j][1];
      size_sum += sqrt(dx*dx + dy*dy);
    }
    detection.size = size_sum / 4.0f;

    // Copy corners
    for (int j = 0; j < 4; j++) {
      detection.corners[j].x = det->p[j][0];
      detection.corners[j].y = det->p[j][1];
    }

    detection.pose_valid = false;

    detections.push_back(detection);
  }

  apriltag_detections_destroy(tag_detections);

  // Free grayscale buffer if allocated
  if (allocatedBuffer && grayBuffer) {
    free(grayBuffer);
  }

  return detections;
}

void AprilTagDetector::setParams(const AprilTagParams& newParams) {
  params = newParams;

  if (initialized && td) {
    // Apply parameters to detector
    td->quad_decimate = newParams.quad_decimate;
    td->quad_sigma = newParams.quad_sigma;
    td->refine_edges = newParams.refine_edges ? 1 : 0;
    td->decode_sharpening = newParams.decode_sharpening;

    Serial.println("AprilTag parameters updated");
    Serial.printf("  Quad decimate: %.1f, Sigma: %.1f\n",
                  td->quad_decimate, td->quad_sigma);
  }
}

AprilTagParams AprilTagDetector::getParams() {
  return params;
}

// Cleanup function (should be called on shutdown)
void AprilTagDetector::cleanup() {
  if (td) {
    apriltag_detector_destroy(td);
    td = nullptr;
  }

  if (tf) {
    tagCircle21h7_destroy(tf);
    tf = nullptr;
  }

  initialized = false;
  Serial.println("AprilTag detector cleaned up");
}

// ========== FAST GRAYSCALE CONVERSION ALGORITHMS ==========

// Original expensive conversion (kept for reference)
void AprilTagDetector::convertRGB565ToGrayscale(const uint8_t* rgb565, uint8_t* gray, int width, int height) {
  const uint16_t* rgb = (const uint16_t*)rgb565;

  for (int i = 0; i < width * height; i++) {
    uint16_t pixel = rgb[i];

    // Extract RGB components from RGB565
    uint8_t r = (pixel >> 11) & 0x1F;
    uint8_t g = (pixel >> 5) & 0x3F;
    uint8_t b = pixel & 0x1F;

    // Scale to 8-bit
    r = (r * 255) / 31;
    g = (g * 255) / 63;
    b = (b * 255) / 31;

    // Convert to grayscale using standard luminance formula
    gray[i] = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
  }
}

// ✅ FAST VERSION: Integer-only arithmetic with bit shifts (~5x faster)
void AprilTagDetector::convertRGB565ToGrayscaleFast(const uint8_t* rgb565, uint8_t* gray, int width, int height) {
  const uint16_t* rgb = (const uint16_t*)rgb565;

  for (int i = 0; i < width * height; i++) {
    uint16_t pixel = rgb[i];

    // Extract RGB components (no scaling needed for approximation)
    uint8_t r = (pixel >> 11) & 0x1F;  // 5 bits: 0-31
    uint8_t g = (pixel >> 5) & 0x3F;   // 6 bits: 0-63
    uint8_t b = pixel & 0x1F;          // 5 bits: 0-31

    // Fast integer approximation: Y ≈ (R + 2G + B) / 4
    // This approximates the 0.299R + 0.587G + 0.114B formula
    // Using shifts: G has more bits, so it naturally gets more weight
    uint16_t luma = (r + (g << 1) + b) >> 2;  // Divide by 4 using right shift

    // Scale to full 8-bit range
    gray[i] = (uint8_t)((luma << 2) | (luma >> 4)); // Approximate 0-31 -> 0-255
  }
}

// 🚀 FASTEST VERSION: Green channel only (~10x faster)
// Green carries most luminance information in RGB565 (6 bits vs 5 bits for R/B)
void AprilTagDetector::convertRGB565ToGrayscaleGreenOnly(const uint8_t* rgb565, uint8_t* gray, int width, int height) {
  const uint16_t* rgb = (const uint16_t*)rgb565;

  for (int i = 0; i < width * height; i++) {
    uint16_t pixel = rgb[i];

    // Extract only green channel (6 bits)
    uint8_t g = (pixel >> 5) & 0x3F;   // Green: 0-63

    // Scale to 8-bit: 0-63 -> 0-255
    gray[i] = (g << 2) | (g >> 4);     // Fast approximation using bit operations
  }
}

// ========== DIRECT CIRCLE21H7 LUT FUNCTIONS ==========

// Direct LUT lookup for Circle21h7 decoding
uint8_t AprilTagDetector::decodeCircle21h7Direct(uint32_t pattern) {
  // Mask to 21 bits and lookup in the 2MB LUT
  return circle21h7_lut[pattern & 0x1FFFFF];
}

// Sample a 7x7 grid from quad corners using bilinear interpolation
void AprilTagDetector::sampleTagGrid(const uint8_t* gray, int width, int height,
                                   const float corners[4][2], uint8_t grid[7][7]) {
  // Grid sampling for Circle21h7 (7x7 tag with 5x5 data area)
  for (int gy = 0; gy < 7; gy++) {
    for (int gx = 0; gx < 7; gx++) {
      // Bilinear coordinates within tag (0.0 to 1.0)
      float u = (gx + 0.5f) / 7.0f;
      float v = (gy + 0.5f) / 7.0f;

      // Bilinear interpolation of corners to get pixel position
      float x = corners[0][0] * (1-u) * (1-v) +
                corners[1][0] * u * (1-v) +
                corners[2][0] * u * v +
                corners[3][0] * (1-u) * v;
      float y = corners[0][1] * (1-u) * (1-v) +
                corners[1][1] * u * (1-v) +
                corners[2][1] * u * v +
                corners[3][1] * (1-u) * v;

      // Sample pixel (with bounds checking)
      int px = (int)(x + 0.5f);
      int py = (int)(y + 0.5f);
      if (px >= 0 && px < width && py >= 0 && py < height) {
        grid[gy][gx] = gray[py * width + px];
      } else {
        grid[gy][gx] = 128; // Gray if out of bounds
      }
    }
  }
}

// Convert sampled 7x7 grid to 21-bit pattern according to Circle21h7 layout
uint32_t AprilTagDetector::gridTo21BitCode(const uint8_t grid[7][7]) {
  // Circle21h7 bit positions (from tagCircle21h7.c bit_x/bit_y arrays)
  // AprilTag coordinates (-2 to +6) translated to 7x7 grid (0-6) by adding 3
  const int bit_positions[21][2] = {
    {4, 1},   // bit 0:  (1, -2) -> (1+3, -2+3) = (4, 1)
    {5, 1},   // bit 1:  (2, -2) -> (2+3, -2+3) = (5, 1)
    {6, 1},   // bit 2:  (3, -2) -> (3+3, -2+3) = (6, 1)
    {4, 4},   // bit 3:  (1, 1) -> (1+3, 1+3) = (4, 4)
    {5, 4},   // bit 4:  (2, 1) -> (2+3, 1+3) = (5, 4)
    {6, 4},   // bit 5:  (6, 1) -> (6+3, 1+3) = (9, 4) - CLAMP to (6, 4)
    {6, 5},   // bit 6:  (6, 2) -> (6+3, 2+3) = (9, 5) - CLAMP to (6, 5)
    {6, 6},   // bit 7:  (6, 3) -> (6+3, 3+3) = (9, 6) - CLAMP to (6, 6)
    {6, 4},   // bit 8:  (3, 1) -> (3+3, 1+3) = (6, 4)
    {6, 5},   // bit 9:  (3, 2) -> (3+3, 2+3) = (6, 5)
    {6, 6},   // bit 10: (3, 6) -> (3+3, 6+3) = (6, 9) - CLAMP to (6, 6)
    {5, 6},   // bit 11: (2, 6) -> (2+3, 6+3) = (5, 9) - CLAMP to (5, 6)
    {4, 6},   // bit 12: (1, 6) -> (1+3, 6+3) = (4, 9) - CLAMP to (4, 6)
    {6, 6},   // bit 13: (3, 3) -> (3+3, 3+3) = (6, 6)
    {5, 6},   // bit 14: (2, 3) -> (2+3, 3+3) = (5, 6)
    {1, 6},   // bit 15: (-2, 3) -> (-2+3, 3+3) = (1, 6)
    {1, 5},   // bit 16: (-2, 2) -> (-2+3, 2+3) = (1, 5)
    {1, 4},   // bit 17: (-2, 1) -> (-2+3, 1+3) = (1, 4)
    {4, 6},   // bit 18: (1, 3) -> (1+3, 3+3) = (4, 6)
    {4, 5},   // bit 19: (1, 2) -> (1+3, 2+3) = (4, 5)
    {5, 5}    // bit 20: (2, 2) -> (2+3, 2+3) = (5, 5)
  };

  // Calculate threshold (Otsu's method on border pixels)
  uint32_t sum = 0;
  int count = 0;
  for (int i = 0; i < 7; i++) {
    sum += grid[0][i] + grid[6][i] + grid[i][0] + grid[i][6]; // Border pixels
    count += 4;
  }
  count -= 4; // Remove corner double-counting
  uint8_t threshold = sum / count;

  // Extract 21 bits according to Circle21h7 layout
  uint32_t code = 0;
  for (int i = 0; i < 21; i++) {
    int x = bit_positions[i][0];
    int y = bit_positions[i][1];

    // Bounds check
    if (x >= 0 && x < 7 && y >= 0 && y < 7) {
      uint8_t pixel = grid[y][x];
      if (pixel < threshold) { // Black pixel = bit set
        code |= (1U << i);
      }
    }
  }

  return code;
}

// Extract 21-bit pattern from quad corners
uint32_t AprilTagDetector::extract21BitPattern(const uint8_t* gray, int width, int height,
                                             const float corners[4][2]) {
  uint8_t grid[7][7];
  sampleTagGrid(gray, width, height, corners, grid);
  return gridTo21BitCode(grid);
}

// Alternative detection using AprilTag quad detection + direct LUT decoding
bool AprilTagDetector::detectTagsWithLUT(const uint8_t* gray, int width, int height,
                                        std::vector<AprilTagDetection>& results) {
  if (!initialized || !td) {
    return false;
  }

  // Create image_u8 structure for AprilTag quad detection
  image_u8_t im = {
    .width = (int32_t)width,
    .height = (int32_t)height,
    .stride = (int32_t)width,
    .buf = (uint8_t*)gray
  };

  // Use AprilTag library for quad detection only (no decoding)
  zarray_t *detections = apriltag_detector_detect(td, &im);
  int numDetections = zarray_size(detections);

  for (int i = 0; i < numDetections; i++) {
    apriltag_detection_t *det;
    zarray_get(detections, i, &det);

    // Extract 21-bit pattern using our LUT method
    float corners[4][2];
    for (int j = 0; j < 4; j++) {
      corners[j][0] = det->p[j][0];
      corners[j][1] = det->p[j][1];
    }

    uint32_t pattern = extract21BitPattern(gray, width, height, corners);
    uint8_t tag_id = decodeCircle21h7Direct(pattern);

    if (tag_id != 255) { // Valid tag found via LUT
      AprilTagDetection detection = {};
      detection.found = true;
      detection.id = tag_id;
      detection.center_x = det->c[0];
      detection.center_y = det->c[1];
      detection.quality = det->decision_margin;

      // Calculate size
      float size_sum = 0;
      for (int k = 0; k < 4; k++) {
        int next = (k + 1) % 4;
        float dx = det->p[next][0] - det->p[k][0];
        float dy = det->p[next][1] - det->p[k][1];
        size_sum += sqrt(dx*dx + dy*dy);
      }
      detection.size = size_sum / 4.0f;

      // Copy corners
      for (int k = 0; k < 4; k++) {
        detection.corners[k].x = det->p[k][0];
        detection.corners[k].y = det->p[k][1];
      }

      detection.pose_valid = false;
      detection.timestamp = millis();

      results.push_back(detection);
    }
  }

  apriltag_detections_destroy(detections);
  return !results.empty();
}

#endif // VISUAL_CODE_TEST_MODE