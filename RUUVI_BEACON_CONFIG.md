# ATC_MiThermometer - BTHome & Ruuvi Configuration

## What This Repository Does

This repository provides custom firmware configuration and code paths for Ruuvi RAWv2 broadcasting on
ATC_MiThermometer-supported Telink hardware. In this workspace, Ruuvi mode is treated as a primary target,
with BTHome as the secondary option, plus device-specific compatibility fixes needed for dependable field use.

## Summary of Changes

### 1. Disabled Unused Beacon Types (RAM space savings)
**Files Modified**: `src/app_config.h`, `src/app.h`

- `USE_ATC_BEACON` = 0 (disabled)
- `USE_MIHOME_BEACON` = 0 (disabled)  
- `USE_BTHOME_BEACON` = 1 (enabled)
- `USE_CUSTOM_BEACON` = 1 (enabled - uses Ruuvi implementation)

### 2. Simplified Advertising Type Enum
**File**: `src/app.h`

New advertising type map:
```c
enum {
    ADV_TYPE_BTHOME = 0,
    ADV_TYPE_RUUVI = 1
} ADV_TYPE_ENUM;

#define ADV_TYPE_DEFAULT    ADV_TYPE_RUUVI  // Ruuvi is default
```

### 3. Updated BLE Advertising Logic
**File**: `src/ble.c` - `set_adv_data()` function

- Simplified to handle only BTHome (value 0) and Ruuvi (value 1)
- Ruuvi data beacon calls `ruuvi_data_beacon()` from custom_beacon.c

### 4. Ruuvi RAWv2 Beacon Implementation
**Files**: `src/custom_beacon.c`, `src/custom_beacon.h`

Ruuvi RAWv2 beacon format includes:
- Temperature (with 0.005°C resolution)
- Humidity (0.5% per unit)
- Battery level
- TX power
- MAC address
- Movement/measurement counter

## How to Switch Advertising Types

### Via WebUI Configuration Dropdown
The webUI dropdown will present two options:
- **0**: BTHome v2 (standard Bluetooth mesh home automation format)
- **1**: Ruuvi RAWv2 (Ruuvi Innovations beacon format - **DEFAULT**)

### Via Command Interface (if supported)
To change advertising type, send command to device characteristic:
```
Command: AA01  (Switch to Ruuvi - ADV_TYPE_RUUVI = 1)
Command: AA00  (Switch to BTHome - ADV_TYPE_BTHOME = 0)
```

### Default Configuration
- **Default advertising format**: Ruuvi RAWv2 (selected on startup)
- This matches the "Custom" flag behavior from webUI

## RAM Space Benefits
By disabling ATC and MI beacon implementations:
- **Freed flash**: ~2-3 KB
- **Freed RRAM**: ~500-800 bytes
- Result: More memory for display buffer, BLE features, or sensor support

## Build & Test
```bash
make clean
make
```

To enable Ruuvi beacon advertising after firmware flash:
1. Connect device via BLE in web interface
2. Select "Ruuvi RAWv2" from Advertising Type dropdown
3. Send configuration - device will immediately start broadcasting Ruuvi format

## Broadcasting Format Verification
### Ruuvi RAWv2 (Manufacturer Specific Data)
- Manufacturer ID: 0x0499 (Ruuvi Innovations)
- Flags: 0x05 (RAWv2)
- Contains: temp, humidity, battery, TX power, MAC

### BTHome v2 (GATT Service Data)
- Service UUID: 0x181A (Environmental Sensing)
- Format: BTHome v2 specification compliant
