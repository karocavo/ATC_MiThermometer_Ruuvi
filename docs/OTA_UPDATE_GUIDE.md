# OTA Update Guide - How to Know It Worked

## Quick Answer

After flashing the new firmware (v5.8), connect with a BLE scanner app and check:

**Before (v5.7):**
- Manufacturer Name: "DIY.home"
- Software Revision: "V5.7"

**After (v5.8 with Ruuvi):**
- Manufacturer Name: "DIY.Ruuvi-v58" ← **NEW!**
- Software Revision: "V5.8" ← **NEW!**

If you see these new values, **the OTA worked!**

## What Was Changed

### 1. Version Number
**File:** `src/app_config.h`
```c
#define VERSION 0x58  // Was 0x57
```
This displays as "V5.8" in Software Revision characteristic.

### 2. Manufacturer String
**File:** `src/app_att.c`
```c
static const u8 my_ManStr[] = {"DIY.Ruuvi-v58"};  // Was "DIY.home"
```
This makes it obvious you're running the Ruuvi-enabled firmware.

## How to Check After OTA

### Using nRF Connect (Android/iOS)

1. Open nRF Connect
2. Scan for devices
3. Find your thermometer
4. Tap "CONNECT"
5. Scroll to "Device Information" service
6. Check these characteristics:
   - **Manufacturer Name String** → Should say "DIY.Ruuvi-v58"
   - **Software Revision String** → Should say "V5.8"

### Using LightBlue (iOS/macOS)

1. Open LightBlue
2. Find your device in the list
3. Tap to connect
4. Scroll to "Device Information" (UUID 180A)
5. Check:
   - **Manufacturer Name** → "DIY.Ruuvi-v58"
   - **Software Revision** → "V5.8"

### Using Web Bluetooth (Browser)

Many BLE web apps can also read these characteristics. Look for Device Information service (UUID 0x180A).

## Troubleshooting: LCD Dead + No Advertisements

You mentioned: "fw works partially, but Why LCD dead + no ads"

### Most Likely Causes

1. **Config Corruption During Flash**
   - Some config bytes may have wrong values
   - The code has protections, but flash corruption can happen

2. **Screen Mode Set to OFF**
   - Config flag `screen_off` may be set to 1
   - This is a power-saving feature but can look like "dead LCD"

3. **Advertising Interval Set to 0**
   - If config is corrupted, this could happen
   - Code protection exists: `if (cfg.advertising_interval == 0) cfg.advertising_interval = 1;`
   - But if flash is partially written, protection might not run

4. **Partial Flash Write**
   - Critical config sector might not be fully written
   - Device boots but with incomplete configuration

### Solutions

#### Solution 1: Reset to Defaults (Recommended)
The easiest way is to erase the config flash sector and let it use defaults:

1. Use your SWD programmer
2. Erase user config sectors (typically 0x7C000-0x80000)
3. Reflash the firmware
4. Device will use default config (LCD ON, advertising enabled)

#### Solution 2: 10x Reset to Bootloader
You mentioned: "recoverable by 10x push contacts and reflash"

1. Press reset button 10 times quickly
2. Device enters bootloader mode
3. Use programmer to flash firmware
4. This should work if bootloader is intact

#### Solution 3: Flash via Web Tool
If you're using the web flasher:
1. Select "Erase All" option
2. Flash the new firmware
3. This ensures clean flash with default config

#### Solution 4: Manual Config Reset via BLE
If you can connect via BLE (even without LCD):
1. Connect to device
2. Write to config service to reset to defaults
3. Disconnect and reboot
4. Device should use default config

### Debug Checklist

If still not working:

- [ ] Can you see BLE advertisements at all?
  - Yes → Config partially OK, advertising works
  - No → Advertising interval corrupted or device not booting

- [ ] Can you connect via BLE?
  - Yes → BLE stack working, check config
  - No → Device may be in bad state

- [ ] Does LCD show anything during boot?
  - Yes → LCD working, screen_off flag issue
  - No → LCD init failed or powered down

- [ ] Is battery voltage OK?
  - Should be > 2.0V
  - Low battery can cause issues

## Config Protection Already in Code

Good news! The code already has protection:

```c
// From src/app.c
if (cfg.advertising_interval == 0)
    cfg.advertising_interval = 1;  // Minimum: 62.5ms

if (cfg.advertising_interval > 160)
    cfg.advertising_interval = 160;  // Maximum: 10 sec
```

So if config is read properly, advertising will work.

### Default Config Values

From `src/app.c`:
```c
const cfg_t def_cfg = {
    .advertising_interval = 40,      // 2.5 seconds (device dependent)
    .flg2.screen_off = 0,            // Screen ON
    .flg.advertising_type = ADV_TYPE_BTHOME,  // BTHome format
    // ... other defaults
};
```

If config is completely unreadable, these defaults should be used.

## What to Do Next

1. **Flash the new v5.8 firmware**
   - Use your preferred method (OTA, programmer, web tool)
   - Watch for any errors during flash

2. **Check version strings**
   - Connect with BLE scanner
   - Verify Manufacturer = "DIY.Ruuvi-v58"
   - Verify Software = "V5.8"

3. **If LCD/ads don't work:**
   - Try "Erase All" option
   - Reflash firmware
   - Device should use defaults

4. **Test everything:**
   - LCD shows temp/humidity
   - BLE advertising visible
   - Can connect and see data
   - Ruuvi format advertising works

## Need Help?

If you still have issues:

1. **Share detailed info:**
   - Can you see BLE ads?
   - Can you connect?
   - Does LCD do anything?
   - What flashing method?

2. **Check these docs:**
   - `VERSION_IDENTIFICATION.md` - Detailed version info
   - `DEVICE_BRICK_FIX.md` - Recovery methods
   - `RUUVI_SAFETY_ANALYSIS.md` - Ruuvi implementation details

3. **Try recovery:**
   - 10x reset to bootloader
   - Flash via programmer
   - Erase all + reflash

The firmware has proper safeguards, so if flash completes correctly, it should work!
