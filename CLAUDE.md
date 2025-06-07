# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESP32-S3-CAM based system that combines:
- **Photo capture** with configurable intervals and quality settings
- **AWS S3 cloud upload** with tracking and retry logic
- **NTRIP RTK corrections** for high-precision GNSS positioning
- **Power management** with CPU scaling and sleep modes

Target hardware: **Seeed Studio XIAO ESP32S3 Sense** with camera module.

## Build and Development

This is an Arduino IDE/PlatformIO project for ESP32-S3. No specific build commands are configured - use standard Arduino IDE or PlatformIO workflows.

**Key files to modify:**
- Main application: `ESPCAMTRIP/ESPCAMTRIP.ino`
- Configuration: `ESPCAMTRIP/config.h` and `ESPCAMTRIP/config.cpp`
- Manager classes: `ESPCAMTRIP/*_manager.{h,cpp}`

## Architecture

The system uses a **modular manager-based architecture** with FreeRTOS tasks:

### Core Managers
- **CameraManager**: Hardware initialization, photo capture, power control
- **StorageManager**: Thread-safe SD card operations with semaphore protection
- **WiFiManager**: Network connectivity and NTP time synchronization  
- **UploadManager**: AWS S3 uploads with state tracking and retry logic
- **NtripClient**: RTCM correction data processing with validation
- **PowerManager**: CPU frequency scaling and sleep mode coordination
- **SystemState**: Global state coordination between tasks

### Task Architecture
- **Main loop**: Button handling and health monitoring
- **CameraTask** (Core 0): Photo capture with configurable intervals
- **UploadTask** (Core 0): S3 upload coordination with capture pause/resume
- **NTRIPClient** (Core 1): RTCM data streaming and processing

### Configuration System
All settings are centralized in `Config` namespace with runtime loading from SD card JSON file. Key config sections:
- `Config::wifi` - Network credentials and timeouts
- `Config::s3` - AWS credentials and upload settings  
- `Config::ntrip` - NTRIP server details and RTCM parameters
- `Config::pins` - GPIO mappings for XIAO ESP32S3 Sense
- `Config::camera` - Image quality and resolution settings
- `Config::power` - Power optimization features

## Pin Configuration

Configured for **Seeed Studio XIAO ESP32S3 Sense**:
- Capture button: GPIO2 (D0)
- Upload button: GPIO4 (D2) 
- Status LED: GPIO21 (built-in)
- RTCM UART: TX=GPIO6, RX=GPIO7
- Camera pins: Standard XIAO camera connector mapping

## Thread Safety

**StorageManager** implements FreeRTOS semaphore-based locking for all SD card operations. Other managers coordinate through **SystemState** for camera usage and power management.

## Memory Management

Uses custom partition table (`partitions.csv`) with SPIFFS for additional storage alongside SD card. System includes memory monitoring and cleanup routines.