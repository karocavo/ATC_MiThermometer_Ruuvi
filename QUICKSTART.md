# Quick Start Guide - LYWSD03MMC with Ruuvi Support

## What Was Fixed
- **Critical Makefile bug**: Device was compiling as TS0201 instead of LYWSD03MMC
- **Result**: Device will now be properly recognized, LCD will work, and all features enabled

## Build & Flash

### Build (Windows/Eclipse):
```cmd
make clean
make
```

### Build (Docker):
```bash
docker run --rm -v $(pwd):/project skaldo/telink-sdk:0.1 make -C /project clean
docker run --rm -v $(pwd):/project skaldo/telink-sdk:0.1 make -C /project
```

### Flash:
```cmd
python TlsrPgm.py -p COM6 -w we 0 ATC_Thermometer.bin
```

## Expected Results

### Before Fix:
❌ Device Type: "Unknown or DIY (49)"  
❌ LCD: Shows "0°C / 0%"  
❌ Config: LCD controls hidden in web UI  

### After Fix:
✅ Device Type: "LYWSD03MMC B1.4" (or B1.5/B1.6/B1.7/B1.9/B2.0)  
✅ LCD: Shows correct temp/humi (e.g., "23.7°C / 27%")  
✅ Config: All LCD controls visible (Repair LCD, clock, intervals)  

## Enable Ruuvi Advertising

Ruuvi beacon is **compiled in** but not **default**. To use:

### Option 1: Via Web UI (TelinkMiFlasher)
1. Connect to device
2. Go to Config tab
3. Change "Advertising Type" to **4** (Ruuvi)
4. Click "Send"
5. Scan with nRF Connect - should see Ruuvi beacon

### Option 2: Make Ruuvi Default (recompile)
Edit `src/app.h` line 41:
```c
#define ADV_TYPE_DEFAULT	ADV_TYPE_RUUVI  // was ADV_TYPE_BTHOME
```

## Verify Ruuvi Beacon

### Using nRF Connect:
Look for:
- **Manufacturer Data**: `99 04 05...` (0x0499 = Ruuvi, 0x05 = RAWv2 format)
- **Temperature**: Encoded as int16 big-endian (resolution 0.005°C)
- **Humidity**: Encoded as uint16 big-endian (resolution 0.0025%)
- **MAC Address**: Last 6 bytes of advertisement

### Using Ruuvi Station:
- Should auto-detect as Ruuvi tag
- Shows temperature, humidity, battery

## Available Advertising Formats
All formats are compiled in and selectable:
- **0**: ATC (pvvx custom format)
- **1**: PVVX (extended custom format)
- **2**: Mi Home (Xiaomi format)
- **3**: BTHome v2 (default)
- **4**: Ruuvi RAWv2

## Troubleshooting

### LCD Still Shows 0/0
- Check sensor I2C connection (PC2=SDA, PC3=SCL)
- Verify sensor type detected (SHTC3 @ 0x70 or SHT4x @ 0x44)
- Check flash logs during init_sensor()

### Device Still Shows "Unknown (49)"
- Verify build used fixed Makefile
- Check compiler output for `-DDEVICE_TYPE=DEVICE_LYWSD03MMC`
- Ensure no old .bin cached

### Ruuvi Not Visible
- Verify advertising type set to 4
- Check nRF Connect filter (show all manufacturers)
- Ensure device is advertising (not in deep sleep)

## Additional Notes

- **Hardware Version**: Auto-detected based on LCD I2C address (0x3C/0x3E/UART)
- **Battery**: Monitored and included in Ruuvi beacon
- **Update Rate**: LCD ~2.45s, Advertising ~2.5s (configurable)
- **Range**: Standard BLE (~10m indoors)

## Need More Info?
See **FIXES_APPLIED.md** for detailed technical information.
