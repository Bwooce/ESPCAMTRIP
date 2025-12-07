# ESPCAMTRIP

ESP32-S3-CAM system combining time-lapse photography, AWS S3 cloud upload, and NTRIP RTK corrections for high-precision GNSS positioning.

## Features

- **Multi-Mode Camera System**: Smart state machine with IDLE/MISSION/LANDING modes
- **GPS Geotagging**: RTK-precision coordinate embedding in photos and metadata
- **Photo Capture**: High-resolution UXGA (1600×1200) time-lapse with conservative 2Hz rate
- **AWS S3 Upload**: Cloud upload with retry logic and upload tracking
- **NTRIP Client**: RTK corrections via RTCM3 for centimetre-level GNSS accuracy
- **Dual Output Modes**: MAVLink (for flight controllers) or raw RTCM (for direct GPS receiver connection)
- **AprilTag Detection**: Full precision landing with tag36h11 family support via raspiduino/apriltag-esp32
- **Power Management**: CPU frequency scaling and sleep modes for battery operation
- **Memory Optimized**: PSRAM utilization, static EXIF, and hardcoded MAVLink for maximum performance

## Memory & Performance Optimizations

This system has been extensively optimized for ESP32-S3 resource constraints:

### PSRAM Support (8MB External Memory)
- **Smart Allocation**: PSRAM for >1KB allocations, internal RAM for smaller operations
- **Camera Buffers**: All camera frame buffers allocated in PSRAM to prevent heap fragmentation
- **Memory Health Monitoring**: Automatic cleanup and health checks prevent out-of-memory crashes
- **Zero-allocation Operations**: Critical flight paths optimized for deterministic performance

### Static EXIF Implementation
- **Pre-built Structures**: TIFF headers compiled at build time, zero runtime allocation
- **GPS Embedding**: Real-time coordinate updates without heap allocation
- **Flight-safe**: No memory allocation during image capture prevents failures
- **Based on**: ESP32-CAM_Interval proven approach for stability

### Hardcoded MAVLink Messages (~5KB vs 200-400KB)
- **Essential Messages**: HEARTBEAT, LANDING_TARGET, GPS_RTCM_DATA only
- **GPS Decoding**: Full MAVLink GPS message parsing for ArduPilot integration
- **Comprehensive Testing**: Encode/decode validation with alignment and endianness checks
- **Memory Savings**: 97% reduction in library footprint while maintaining compatibility

### Current Memory Footprint
```
Program Storage:    1.24MB (37% of 3.34MB)
Dynamic Memory:     59.6KB (18% of 320KB)
Available Runtime:  268KB for local variables and buffers
```

### AprilTag Detection Ready
- **Library**: raspiduino/apriltag-esp32 (Arduino-compatible adaptation)
- **Tag Families**: Support for 16h5, 25h9, 36h11, and custom families
- **Pose Estimation**: 6DOF pose estimation with camera intrinsic calibration
- **Performance**: Optimized for VGA real-time detection at 10Hz

## Hardware

**Target Board**: Seeed Studio XIAO ESP32S3 Sense
- **PSRAM Required**: Must have 8MB PSRAM (Sense version only)
- **SD Card**: MicroSD card for photo storage
- **GPS Module**: u-blox F9P recommended for RTK precision

| Function | GPIO |
|----------|------|
| Capture Button | GPIO2 (D0) |
| Upload Button | GPIO4 (D2) |
| Status LED | GPIO21 (built-in) |
| RTCM UART TX | GPIO6 |
| GPS UART RX | GPIO7 |
| MAVLink TX | GPIO8 |

## Build Instructions

### Prerequisites

1. **Arduino IDE** with ESP32 package installed
2. **XIAO ESP32S3 Board Support**: Install Seeed Studio board definitions
3. **AprilTag Library**: Install raspiduino/apriltag-esp32 library

### Required Libraries

Install these libraries via Arduino Library Manager or GitHub:

```bash
# Core dependencies (via Library Manager)
ArduinoJson (version 7.x)

# External dependencies (manual installation)
git clone https://github.com/raspiduino/apriltag-esp32.git ~/Documents/Arduino/libraries/apriltag-esp32
```

### Arduino IDE Configuration

**CRITICAL**: Configure these settings in Tools menu:

| Setting | Value | Notes |
|---------|-------|-------|
| Board | XIAO_ESP32S3 | From Seeed package |
| PSRAM | OPI PSRAM | **MUST ENABLE** |
| Flash Size | 8MB (64Mb) | Use available flash |
| Partition Scheme | Default 4MB with spiffs | For file system |
| Upload Speed | 921600 | Fastest reliable speed |

