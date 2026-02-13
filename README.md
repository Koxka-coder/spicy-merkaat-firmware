# Spicy Merkaat Firmware

Código ESP-IDF para nodos ESP32/ESP32-C3: drivers de sensores, FSM de zonas, cliente MQTT, integración ESP-WIFI-MESH y OTA con rollback.

## Project Overview

This is an ESP-IDF v5.x C project for ESP32-class microcontrollers (currently built and validated for ESP32-C3). The firmware provides a complete IoT solution with the following features:

- **Zone FSM**: Finite State Machine for zone management
- **MQTT Client**: Communication with backend server
- **Mesh Bridge**: ESP-WIFI-MESH network integration
- **ADC Driver**: Sensor data acquisition
- **NVS Config**: Non-volatile storage for configuration
- **OTA Manager**: Over-the-air updates with rollback support

## Project Structure

```
spicy-merkaat-firmware/
├── CMakeLists.txt              # Root build configuration
├── sdkconfig.defaults          # ESP-IDF configuration defaults
├── partitions_ota.csv          # Partition table for OTA
├── main/                       # Main application
│   ├── CMakeLists.txt
│   └── main.c
└── components/                 # Custom components
    ├── zone_fsm/              # Zone state machine
    │   ├── CMakeLists.txt
    │   ├── zone_fsm.c
    │   └── include/
    │       └── zone_fsm.h
    ├── mqtt_client/           # MQTT client wrapper
    │   ├── CMakeLists.txt
    │   ├── mqtt_client.c
    │   └── include/
    │       └── mqtt_client.h
    ├── mesh_bridge/           # ESP-WIFI-MESH integration
    │   ├── CMakeLists.txt
    │   ├── mesh_bridge.c
    │   └── include/
    │       └── mesh_bridge.h
    ├── adc_driver/            # ADC sensor driver
    │   ├── CMakeLists.txt
    │   ├── adc_driver.c
    │   └── include/
    │       └── adc_driver.h
    ├── nvs_config/            # NVS configuration manager
    │   ├── CMakeLists.txt
    │   ├── nvs_config.c
    │   └── include/
    │       └── nvs_config.h
    └── ota_manager/           # OTA update manager
        ├── CMakeLists.txt
        ├── ota_manager.c
        └── include/
            └── ota_manager.h
```

## Prerequisites

- **ESP-IDF v5.x**: Install from [Espressif's official repository](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- **CMake**: Version 3.16 or higher
- **Python 3**: Required by ESP-IDF
- **ESP32-C3 toolchain (`riscv32-esp-elf`)**: Installed automatically with ESP-IDF

## Getting Started

### 1. Install ESP-IDF

Follow the official [ESP-IDF installation guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html).

### 2. Set up Environment

```bash
# Source the ESP-IDF environment
. $HOME/esp/esp-idf/export.sh
```

### 3. Configure the Project

```bash
# Set target to ESP32-C3
idf.py set-target esp32c3

# (Optional) Configure project settings
idf.py menuconfig
```

### 4. Build the Project

```bash
idf.py build
```

### 5. Flash to ESP32-C3

```bash
# Flash and monitor
idf.py -p COM3 flash monitor

# Or just flash
idf.py -p COM3 flash
```

Replace `COM3` with your board serial port if needed.

## Configuration

The project uses `sdkconfig.defaults` for default configuration. Key settings include:

- **Target**: ESP32-C3
- **Flash Size**: 4MB
- **WiFi**: Static and dynamic buffers configured
- **MQTT**: Protocol 3.1.1 enabled
- **OTA**: Rollback enabled for safe updates
- **Partition Table**: Custom OTA partition layout

## Partition Table

The project uses a custom partition table (`partitions_ota.csv`) with:

- **NVS**: 24KB for non-volatile storage
- **OTA Data**: 8KB for active OTA slot metadata
- **PHY Init**: 4KB for PHY calibration data
- **Factory**: 1MB for factory firmware
- **OTA_0**: 1MB for OTA partition 0
- **OTA_1**: 1MB for OTA partition 1
- **Storage (SPIFFS)**: 832KB for additional data storage
- **Coredump**: 64KB for crash dumps

## GPIO Mapping (Zone FSM)

Default valve GPIO mapping is target-dependent:

- **ESP32-C3**: Zone 0 = GPIO4, Zone 1 = GPIO5, Zone 2 = GPIO6
- **Other ESP32 targets**: Zone 0 = GPIO32, Zone 1 = GPIO33, Zone 2 = GPIO25

`zone_fsm` validates that each GPIO is valid as output for the selected target; invalid pins are disabled with an error log.

## Components

### Zone FSM
Manages zone states and transitions using a finite state machine.

### MQTT Client
Provides MQTT connectivity for cloud communication.

### Mesh Bridge
Enables ESP-WIFI-MESH networking for multi-node deployments.

### ADC Driver
Handles analog sensor readings through ESP32's ADC.

### NVS Config
Manages persistent configuration storage.

### OTA Manager
Handles firmware updates with automatic rollback on failure.

## Development

All components are currently stubs with TODO comments indicating where implementation is needed. Each component includes:

- Header file with proper header guards
- Source file with logging
- CMakeLists.txt for build integration
- Comprehensive documentation

## License

[Add your license information here]

## Contributing

[Add contribution guidelines here]
