# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Development Commands

### Build and Flash
```bash
# Build the project for ESP32-C3
pio run -e esp32-c3-devkitm-1

# Upload/flash to the device
pio run -e esp32-c3-devkitm-1 -t upload

# Build and upload in one command
pio run -e esp32-c3-devkitm-1 -t upload
```

### Monitoring and Debugging
```bash
# Monitor serial output
pio device monitor -e esp32-c3-devkitm-1

# Upload and monitor
pio run -e esp32-c3-devkitm-1 -t upload -t monitor
```

### Code Formatting
```bash
# Format all C and header files using clang-format
make format
```

### Device Web UI (`ui/`)
```bash
make ui                                           # test + build -> data/public_html/index.html.gz
cd ui && DEVICE_URL=http://<device-ip> pnpm dev   # dev server, /api proxied to device
```
- Vanilla TS, no framework; `vite-plugin-singlefile` inlines JS/CSS into one `index.html`, vite plugin gzips it into `data/public_html/` (dir is wiped on build)
- `ui/src/protocol.ts` mirrors firmware binary request format (`app_defines.h`); keep in sync, also with `lightbar-web/lib/connections`

### File System
```bash
# Upload filesystem data (from ./data directory to LittleFS)
pio run -e esp32-c3-devkitm-1 -t uploadfs
```

## Architecture Overview

### Core Components

**Main Application (`src/main.c`)**
- Entry point with `app_main()` function
- Initializes event system for inter-module communication
- Sets up event handlers for chunked file processing
- Coordinates initialization of all subsystems

**Network Stack (`app_network.c`)**
- Manages dual WiFi modes (STA + AP)
- Handles WiFi connection/disconnection events
- Auto-reconnection logic with timeouts
- Credentials management through NVS

**HTTP Server (`app_server.c`)**
- Serves web interface from LittleFS storage
- Supports gzipped static files
- RESTful API endpoints for device configuration
- Chunked upload handling for large files
- Content-type detection based on file extensions

**LED Control (`app_lights.c`)**
- RMT-based LED strip control using custom encoder
- Color palette system with HSV to RGB conversion
- Timer-based animation loop
- Binary color format with hue and lightness encoding
- Support for dynamic LED count configuration

**Virtual File System (`app_vfs.c`)**
- LittleFS integration for persistent storage
- File operations with error handling
- Directory management utilities

**Non-Volatile Storage (`app_nvs.c`)**
- WiFi credentials persistence
- Device configuration storage
- Factory reset capabilities

### Data Flow Architecture

1. **Chunked File Processing Pipeline**:
   - Client uploads files in chunks via HTTP POST
   - Chunks are assembled in temporary storage (`/storage/temp/`)
   - Complete files trigger processing events
   - WiFi credentials and light frame data extracted
   - Processed data applied to respective subsystems

2. **Event-Driven Communication**:
   - `APP_EVENT_PROCESS_REQUEST_CHUNK` - Handle individual chunks
   - `APP_EVENT_PROCESS_REQUEST_CHUNKS_FILE` - Process complete files
   - `APP_EVENT_INIT_LIGHTS_SCHEMA` - Initialize LED patterns

### File System Layout
```
/storage/
├── public_html/     # Web interface files
├── temp/           # Temporary upload processing
└── lights/         # LED pattern data
```

## Hardware Configuration

**Target Platform**: ESP32-C3 (RISC-V based)
- Flash size: Configured via partitions
- LED strip: WS2812/SK6812 compatible via RMT peripheral
- GPIO pin for LED data: Defined in `app_lights.h`

## Build System

- **PlatformIO**: Primary build system with ESP-IDF framework
- **CMake**: ESP-IDF native build support
- **LittleFS**: File system with automatic image creation from `./data/`
- **Dependencies**: esp_littlefs library from GitHub

## Key Constants and Configurations

- Maximum LED count: `RMT_LED_NUMBERS` (defined in headers)
- WiFi credentials length: 32 chars SSID, 64 chars password
- File path maximum: 256 characters
- Context buffer: 10KB for request processing
- Connection timeouts: Configurable via ESP-IDF menuconfig