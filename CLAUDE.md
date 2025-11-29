# CLAUDE.md

Development guidance for Claude Code when working with this repository.

## Project Overview

ESP32-S3-CAM system combining photo capture, AWS S3 upload, and NTRIP RTK corrections.
See [README.md](README.md) for user documentation.

**Target hardware**: Seeed Studio XIAO ESP32S3 Sense

## Build and Development

Arduino IDE/PlatformIO project for ESP32-S3. Use standard workflows.

**Key files:**
- `ESPCAMTRIP/ESPCAMTRIP.ino` - Main application and compile-time flags
- `ESPCAMTRIP/config.h` / `config.cpp` - Configuration system
- `ESPCAMTRIP/*_manager.{h,cpp}` - Manager classes

## Architecture

Modular manager-based architecture with FreeRTOS tasks:

### Managers
- **CameraManager**: Hardware init, capture, power control
- **StorageManager**: Thread-safe SD operations (semaphore protected)
- **WiFiManager**: Network and NTP
- **UploadManager**: S3 uploads with retry logic
- **NtripClient**: RTCM processing and validation
- **PowerManager**: CPU scaling and sleep modes
- **SystemState**: Global state coordination

### Tasks
- **Main loop**: Buttons and health monitoring
- **CameraTask** (Core 0): Photo capture
- **UploadTask** (Core 0): S3 uploads
- **NTRIPClient** (Core 1): RTCM streaming

## Pin Configuration

| Function | GPIO |
|----------|------|
| Capture Button | 2 (D0) |
| Upload Button | 4 (D2) |
| Status LED | 21 |
| RTCM TX | 6 |
| RTCM RX | 7 |

## RTCM Output Modes

Compile-time flags at top of `ESPCAMTRIP.ino`:

```cpp
#define RTCM_OUTPUT_MAVLINK  // For flight controllers (default)
// #define RTCM_OUTPUT_RAW   // For direct GPS receiver connection
#define RTCM_RAW_BAUD_RATE 115200
```

## Thread Safety

StorageManager uses FreeRTOS semaphores. Other managers coordinate through SystemState.

## Memory

Custom partition table with SPIFFS alongside SD card. Includes memory monitoring.
