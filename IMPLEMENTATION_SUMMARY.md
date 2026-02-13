# Implementation Summary: Ruuvi RAWv2 BLE Advertisement Support

## Overview
Successfully implemented Ruuvi RAWv2 (Data Format 5) BLE advertisement format support for the ATC_MiThermometer firmware. This feature enables compatibility with Victron Cerbo GX BLE scanning for RuuviTags and other Ruuvi-compatible receivers.

**CRITICAL UPDATE:** Fixed endianness to match Ruuvi specification - all multi-byte fields are now correctly encoded as **BIG-ENDIAN (MSB first)** as required by the official Ruuvi RAWv2 protocol.

## Files Modified

### Core Implementation
1. **src/custom_beacon.h**
   - Added `ADV_RUUVI_COMPANY_ID` constant (0x0499)
   - Added byte-swap helper macros: `U16_TO_BIG_ENDIAN`, `S16_TO_BIG_ENDIAN`
   - Defined `adv_ruuvi_rawv2_t` structure (28 bytes)
   - **Changed multi-byte fields to byte arrays** for explicit big-endian control
   - Added function declaration for `ruuvi_data_beacon()`

2. **src/custom_beacon.c**
   - Implemented `ruuvi_data_beacon()` function
   - **Added explicit big-endian encoding** for all multi-byte fields
   - Temperature conversion with overflow protection (-163.835°C to +163.835°C)
   - Humidity conversion with overflow protection (0% to 100%)
   - Battery voltage encoding (1600-3646 mV range)
   - TX power encoding (0 dBm default, 2dBm steps)
   - Set appropriate sentinel values for unused fields

3. **src/app_config.h**
   - Added `USE_RUUVI_BEACON` flag (default: 1)
   - Updated beacon type sum checks to include Ruuvi

4. **src/app.h**
   - Added `ADV_TYPE_RUUVI` to advertising type enum (value: 4)
   - Expanded `advertising_type` bitfield from 2 to 3 bits
   - Added configuration compatibility note

5. **src/ble.c**
   - Updated `set_adv_data()` to handle Ruuvi beacon type
   - Added conditional compilation for Ruuvi support

6. **src/rds_count.c**
   - Updated `set_rds_adv_data()` to handle Ruuvi type
   - Ruuvi uses `default_event_beacon()` for RDS events

### Documentation & Testing
7. **RUUVI_RAWV2.md**
   - Comprehensive format specification
   - Conversion formulas and examples
   - Usage instructions
   - Compatibility notes

8. **test_ruuvi_format.py**
   - Validation script for packet structure
   - Tests for all field encodings
   - Example output with real values

## Technical Details

### Endianness - CRITICAL

**The Ruuvi RAWv2 specification requires all multi-byte fields to be BIG-ENDIAN (MSB first).** This is explicitly stated in the official Ruuvi documentation:

> "All fields are MSB first 2-complement, i.e. 0xFC18 is read as -1000 and 0x03E8 is read as 1000."

The implementation uses:
- Byte array fields instead of native u16/s16 types
- Explicit byte-swap macros: `U16_TO_BIG_ENDIAN()`, `S16_TO_BIG_ENDIAN()`
- Manual MSB-first encoding: `arr[0] = (val >> 8); arr[1] = val;`

**Exception:** Company ID (0x0499) uses little-endian per BLE specification for manufacturer ID field.

### Packet Structure
- Total size: 28 bytes
- Size field: 27 (total - 1)
- Manufacturer ID: 0x0499 (transmitted as 0x99 0x04 little-endian)
- Data format: 0x05 (RAWv2)
- All data fields: BIG-ENDIAN (temperature, humidity, pressure, acceleration, power_info, sequence)

### Example Packet (Big-Endian)
```
1BFF990405125C5654FFFF000000000000961400007B112233445566
```

Compare with incorrect little-endian version:
- Temperature: `12 5C` (big-endian, correct) vs `5C 12` (little-endian, wrong)
- Humidity: `56 54` (big-endian, correct) vs `54 56` (little-endian, wrong)
- Power info: `96 14` (big-endian, correct) vs `14 96` (little-endian, wrong)

### Data Conversions

#### Temperature
- Input: 0.01°C units (e.g., 2350 = 23.50°C)
- Output: 0.005°C resolution
- Formula: `value = input * 2`
- Range: -16383 to +16383 input (-163.835°C to +163.835°C)
- Overflow protection: Values clamped to valid range

