# ESPCAMTRIP - ESP32 Camera RTK Positioning System

ESP32-based camera system with RTK positioning and AprilTag detection capabilities.

## Features

- **RTK Positioning**: Centimeter-level GPS accuracy via NTRIP correction data
- **Camera Capture**: High-quality photo capture with ESP32-CAM/XIAO ESP32S3 Sense
- **AprilTag Detection**: Real-time visual fiducial marker detection
- **WiFi Connectivity**: Automatic NTRIP station discovery and data upload
- **SD Card Storage**: Local photo storage and configuration management

## Hardware Support

- ESP32-S3 (XIAO ESP32S3 Sense recommended)
- ESP32-CAM modules
- OV2640/OV3660/OV5640 camera modules
- GPS/GNSS modules (UART connection)
- SD card storage (SPI interface)

## Quick Start

### 1. Clone Repository with Submodules

```bash
# Clone with all submodules (recommended)
git clone --recursive https://github.com/your-username/ESPCAMTRIP.git

# OR clone first, then initialize submodules
git clone https://github.com/your-username/ESPCAMTRIP.git
cd ESPCAMTRIP
git submodule update --init --recursive
```

### 2. Required Libraries

This project uses git submodules for external dependencies:

- **AprilTag Detection**: `libraries/apriltag-esp32` (automatically included via submodule)

### 3. Arduino IDE Setup

1. Install ESP32 board package in Arduino IDE
2. Select your ESP32 board (ESP32-S3 or ESP32-CAM)
3. Configure partition scheme with at least 3MB APP space
4. Open `ESPCAMTRIP.ino` and compile

### 4. Configuration

Create `config.json` on SD card root, or use hardcoded defaults:

```json
{
  "wifi": {
    "ssid": "YourWiFiNetwork",
    "password": "YourPassword"
  },
  "ntrip": {
    "host": "your-ntrip-host.com",
    "port": 2101,
    "username": "your-username",
    "password": "your-password",
    "mountpoint": "your-mountpoint"
  }
}
```

## Development

### Working with Submodules

```bash
# Update all submodules to latest versions
git submodule update --remote

# Update specific submodule
cd libraries/apriltag-esp32
git pull origin main
cd ../..
git add libraries/apriltag-esp32
git commit -m "Update AprilTag library"

# Check submodule status
git submodule status
```

### Build Modes

- **Production Mode**: Photo capture + RTK positioning + cloud upload
- **AprilTag Test Mode**: Real-time tag detection (enable `VISUAL_CODE_TEST_MODE`)
- **Debug Mode**: Verbose logging and diagnostics

## Memory Requirements

- **Flash**: ~2.5MB (ESP32-S3 recommended)
- **RAM**: ~500KB + PSRAM for image buffers
- **PSRAM**: 8MB recommended for camera operations

## Pin Configuration

### XIAO ESP32S3 Sense (Default)
- Camera: Built-in OV2640
- SD Card: Built-in slot
- GPS: UART pins (configurable)
- Status LED: GPIO 21

### Custom ESP32-CAM
Configure pins in `config.h` for your specific board.

## Troubleshooting

### Compilation Issues

1. **Missing AprilTag library**: Ensure submodules are initialized
   ```bash
   git submodule update --init --recursive
   ```

2. **Memory errors**: Select partition scheme with sufficient APP space

3. **Camera initialization fails**: Check pin configuration and hardware

### Runtime Issues

1. **WiFi connection fails**: Verify credentials in config
2. **SD card not detected**: Check SPI connections and power
3. **GPS no fix**: Ensure antenna visibility and UART configuration
4. **AprilTag not detected**: Verify camera is in GRAYSCALE mode for detection

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make changes with proper testing
4. Submit a pull request

## License

[Add your license here]

## Acknowledgments

- [AprilTag Library](https://github.com/raspiduino/apriltag-esp32) by raspiduino
- [NTRIP Atlas](https://github.com/your-username/ntrip-atlas) for global station discovery