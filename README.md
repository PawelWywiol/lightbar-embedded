# lightbar-embedded

ESP32-based embedded firmware for controlling RGB LED lightbar with WiFi connectivity and web interface.

## Overview

This project provides firmware for ESP32 microcontrollers (specifically ESP32-C3) to control addressable RGB LED strips. It features WiFi connectivity, a web-based configuration interface, and dynamic LED pattern rendering with frame-based animation support.

## Features

- **WiFi Connectivity**: Dual-mode operation (AP and STA modes)
- **Web Interface**: Browser-based configuration and control
- **LED Control**: Support for addressable RGB LED strips
- **File System**: LittleFS-based storage for configurations and LED animation frames
- **RESTful API**: HTTP server with CORS support for external control
- **NVS Storage**: Non-volatile storage for WiFi credentials and settings
- **Dynamic LED Count**: Runtime detection and configuration of LED strip length

## Hardware Requirements

- ESP32-C3 DevKit M-1 (or compatible ESP32 board)
- Addressable RGB LED strip (WS2812B, SK6812, or similar)
- 5V power supply (appropriate for LED strip power requirements)

## Project Structure

```
lightbar-embedded/
├── src/
│   ├── main.c              # Main application entry point
│   ├── app_server.c        # HTTP server and API endpoints
│   ├── app_lights.c        # LED control and rendering
│   ├── app_network.c       # WiFi and network management
│   ├── app_nvs.c           # Non-volatile storage management
│   ├── app_vfs.c           # Virtual file system operations
│   └── app_utils.c         # Utility functions
├── include/
│   ├── app_defines.h       # Project-wide definitions
│   ├── app_events.h        # Event system definitions
│   └── *.h                 # Component headers
└── CMakeLists.txt          # Build configuration
```

## Building and Flashing

### Prerequisites

- ESP-IDF framework (version 4.4 or later recommended)
- PlatformIO (optional, if using PlatformIO build system)

## Configuration

The device can be configured via:

1. **Initial Setup**: Connect to the device's WiFi AP (default credentials in sdkconfig)
2. **Web Interface**: Navigate to the device's IP address in a browser
3. **WiFi Credentials**: Submit WiFi credentials through the web interface to connect to your network

## API Endpoints

The device exposes several HTTP endpoints for control:

- `GET /api/lightbar` - Get device status and configuration
- `POST /api/lightbar` - Update device configuration

## License

This project is licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE) for details.

## Development Status

This project is currently in active development.
