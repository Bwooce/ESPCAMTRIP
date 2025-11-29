# ESPCAMTRIP

ESP32-S3-CAM system combining time-lapse photography, AWS S3 cloud upload, and NTRIP RTK corrections for high-precision GNSS positioning.

## Features

- **Photo Capture**: Configurable interval time-lapse photography with JPEG quality settings
- **AWS S3 Upload**: Cloud upload with retry logic and upload tracking
- **NTRIP Client**: RTK corrections via RTCM3 for centimetre-level GNSS accuracy
- **Dual Output Modes**: MAVLink (for flight controllers) or raw RTCM (for direct GPS receiver connection)
- **Power Management**: CPU frequency scaling and sleep modes for battery operation

## Hardware

**Target Board**: Seeed Studio XIAO ESP32S3 Sense

| Function | GPIO |
|----------|------|
| Capture Button | GPIO2 (D0) |
| Upload Button | GPIO4 (D2) |
| Status LED | GPIO21 (built-in) |
| RTCM UART TX | GPIO6 |
| RTCM UART RX | GPIO7 |

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
