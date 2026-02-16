# Fixes Applied for LYWSD03MMC Device Detection and LCD Issues

## Critical Bugs Fixed

### 1. **Makefile Typo - Device Type Not Being Set** (CRITICAL)

**Problem:** 
- Line 92 had typo: `POJECT_DEF` instead of `PROJECT_DEF`
- `PROJECT_DEF` variable was never defined, so `DEVICE_TYPE` was never passed to compiler
- Device was compiling with default `DEVICE_TYPE=DEVICE_TS0201` (value 17) instead of `DEVICE_LYWSD03MMC` (value 10)
- This caused device to show as "Unknown or DIY (49)" in web UI

**Fixes Applied:**
1. Fixed typo on line 92: `$(POJECT_DEF)` → `$(PROJECT_DEF)`
2. Added line 22: `PROJECT_DEF := -DDEVICE_TYPE=DEVICE_LYWSD03MMC`

**Result:**
- Device will now compile with correct `DEVICE_TYPE=DEVICE_LYWSD03MMC`
- Hardware version detection will work properly
- Web UI will show correct device type (LYWSD03MMC B1.4/B1.7/B1.9 etc.)
- LCD driver and sensor code will be properly compiled and initialized

## Configuration Status

### Device Services (LYWSD03MMC)
✅ **SERVICE_SCREEN** - LCD display enabled (line 482 in app_config.h)  
✅ **SERVICE_THS** - Temperature/Humidity sensor enabled (line 484)  
✅ **SERVICE_OTA** - OTA updates enabled (line 477)  
✅ **SERVICE_HISTORY** - Data logging enabled (line 481)  
✅ **SERVICE_KEY** - Connect button enabled (line 485)  

### Beacon Configuration
✅ **USE_RUUVI_BEACON = 1** - Ruuvi RAWv2 format enabled (line 2908 in app_config.h)  
✅ **USE_BTHOME_BEACON = 1** - BTHome v2 enabled (line 2905)  
✅ **USE_MIHOME_BEACON = 1** - MiHome beacon enabled (line 2903)  

### Default Advertising Type
ℹ️ **Default is ADV_TYPE_BTHOME** (BTHome v2 format)

To use Ruuvi advertising format, you need to:
1. Flash the firmware
2. Connect to device via TelinkMiFlasher or nRF Connect
3. Change advertising type to "Ruuvi" (value 4) in the configuration

**OR** change the default by editing `src/app.h` line 41:
```c
// Current:
#define ADV_TYPE_DEFAULT	ADV_TYPE_BTHOME

// Change to:
#define ADV_TYPE_DEFAULT	ADV_TYPE_RUUVI
```

## How Hardware Version Detection Works (LYWSD03MMC)

The device auto-detects hardware version at runtime based on I2C addresses:

| HW Version | LCD I2C Address | Sensor I2C Address | Sensor Type |
|------------|-----------------|--------------------|-----------  |
| B1.4       | 0x3C            | 0x70               | SHTC3       |
| B1.5       | UART            | 0x70               | SHTC3       |
| B1.6       | UART/SPI        | 0x44               | SHT4x       |
| B1.7       | 0x3C            | 0x44               | SHT4x       |
| B1.9       | 0x3E            | 0x44               | SHT4x       |
| B2.0       | 0x3C            | 0x44               | SHT4x       |

Detection happens in `src/app.c` function `set_hw_version()` (line 341).

## LCD Display Update Flow

1. **Sensor Reading** (`read_sensors()` in app.c line 557):
   - Reads temperature and humidity from I2C sensor
   - Converts to x01 format: `measured_data.temp_x01 = (measured_data.temp + 5) / 10`
   - Converts to x1 format: `measured_data.humi_x1 = (measured_data.humi + 50) / 100`

2. **LCD Update Trigger** (main loop line 1254):
   - Timer-based: every `min_step_time_update_lcd` periods (default 49 * 0.05s = 2.45s)
   - Sets `lcd_flg.update = 1`

3. **LCD Refresh** (line 1267-1269):
   - Calls `lcd()` to prepare display buffer
   - Calls `update_lcd()` to send buffer to LCD via I2C/UART/SPI

## Ruuvi Beacon Format (RAWv2 / Data Format 5)

The Ruuvi beacon implementation (`src/custom_beacon.c` line 157) transmits:
- **Manufacturer ID:** 0x0499 (Ruuvi Innovations)
- **Data Format:** 0x05 (RAWv2)
- **Temperature:** int16 big-endian, resolution 0.005°C
- **Humidity:** uint16 big-endian, resolution 0.0025%
- **Pressure:** 0xFFFF (not available on LYWSD03MMC)
- **Battery Voltage:** encoded in power_info field
- **TX Power:** 0 dBm (encoded in power_info field)
- **Sequence Number:** uint16 big-endian
- **MAC Address:** 6 bytes

## Expected Results After Recompiling

1. **Device Type Detection:**
   - Web UI will show "LYWSD03MMC B1.x" instead of "Unknown or DIY (49)"
   - Hardware version will be correctly auto-detected based on LCD/sensor I2C addresses

2. **LCD Display:**
   - Should show correct temperature (e.g., "23.7°C") instead of "0.0°C"
   - Should show correct humidity (e.g., "27%") instead of "0%"
   - Update every ~2.45 seconds (configurable)

3. **Ruuvi Advertising:**
   - Available in web UI dropdown menu as advertising type option
   - When selected, broadcasts Ruuvi RAWv2 format (0x99 0x04 0x05...)
   - Visible in nRF Connect and other BLE scanners
   - Compatible with Ruuvi Station app

## Build Instructions

### On Windows (Eclipse/Telink IDE):
```cmd
cd /path/to/ATC_MiThermometer_Ruuvi
make clean
make
```

### In Docker:
```bash
docker run --rm -v $(pwd):/project skaldo/telink-sdk:0.1 make -C /project clean
docker run --rm -v $(pwd):/project skaldo/telink-sdk:0.1 make -C /project
```

### Output:
- Binary: `ATC_Thermometer.bin` (~80 KB)
- Flash using TelinkMiFlasher.py or web flasher

## Verification Steps

1. **Flash firmware** using TelinkMiFlasher
2. **Connect via nRF Connect** or TelinkMiFlasher
3. **Check Device Info:**
   - Should show "LYWSD03MMC B1.x" (not "Unknown or DIY (49)")
   - Software version: 5.8
4. **Check LCD:**
   - Should display correct temperature and humidity
   - Should update every few seconds
5. **Check Advertising:**
   - In config, set advertising type to 4 (Ruuvi)
   - Scan with nRF Connect - should see Ruuvi beacon (0x99 0x04 0x05...)

## Notes

- The fixes are **minimal and surgical** - only changed Makefile
- No changes to actual C code were needed
- All beacon formats (ATC, PVVX, MiHome, BTHome, Ruuvi) are compiled in
- User selects format via web UI or config write
- LCD and sensor code was already correct, just wasn't being compiled for correct device type
