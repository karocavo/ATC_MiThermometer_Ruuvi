# Ruuvi RAWv2 (Data Format 5) BLE Advertisement Support

## Overview

This implementation adds support for Ruuvi RAWv2 (Data Format 5) BLE advertisement format to the ATC_MiThermometer firmware. This format is compatible with Victron Cerbo BLE scanning for RuuviTags and other Ruuvi-compatible BLE receivers.

## Ruuvi RAWv2 Format Specification

### Critical Note: BIG-ENDIAN Encoding

**IMPORTANT:** According to the official Ruuvi RAWv2 specification, **ALL multi-byte fields MUST be transmitted in big-endian format (MSB first)**. This is explicitly stated in the Ruuvi protocol documentation:

> "All fields are MSB first 2-complement, i.e. 0xFC18 is read as -1000 and 0x03E8 is read as 1000. Values are 2-complement int16_t, MSB first."

This differs from many BLE implementations that use little-endian. The implementation uses byte arrays and explicit byte-order conversion macros to ensure correct big-endian encoding.

### Manufacturer Specific Data Structure

The Ruuvi RAWv2 format uses Manufacturer Specific Data (AD Type 0xFF) with the following structure (28 bytes total):

| Offset | Length | Description | Format | Endianness | Notes |
|--------|--------|-------------|--------|------------|-------|
| 0 | 1 | Size | uint8 | N/A | Total size - 1 (27 bytes) |
| 1 | 1 | AD Type | uint8 | N/A | 0xFF (Manufacturer Specific Data) |
| 2-3 | 2 | Company ID | uint16 | **LITTLE** | 0x0499 (Ruuvi Innovations Ltd.) transmitted as 0x99 0x04 |
| 4 | 1 | Data Format | uint8 | N/A | 0x05 (RAWv2) |
| 5-6 | 2 | Temperature | int16 | **BIG** | Resolution: 0.005°C |
| 7-8 | 2 | Humidity | uint16 | **BIG** | Resolution: 0.0025% |
| 9-10 | 2 | Pressure | uint16 | **BIG** | Not used, set to 0xFFFF |
| 11-12 | 2 | Acceleration X | int16 | **BIG** | Not used, set to 0 |
| 13-14 | 2 | Acceleration Y | int16 | **BIG** | Not used, set to 0 |
| 15-16 | 2 | Acceleration Z | int16 | **BIG** | Not used, set to 0 |
| 17-18 | 2 | Power Info | uint16 | **BIG** | See below |
| 19 | 1 | Movement Counter | uint8 | N/A | Not used, set to 0 |
| 20-21 | 2 | Measurement Seq | uint16 | **BIG** | Measurement count |
| 22-27 | 6 | MAC Address | 6 bytes | LITTLE | Device MAC address (BLE standard) |

**Note:** Only the Company ID (per BLE specification) and MAC address use little-endian. All Ruuvi data fields use big-endian.

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

**This firmware version changes the internal configuration structure.** The `advertising_type` field was expanded from 2 to 3 bits to accommodate the new Ruuvi format option. 

**Configuration Migration:**
- The structure still fits within the same memory layout
- After flashing this firmware, verify your advertisement type setting
- If you experience unexpected behavior, perform a configuration reset
- Future versions may include automatic migration logic based on `EEP_SUP_VER`

**For Developers:**
- Consider incrementing `EEP_SUP_VER` in `app_config.h` to force config reset
- The VERSION field is currently 0x57 (v5.7 in BCD format)
- Migration logic could check old `advertising_type` values (0-3) and validate

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

## Future Enhancements

### Extended Temperature Range
Currently, the implementation clamps temperature to ±163.83°C (input range ±16383 in 0.01°C units) to prevent overflow. The Ruuvi RAWv2 format theoretically supports the full ±163.835°C range, and this could be expanded if needed for:
- External high-temperature sensor support (e.g., via header connector)
- Industrial or specialized applications
- The full range would require input values up to ±32767 after the *2 conversion

### Runtime Configuration
Currently, the Ruuvi beacon format is controlled by:
- **Compile-time**: `USE_RUUVI_BEACON` flag in `app_config.h`
- **Runtime**: Selection via `advertising_type` field (value 4)

Future enhancements could add:
- Web UI toggle for enabling/disabling Ruuvi format
- Flash-based runtime configuration
- No recompilation required for format changes

### Alternating Advertisements
For maximum compatibility, future versions could support alternating between different advertisement formats:
- **Ruuvi RAWv2** for Victron Cerbo GX compatibility
- **BTHome v2** for Home Assistant integration
- Alternation could occur on a per-packet or time-based schedule
- Would require coordination with existing beacon rotation logic

### MAC Address Handling
The MAC address is embedded in the advertisement payload:
- Automatically populated from device MAC via `mac_public` variable
- Cannot be changed without reflashing firmware
- OTA updates via WebUI flasher tool maintain the same MAC address
- MAC is part of the BLE controller, not changeable at application level
- For testing with different MACs, would require hardware programming

## Example Victron Cerbo GX Compatible Packet

Here's a complete example packet that should be recognized by Victron Cerbo GX:

```
1BFF990405125C5654FFFF000000000000961400007B112233445566
```

Breakdown:
- `1B` - Size (27 bytes)
- `FF` - Manufacturer Specific Data
- `9904` - Ruuvi Company ID (0x0499 little-endian)
- `05` - Data Format 5 (RAWv2)
- `125C` - Temperature: 4700 = 23.50°C (big-endian)
- `5654` - Humidity: 22100 = 55.25% (big-endian)
- `FFFF` - Pressure: unavailable sentinel
- `000000000000` - Acceleration X/Y/Z: all 0
- `9614` - Power info: battery 2800mV, TX 0dBm (big-endian)
- `00` - Movement counter: 0
- `007B` - Sequence: 123 (big-endian)
- `112233445566` - MAC address

You can verify this packet structure using:
- **nRF Connect** mobile app (Nordic Semiconductor)
- **Wireshark** with Bluetooth LE capture
- **Victron Cerbo GX** BLE scanner (should detect as RuuviTag)

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