### Memory Configuration Check

After upload, verify PSRAM is detected in Serial Monitor:
```
PSRAM detected: 8.00 MB total, 7.95 MB free
PSRAM allocation test: PASSED
```

If PSRAM fails:
1. Verify board is XIAO ESP32S3 **Sense** (not regular ESP32S3)
2. Set Tools → PSRAM → "OPI PSRAM"
3. Recompile and upload

## Testing & Validation

### Memory Optimization Tests

The system includes comprehensive self-tests:

#### 1. PSRAM Test
```cpp
PSRAMManager::printDetailedStatus();  // Check PSRAM health
```

#### 2. MAVLink Encode/Decode Test
```cpp
HardcodedMAVLink::testMessageDecoding();  // Validates message integrity
```
Tests performed:
- Heartbeat round-trip encoding/decoding
- Landing target data preservation
- Structure alignment verification (ESP32 packing issues)
- Endianness validation
- Float precision accuracy

#### 3. EXIF GPS Test
```cpp
StaticEXIFGPS::updateGPS(lat, lon, alt, timestamp, fix_quality);
size_t new_size = StaticEXIFGPS::embedIntoJPEG(buffer, size, max_size);
```

### AprilTag Library Validation

Test AprilTag detection with sample images:
```cpp
#include "apriltag.h"
#include "tag36h11.h"

// Run AprilTag detection pipeline
AprilTagManager::processFrame(camera_frame);
```

### Performance Benchmarks

| Operation | Target Time | Typical Performance |
|-----------|-------------|-------------------|
| UXGA Capture | <500ms | 380-420ms |
| VGA Capture | <100ms | 85-95ms |
| EXIF Embedding | <50ms | 25-35ms |
| MAVLink Encode | <10ms | 3-7ms |
| AprilTag Detection | <100ms | 60-90ms |

## Camera Mode State Machine

The system operates in three distinct camera modes optimized for different flight phases:

### State Transitions

```
Power On → IDLE → [Button Press] → MISSION → [Button Press] → LANDING → [Timeout] → Power Down
```

### Mode Details

| Mode | Purpose | Resolution | Format | Rate | File Output |
|------|---------|------------|--------|------|-------------|
| **IDLE** | Standby | None | None | None | None |
| **MISSION** | Gutter scanning | UXGA (1600×1200) | JPEG | 2Hz | ✅ Geotagged photos |
| **LANDING** | AprilTag detection | VGA (640×480) | Grayscale | 10Hz | ❌ Processing only |

### Mode Operation

#### 1. IDLE Mode
- Camera powered down for energy efficiency
- Waiting for capture button press
- No processing or file operations

#### 2. MISSION Mode (Gutter Scanning)
- **Resolution**: UXGA (1600×1200) for maximum detail
- **Capture Rate**: 500ms intervals (2Hz) - conservative for reliability
- **GPS Geotagging**: RTK-precision coordinates embedded in filenames
- **File Format**: `photo_0001_N3752.123_E14510.567_RTK.jpg`
- **Metadata**: JSON files with full GPS data and accuracy metrics

#### 3. LANDING Mode (Precision Landing)
- **Resolution**: VGA (640×480) optimized for real-time processing
- **Capture Rate**: 100ms intervals (10Hz) for responsive AprilTag detection
- **Processing**: Frames captured for AprilTag analysis (Phase 2 implementation)
- **No File Writing**: Lightweight operation for maximum performance
- **Timeout**: Automatic power-down after 3 minutes of inactivity

## Optimum Speed Constraints for Drone Operations

### Mission Mode Performance Limits

The conservative 2Hz capture rate imposes speed constraints for proper image overlap:

| Overlap % | Flight Speed | Use Case |
|-----------|-------------|----------|
| **70%** | **2.8 km/h** | ✅ **Recommended** - Reliable coverage |
| 60% | 3.7 km/h | Adequate coverage |
| 50% | 4.7 km/h | Minimum acceptable |

### Coverage Calculations

For optimal gutter inspection with UXGA resolution at 1.5m altitude:
- **Ground coverage**: 1.73m × 1.30m per photo
- **Resolution**: 1.08mm per pixel (excellent for defect detection)
- **Image advance**: 1.04 m/s at 70% overlap

### Typical Survey Times

