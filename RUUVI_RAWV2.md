# Ruuvi RAWv2 (Data Format 5) BLE Advertisement Support

## Overview

This implementation adds support for Ruuvi RAWv2 (Data Format 5) BLE advertisement format to the ATC_MiThermometer firmware. This format is compatible with Victron Cerbo BLE scanning for RuuviTags and other Ruuvi-compatible BLE receivers.

## Ruuvi RAWv2 Format Specification

### Manufacturer Specific Data Structure

The Ruuvi RAWv2 format uses Manufacturer Specific Data (AD Type 0xFF) with the following structure (28 bytes total):

| Offset | Length | Description | Format | Notes |
|--------|--------|-------------|--------|-------|
| 0 | 1 | Size | uint8 | Total size - 1 (27 bytes) |
| 1 | 1 | AD Type | uint8 | 0xFF (Manufacturer Specific Data) |
| 2-3 | 2 | Company ID | uint16 LE | 0x0499 (Ruuvi Innovations Ltd.) |
| 4 | 1 | Data Format | uint8 | 0x05 (RAWv2) |
| 5-6 | 2 | Temperature | int16 LE | Resolution: 0.005°C |
| 7-8 | 2 | Humidity | uint16 LE | Resolution: 0.0025% |
| 9-10 | 2 | Pressure | uint16 LE | Not used, set to 0xFFFF |
| 11-12 | 2 | Acceleration X | int16 LE | Not used, set to 0 |
| 13-14 | 2 | Acceleration Y | int16 LE | Not used, set to 0 |
| 15-16 | 2 | Acceleration Z | int16 LE | Not used, set to 0 |
| 17-18 | 2 | Power Info | uint16 LE | See below |
| 19 | 1 | Movement Counter | uint8 | Not used, set to 0 |
| 20-21 | 2 | Measurement Seq | uint16 LE | Measurement count |
| 22-27 | 6 | MAC Address | 6 bytes | Device MAC address |

### Power Info Field (bytes 17-18)

The power info field is a 16-bit value packed as follows:
- **Bits 15-5 (11 bits)**: Battery voltage above 1600mV in 1mV increments
  - Range: 1600-3646 mV
  - Formula: `encoded_value = battery_mv - 1600`
- **Bits 4-0 (5 bits)**: TX power in 2dBm steps, offset by -40dBm
  - Range: -40 to +22 dBm
  - Formula: `encoded_value = (tx_power_dbm + 40) / 2`

## Implementation Details

### Temperature Conversion

The LYWSD03MMC measures temperature in 0.01°C units, while Ruuvi uses 0.005°C resolution:

```c
// measured_data.temp is in 0.01°C units
// Ruuvi format: value = temperature / 0.005
// Conversion: (temp * 0.01) / 0.005 = temp * 2
// Valid range check to prevent overflow
s16 temp_clamped = measured_data.temp;
if (temp_clamped < -16383) temp_clamped = -16383;
if (temp_clamped > 16383) temp_clamped = 16383;
ruuvi_temp = temp_clamped * 2;
```

**Example:**
- Measured: 2350 (23.50°C)
- Ruuvi: 4700 (23.50°C / 0.005 = 4700)

**Valid Range:**
- Ruuvi RAWv2: -163.835°C to +163.835°C
- Input range: -16383 to +16383 (in 0.01°C units)
- Values outside this range are clamped

### Humidity Conversion

The LYWSD03MMC measures humidity in 0.01% units, while Ruuvi uses 0.0025% resolution:

```c
// measured_data.humi is in 0.01% units
// Ruuvi format: value = humidity / 0.0025
// Conversion: (humi * 0.01) / 0.0025 = humi * 4
// Valid range check to prevent overflow
u16 humi_clamped = measured_data.humi;
if (humi_clamped > 10000) humi_clamped = 10000;
ruuvi_humi = humi_clamped * 4;
```

**Example:**
- Measured: 5525 (55.25%)
- Ruuvi: 22100 (55.25% / 0.0025 = 22100)

**Valid Range:**
- Ruuvi RAWv2 can encode: 0% to 163.835%
- Sensor valid range: 0% to 100% (0 to 10000 in 0.01% units)
- Values above 100% are clamped

### Battery Voltage Encoding

```c
// Clamp to valid range
if (battery_mv < 1600) battery_mv = 1600;
if (battery_mv > 3646) battery_mv = 3646;

// Encode: subtract 1600 and shift to bits 11-5
battery_shifted = ((battery_mv - 1600) << 5) & 0xFFE0;
```

**Example:**
- Battery: 2800 mV
- Encoded: (2800 - 1600) = 1200 → shifted to bits 11-5 → 0x9600

### TX Power Encoding

The implementation uses 0 dBm as the default TX power:

```c
// TX power: 0 dBm
// Encode: (0 + 40) / 2 = 20
tx_power_bits = 20 & 0x1F;
power_info = battery_shifted | tx_power_bits;
```

**Example:**
- TX Power: 0 dBm
- Encoded: (0 + 40) / 2 = 20 → 0x14

## Usage

### Important: Configuration Compatibility Note

**This firmware version changes the internal configuration structure.** The `advertising_type` field was expanded from 2 to 3 bits to accommodate the new Ruuvi format option. While the structure still fits within the same memory layout, if you are upgrading from a previous firmware version:

- Your existing configuration may need to be reset
- After flashing this firmware, verify your advertisement type setting in the configuration interface
- If you experience unexpected behavior, perform a factory reset of the configuration

### Selecting Ruuvi Format

The Ruuvi RAWv2 format can be selected through the configuration interface as advertising type 4 (ADV_TYPE_RUUVI).

### Compile-Time Configuration

The Ruuvi beacon support is controlled by the `USE_RUUVI_BEACON` flag in `app_config.h`:

```c
#ifndef USE_RUUVI_BEACON
#define USE_RUUVI_BEACON	1 // = 1 Ruuvi RAWv2 (Data Format 5)
#endif
```

## Limitations

1. **No Pressure Sensor**: The LYWSD03MMC does not have a pressure sensor, so the pressure field is set to the sentinel value 0xFFFF.

2. **No Accelerometer**: The device does not have an accelerometer, so all acceleration fields are set to 0.

3. **No Movement Counter**: The movement counter field is not applicable and is set to 0.

4. **Fixed TX Power**: The TX power is fixed at 0 dBm in this implementation.

## Compatibility

This implementation is designed to be compatible with:
- Victron Cerbo BLE scanning for RuuviTags
- Ruuvi Gateway devices
- Ruuvi Mobile applications
- Any BLE scanner that supports Ruuvi RAWv2 format

## Testing

A test script (`test_ruuvi_format.py`) is provided to validate the packet structure and field encodings:

```bash
python3 test_ruuvi_format.py
```

## References

- [Ruuvi Sensor Protocols](https://github.com/ruuvi/ruuvi-sensor-protocols)
- [Ruuvi RAWv2 Specification](https://docs.ruuvi.com/communication/bluetooth-advertisements/data-format-5-rawv2)
- [Victron Cerbo GX BLE Support](https://www.victronenergy.com/live/venus-os:gx_device_ruuvi_tags)
