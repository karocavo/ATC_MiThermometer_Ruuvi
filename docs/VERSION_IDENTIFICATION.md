# Version Identification After OTA

## Problem
After performing an OTA (Over-The-Air) firmware update, it's difficult to tell if the update was successful because version strings may not be clearly visible or may not have changed.

## Solution Implemented

### 1. Version Number Update
**File:** `src/app_config.h`

Changed VERSION from `0x57` to `0x58`:
```c
#define VERSION 0x58  // BCD format - displays as "V5.8"
```

This version will be displayed in the **Software Revision** BLE characteristic (UUID 0x2A28).

### 2. Manufacturer String Update  
**File:** `src/app_att.c`

Changed Manufacturer Name from `"DIY.home"` to `"DIY.Ruuvi-v58"`:
```c
static const u8 my_ManStr[] = {"DIY.Ruuvi-v58"};
```

This appears in the **Manufacturer Name String** BLE characteristic (UUID 0x2A29).

### How to Verify OTA Success

After flashing the new firmware, connect with a BLE scanner app (nRF Connect, LightBlue, etc.) and check:

1. **Manufacturer Name**: Should show `"DIY.Ruuvi-v58"` (was `"DIY.home"`)
2. **Software Revision**: Should show `"V5.8"` (was `"V5.7"`)
3. **Firmware Revision**: Should show `"github.com/pvvx"`

If you see these values, the OTA was successful!

## Configuration Safety

The firmware includes safeguards to prevent common issues:

### Advertising Interval Protection
If the config is corrupted and advertising_interval is 0, it's automatically fixed:
```c
if (cfg.advertising_interval == 0)
    cfg.advertising_interval = 1;  // Minimum: 62.5ms
```

### Screen Control
The LCD/display can be controlled via `cfg.flg2.screen_off` flag:
- `0` = Screen ON (default)
- `1` = Screen OFF (power saving)

## Common Issues After OTA

### Symptom: LCD Dead + No Advertisements

**Possible Causes:**
1. **Config Corrupted During Flash**
   - advertising_interval set to 0
   - screen_off flag incorrectly set
   
2. **Partial Flash**
   - Critical config sector not fully written
   - Use bootloader recovery (10x reset method)

3. **Boot Stuck in Low Power Mode**
   - Check battery voltage
   - Reflash with programmer if needed

### Recovery Methods

1. **10x Reset to Bootloader**
   - Press reset button 10 times quickly
   - Device enters bootloader mode
   - Reflash with programmer

2. **Reflash Original ATC**
   - Flash known-good ATC firmware
   - Device should recover
   - Then retry your custom firmware

3. **Erase All & Reflash**
   - Use SWD programmer
   - Erase all flash sectors
   - Flash new firmware
   - Config will use defaults

## Future Version Updates

When releasing new firmware versions:

1. Increment VERSION in `src/app_config.h`:
   ```c
   #define VERSION 0x59  // Next version
   ```

2. Update Manufacturer string in `src/app_att.c`:
   ```c
   static const u8 my_ManStr[] = {"DIY.Ruuvi-v59"};
   ```

3. This makes it immediately obvious which firmware is running!

## Testing Before Release

Before releasing OTA firmware:

1. **Test on device first**
   - Flash via programmer
   - Verify LCD works
   - Verify BLE advertising
   - Check version strings

2. **Test OTA update**
   - Start with known-good firmware
   - Perform OTA
   - Verify version changes
   - Verify all functions work

3. **Have recovery plan**
   - Keep programmer handy
   - Document recovery procedure
   - Test bootloader recovery

## Related Files

- `src/app_config.h` - VERSION definition
- `src/app_att.c` - BLE characteristic strings  
- `src/app.c` - Config initialization and validation
- `src/app.h` - Config structure definition