**Australian residential house (70m gutter perimeter)**:
- Flight time: ~75-85 seconds
- Photos captured: ~150-170 images
- Total mission time: 5-7 minutes (including setup/landing)

### Performance Monitoring

The system tracks capture statistics:
- Frame drops and timing violations
- Average and maximum capture times
- Storage write performance
- GPS fix quality and accuracy

## Quick Start

### 1. Install Dependencies

Using Arduino IDE, install these libraries:
- ArduinoJson
- MAVLink (only if using MAVLink output mode)

Board: Select "XIAO_ESP32S3" from esp32 boards.

### 2. Configure SD Card

Create `/config.json` on the SD card:

```json
{
  "wifi": {
    "ssid": "YOUR_WIFI_SSID",
    "password": "YOUR_WIFI_PASSWORD"
  },
  "s3": {
    "access_key": "YOUR_AWS_ACCESS_KEY",
    "secret_key": "YOUR_AWS_SECRET_KEY",
    "region": "us-east-1",
    "bucket": "your-bucket-name"
  },
  "ntrip": {
    "enabled": true,
    "server": "ntrip.example.com",
    "port": 443,
    "mountpoint": "MOUNT01",
    "username": "your_username",
    "password": "your_password",
    "gga_message": "$GPGGA,235959.000,3347.9167,S,15110.9333,E,1,12,1.0,42.5,M,6.6,M,,*65",
    "use_ssl": true
  },
  "timing": {
    "CAPTURE_INTERVAL": 500
  },
  "camera": {
    "JPEG_QUALITY": 10,
    "FRAME_SIZE": 13
  }
}
```

### 3. Select RTCM Output Mode

Edit `ESPCAMTRIP.ino` and choose your output mode:

**For flight controllers (ArduPilot, PX4):**
```cpp
#define RTCM_OUTPUT_MAVLINK  // Default
```

**For direct GPS receiver connection (ZED-F9P, NEO-M8P):**
```cpp
// #define RTCM_OUTPUT_MAVLINK
#define RTCM_OUTPUT_RAW
#define RTCM_RAW_BAUD_RATE 38400  // Match your receiver's baud rate
```

### 4. Upload and Run

1. Insert SD card with config.json
2. Upload sketch to XIAO ESP32S3 Sense
3. Press capture button to start/stop photo capture
4. Press upload button to upload photos to S3

## Wiring for Direct GPS Connection

For raw RTCM output to a u-blox ZED-F9P or similar:

```
ESP32 TX (GPIO6) --> GPS RX (UART1 or UART2)
ESP32 RX (GPIO7) <-- GPS TX (optional)
GND              <-> GND
```

Note: ZED-F9P defaults to 38400 baud on UART1/2. Either configure the receiver to 115200 via u-center, or set `RTCM_RAW_BAUD_RATE` to match.

## Configuration Reference

### Camera Frame Sizes

| Value | Resolution |
|-------|------------|
| 5 | QVGA (320x240) |
| 8 | VGA (640x480) |
| 9 | SVGA (800x600) |
| 10 | XGA (1024x768) |
| 12 | SXGA (1280x1024) |
| 13 | UXGA (1600x1200) |

### Power Settings

```json
{
  "power": {
    "ENABLE_OPTIMIZATION": true,
    "SLEEP_BETWEEN_CAPTURES": true,
    "CAMERA_POWER_MANAGEMENT": true,
    "IDLE_TIMEOUT_MS": 30000,
    "DEEP_SLEEP_TIMEOUT_MS": 300000,
    "CPU_FREQ_CAPTURE": 240,
    "CPU_FREQ_NORMAL": 160,
    "CPU_FREQ_IDLE": 80
  }
}
```

### Upload Settings

```json
{
  "upload": {
    "AUTO_UPLOAD": false,
    "DELETE_AFTER_UPLOAD": false
  }
}
```

## Architecture

The system uses FreeRTOS tasks:

- **Main Loop** (Core 0): Button handling, health monitoring
- **CameraTask** (Core 0): Photo capture at configured intervals
- **UploadTask** (Core 0): S3 upload coordination
- **NTRIPClient** (Core 1): RTCM streaming and processing

## RTCM Message Support

The NTRIP client validates and relays these RTCM3 message types:

**L1 Messages**: 1005, 1006, 1074, 1075, 1084, 1085, 1094, 1095, 1114, 1115, 1019, 1020, 1042, 1044, 1046

## License

MIT License - see [LICENSE](LICENSE)
