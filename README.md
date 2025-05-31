# ESP32-S3-CAM Multi-Function System

A comprehensive ESP32-S3-CAM system that combines high-resolution photo capture, AWS S3 cloud upload, and NTRIP RTK corrections output via MAVLink.

## Features

### Photo Capture
- High-resolution JPEG capture (1600x1200 UXGA)
- Configurable capture intervals (default 500ms)
- Automatic directory organization with timestamps
- Power-efficient camera management
- Thread-safe SD card operations

### Cloud Upload
- AWS S3 integration with retry logic
- Batch upload of photo directories
- Upload tracking to prevent duplicates
- Automatic cleanup of uploaded content
- Configurable retention policies

### NTRIP RTK Client
- NTRIP v2.0 protocol support
- SSL/TLS secure connections
- RTCM message validation and filtering
- MAVLink output for autopilot integration
- Multi-constellation support (GPS, GLONASS, Galileo, BeiDou, QZSS)
- Real-time statistics and monitoring

### System Features
- Dual-core task management
- Comprehensive power optimization
- Thread-safe operations
- Health monitoring and logging
- Configuration file support
- Automatic error recovery

## Hardware Requirements

- ESP32-S3-CAM development board
- MicroSD card (minimum 4GB recommended)
- External buttons for triggers (optional)
- UART connection for RTCM output
- Stable 5V power supply

## Pin Configuration

| Function | GPIO Pin | Notes |
|----------|----------|-------|
| Capture Button | 12 | Pull-up enabled |
| Upload Button | 13 | Pull-up enabled |
| Status LED | 33 | 2 for ESP32-C3 |
| RTCM TX | 3 | UART2 output |
| Camera | Various | See camera_manager.h |
| SD Card | Default | 1-bit or 4-bit mode |

## Software Architecture

### File Structure
```
project/
├── src/
│   ├── main.cpp              # Main application
│   ├── config.cpp/h          # Configuration management
│   ├── camera_manager.cpp/h  # Camera control
│   ├── storage_manager.cpp/h # SD card operations
│   ├── network_manager.cpp/h # WiFi management
│   ├── upload_manager.cpp/h  # S3 upload handling
│   ├── ntrip_client.cpp/h    # NTRIP RTK client
│   ├── power_manager.cpp/h   # Power optimization
│   └── system_state.cpp/h    # State management
├── platformio.ini            # Build configuration
└── README.md                # This file
```

### Task Distribution

**Core 0:**
- Camera capture task
- S3 upload task
- Storage management

**Core 1:**
- NTRIP client task
- Network management
- Main loop

## Configuration

### WiFi Settings
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### AWS S3 Settings
```cpp
const char* aws_access_key = "YOUR_AWS_ACCESS_KEY";
const char* aws_secret_key = "YOUR_AWS_SECRET_KEY";
const char* aws_region = "us-east-1";
const char* s3_bucket = "your-bucket-name";
```

### NTRIP Settings
```cpp
const char* ntripServer = "ntrip.data.gnss.ga.gov.au";
const int ntripPort = 443;  // HTTPS
const char* ntripMountpoint = "SYDN00AUS0";
const char* ntripUsername = "your_username";
const char* ntripPassword = "your_password";
```

## Building and Uploading

### Using PlatformIO

1. Install PlatformIO IDE or CLI
2. Clone this repository
3. Update configuration in `config.h`
4. Build and upload:
```bash
pio run -t upload
```

### Using Arduino IDE

1. Install ESP32 board support
2. Install required libraries:
   - ESP32 Camera
   - ArduinoJson
   - Base64
   - MAVLink
3. Select "ESP32S3 Dev Module" board
4. Configure board settings:
   - PSRAM: OPI PSRAM
   - Flash Size: 8MB
   - Partition: Huge APP
5. Upload the sketch

## Usage

### Basic Operation

1. **Power On**: System initializes and connects to WiFi
2. **Start Capture**: Press capture button (GPIO 12)
3. **Stop Capture**: Press capture button again
4. **Upload Photos**: Press upload button (GPIO 13)

### LED Indicators

- **Solid ON**: Connected (WiFi + NTRIP)
- **Blinking**: Connection issues
- **OFF**: System idle or error

### Serial Monitor

Connect at 115200 baud to view:
- System status
- Capture progress
- Upload status
- NTRIP statistics
- Health reports

## Power Management

### Optimization Features
- Dynamic CPU frequency scaling
- Camera power down when idle
- Light sleep between captures
- Peripheral management
- Bluetooth disabled

### Power Consumption
- Active capture: ~500mA @ 5V
- Idle: ~100mA @ 5V
- Sleep: ~50mA @ 5V

## Storage Management

### Automatic Cleanup
- Removes uploaded directories after 7 days
- Maintains 1GB minimum free space
- Prioritizes oldest uploaded content

### Directory Structure
```
/capture_YYYYMMDD_HHMMSS/
├── photo_0000.jpg
├── photo_0001.jpg
├── photo_0002.jpg
└── ...
```

## NTRIP RTK Integration

### Supported RTCM Messages
- MSM4/5 for all constellations
- Reference station position (1005/1006)
- Satellite ephemeris data
- **L1-only** for basic receivers
- **L1/L2** for dual-frequency
- **L5** support for modern receivers

### MAVLink Output
- System ID: 128
- Component ID: 191
- GPS_RTCM_DATA messages
- Fragmentation for large messages
- 1Hz heartbeat

## Troubleshooting

### Common Issues

1. **Camera Init Failed**
   - Check camera ribbon cable
   - Verify power supply
   - Try different camera module

2. **SD Card Error**
   - Format as FAT32
   - Check card compatibility
   - Verify connections

3. **WiFi Connection Failed**
   - Check credentials
   - Verify network availability
   - Check signal strength

4. **Upload Failures**
   - Verify AWS credentials
   - Check S3 bucket permissions
   - Ensure internet connectivity

5. **No RTCM Data**
   - Verify NTRIP credentials
   - Check mountpoint availability
   - Confirm network path

### Debug Mode

Enable debug output in `platformio.ini`:
```ini
build_flags = 
    -DCORE_DEBUG_LEVEL=5
```

## Performance Metrics

- **Photo Capture**: 2 photos/second
- **Upload Speed**: ~1MB/s (network dependent)
- **RTCM Latency**: <100ms typical
- **SD Write Speed**: ~4MB/s
- **Memory Usage**: ~100KB heap

## Future Enhancements

- [ ] Web interface for configuration
- [ ] OTA firmware updates
- [ ] GPS geotagging
- [ ] Real-time streaming
- [ ] Image compression options
- [ ] Multi-camera support
- [ ] LoRa telemetry backup
- [ ] Solar power management

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License

This project is released under the MIT License. See LICENSE file for details.

## Acknowledgments

- ESP32 Arduino Core team
- MAVLink development team
- NTRIP protocol developers
- AWS SDK contributors

## Support

For issues and questions:
- Check existing issues on GitHub
- Read the troubleshooting guide
- Contact the maintainers

---

**Version**: 2.0.0  
**Last Updated**: 2024  
**Tested On**: ESP32-S3-CAM with ESP-IDF 5.0+