#### Humidity
- Input: 0.01% units (e.g., 5525 = 55.25%)
- Output: 0.0025% resolution
- Formula: `value = input * 4`
- Range: 0 to 10000 input (0% to 100%)
- Overflow protection: Values clamped to 100%

#### Battery Voltage
- Range: 1600-3646 mV
- Encoding: `((voltage_mv - 1600) << 5) & 0xFFE0`
- Stored in bits 11-5 of power_info field

#### TX Power
- Value: 0 dBm (default)
- Encoding: `((0 + 40) / 2) & 0x1F = 20`
- Stored in bits 4-0 of power_info field
- 2 dBm step resolution

### Unused Fields
- Pressure: 0xFFFF (sentinel for unavailable)
- Acceleration X/Y/Z: 0 (not available)
- Movement counter: 0 (not used)

## Safety Features

### Overflow Protection
- Temperature: Clamped to -16383 to +16383 before multiplication
- Humidity: Clamped to 0 to 10000 before multiplication
- Battery: Clamped to 1600-3646 mV before encoding

### Configuration Compatibility
- Documented that advertising_type bitfield expansion may require configuration reset
- Users advised to verify settings after firmware upgrade
- Structure change is backward-compatible at the byte level but may affect bitfield interpretation

## Testing & Validation

### Test Script Results
```
✓ Temperature: 23.50°C → 4700 (0x125C big-endian) ✓
✓ Humidity: 55.25% → 22100 (0x5654 big-endian) ✓
✓ Battery: 2800 mV → 0x9614 (big-endian) correctly encoded ✓
✓ TX Power: 0 dBm → 20 encoded in 2dBm steps ✓
✓ Packet size: 28 bytes (size field = 27) ✓
✓ Company ID: 0x99 0x04 (little-endian per BLE spec) ✓
✓ Data format: 0x05 ✓
✓ Big-endian verification:
  - Temperature bytes: 12 5C (big-endian decode: 4700 = 23.500°C) ✓
  - Humidity bytes: 56 54 (big-endian decode: 22100 = 55.250%) ✓
  - Power info bytes: 96 14 (big-endian decode: battery 2800mV, TX 0dBm) ✓
```

### Code Review
- All review comments addressed
- No security issues found (CodeQL scan)
- Proper error handling and bounds checking
- Clear documentation and comments

## Compatibility

### Devices
- Victron Cerbo GX BLE scanning
- Ruuvi Gateway devices
- Ruuvi mobile applications
- Any Ruuvi RAWv2 compatible receiver

### Limitations
- No pressure sensor (set to sentinel 0xFFFF)
- No accelerometer (set to 0)
- No movement detection (set to 0)
- Fixed TX power at 0 dBm

## Integration Notes

### Compile-Time Configuration
The feature is enabled by default with `USE_RUUVI_BEACON=1` in app_config.h. To disable:
```c
#define USE_RUUVI_BEACON 0
```

### Runtime Selection
Users can select Ruuvi RAWv2 format as advertising type 4 through the device configuration interface.

### RDS Support
For devices with reed switch/pulse counter (RDS) support, the Ruuvi format uses the default event beacon implementation, similar to ATC format.

## Build Status
- Code compiles with all beacon types enabled
- No compiler warnings
- No CodeQL security alerts
- Minimal code size impact (single new function + structure)

## Next Steps for Users

1. Flash the updated firmware
2. Verify or reset device configuration if needed
3. Select "Ruuvi" as the advertisement type (type 4)
4. Verify reception with Ruuvi-compatible receiver
5. Monitor for expected temperature, humidity, and battery values

## References
- [Ruuvi Sensor Protocols](https://github.com/ruuvi/ruuvi-sensor-protocols)
- [Ruuvi RAWv2 Specification](https://docs.ruuvi.com/communication/bluetooth-advertisements/data-format-5-rawv2)
- [Victron Cerbo GX Documentation](https://www.victronenergy.com/live/venus-os:gx_device_ruuvi_tags)

## Implementation Date
February 13, 2026

## Contributors
- Implementation: GitHub Copilot
- Testing & Validation: Automated testing + manual review
- Code Review: Automated code review with feedback integration
