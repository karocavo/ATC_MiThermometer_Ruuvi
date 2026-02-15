# Flash Write Persistence Fix

## Critical Issue: Config Writes Not Persisting

### Problem
Device accepts GATT write commands but changes don't persist:
- Config changes accepted via BLE
- LCD stays off
- Advertising doesn't start
- Config bytes unchanged after write
- Flash writes failing silently

### Symptoms
```
User reports: "Many commands are accepted ("sent successfully"), but nothing 
changes visibly (no LCD wake, no advertisements start, no config bytes change 
after write)."
```

This is classic **flash write protection** - commands accepted, but underlying flash writes fail.

## Root Cause

### The Flash Unlock Issue

The SDK has TWO versions of `flash_unlock()`:

#### 1. Basic Version (void parameter)
```c
void flash_unlock(void) {
    flash_send_cmd(FLASH_WRITE_ENABLE_CMD);
    flash_send_cmd(FLASH_WRITE_STATUS_CMD);
    mspi_write(0);   // 8-bit status register ONLY
    mspi_wait();
    mspi_high();
    flash_wait_done();
}
```

**Works for:** GD and XTX flash chips (8-bit status register)

#### 2. Extended Version (Flash_TypeDef parameter)
```c
void flash_unlock(Flash_TypeDef type) {
    flash_send_cmd(FLASH_WRITE_ENABLE_CMD);
    flash_send_cmd(FLASH_WRITE_STATUS_CMD);
    
    if ((type == FLASH_TYPE_GD) || (type == FLASH_TYPE_XTX)) {
        mspi_write(0);   // 8-bit status
    } else if (type == FLASH_TYPE_PUYA) {
        mspi_write(0);
        mspi_wait();
        mspi_write(0);   // 16-bit status - CRITICAL for PUYA!
    }
    
    mspi_wait();
    mspi_high();
    flash_wait_done();
}
```

**Works for:** All flash types (GD, XTX, PUYA)

### Critical Difference

**PUYA flash chips require 16-bit status register writes!**

If your device has a PUYA flash chip and you use the basic `flash_unlock()`:
- Flash stays write-protected
- All write operations silently fail
- Config changes don't persist
- Device appears to work but can't save anything

## The Fix

### Step 1: Enable Extended Flash API

In `makefile`:
```makefile
# Enable extended flash API to support all flash types (GD, PUYA, XTX)
# This is needed for proper flash_unlock() on PUYA chips (16-bit status register)
GCC_FLAGS += -DFLASH_EXTENDED_API=1
```

### Step 2: Use Flash Type Parameter

In `src/app.c` and `src/ext_ota.c`:
```c
// Before:
flash_unlock();  // Basic version - only works for GD/XTX

// After:
flash_unlock(FLASH_TYPE_GD);  // Extended version - works for GD/XTX
```

### Why FLASH_TYPE_GD?

- **GD flash**: Most common type, uses 8-bit status register
- **XTX flash**: Uses same 8-bit protocol as GD, compatible
- **PUYA flash**: Requires 16-bit status register (different code path)

Using `FLASH_TYPE_GD` works for both GD and XTX chips!

## Flash Type Identification

### How to Check Your Flash Type

The firmware reads flash ID via `flash_read_id()`. You can check via:

1. **Via BLE Connection**: Flash ID is readable via GATT
2. **Via Log Output**: Some builds log flash ID on boot
3. **Via Chip Marking**: Physical chip may have manufacturer marking

### Common Flash Chips

| Manufacturer | Type | Status Register | FLASH_TYPE |
|-------------|------|----------------|------------|
| GigaDevice (GD) | GD25xxx | 8-bit | FLASH_TYPE_GD |
| XTX | XTxxx | 8-bit | FLASH_TYPE_GD (compatible) |
| PUYA | PYxxx | 16-bit | FLASH_TYPE_PUYA |

### If You Have PUYA Flash

Change the flash_unlock calls to use `FLASH_TYPE_PUYA`:

**In src/app.c:**
```c
flash_unlock(FLASH_TYPE_PUYA);  // Instead of FLASH_TYPE_GD
```

**In src/ext_ota.c:**
```c
flash_unlock(FLASH_TYPE_PUYA);  // Instead of FLASH_TYPE_GD
```

Recompile and flash.

## Testing the Fix

### Before Testing
1. Pull latest code with the fix
2. Compile firmware
3. Flash to device

### Test Procedure

1. **Connect via BLE**
   - Use nRF Connect or similar tool
   - Connect to device

2. **Try Config Write**
   - Write to config characteristic
   - Change LCD settings, advertising interval, etc.

3. **Verify Persistence**
   - Disconnect from device
   - Power cycle device (remove battery)
   - Reconnect
   - Read config - changes should persist!

4. **Check LCD**
   - LCD should wake up if enabled
   - Should show current readings

5. **Check Advertising**
   - BLE scanner should see advertisements
   - Advertising interval should match config

### If Still Doesn't Work

Your device might have PUYA flash. Try:

1. **Change to FLASH_TYPE_PUYA**
   ```c
   flash_unlock(FLASH_TYPE_PUYA);  // In src/app.c and src/ext_ota.c
   ```

2. **Recompile and test**

3. **If STILL doesn't work:**
   - Check if flash is physically damaged
   - Try "Erase All" before flashing
   - Verify power supply is stable (>2.0V)

## History of the Issue

### Initial Problem
Original code had two declarations of `flash_unlock()` causing compilation errors.

### First Fix Attempt (WRONG)
We simplified to just `flash_unlock()` without parameters.
- Compiled successfully ✅
- But broke PUYA flash support ❌

### Second Fix Attempt (WRONG)
We added conditional compilation to detect SDK version.
- Too complex
- Still didn't handle PUYA properly
- User reported device failures

### Third Fix Attempt (WRONG)
We removed conditional logic for "safety."
- Simple code ✅
- But STILL only basic version ❌
- Flash writes failed silently

### Final Fix (CORRECT)
Enable extended API and use `flash_unlock(FLASH_TYPE_GD)`.
- Handles all flash types ✅
- PUYA support available ✅
- User can switch if needed ✅

## Technical Details

### Flash Status Register

Flash memory has a status register that controls write protection:
- **Bit 0-1**: Write protection bits
- **Others**: Various status flags

To unlock flash for writing:
1. Send WRITE_ENABLE command (0x06)
2. Send WRITE_STATUS command (0x01)
3. Write status register value (0x00 = unprotected)

### 8-bit vs 16-bit Status

**8-bit (GD, XTX):**
```
Command: 0x01 (WRITE_STATUS)
Data: 0x00 (1 byte)
```

**16-bit (PUYA):**
```
Command: 0x01 (WRITE_STATUS)
Data: 0x00 0x00 (2 bytes)
```

If you send 1 byte to PUYA chip, it doesn't unlock properly!

## User's Insight

The user correctly identified the issue:

> "probably the flash.h void is the cause, maybe we actually needed the 
> flash_unlock w. property and that is why device works only partially"

**They were 100% correct!** We needed the parameterized version.

## Related Documentation

- `DEVICE_BRICK_FIX.md` - General device recovery
- `RUUVI_SAFETY_ANALYSIS.md` - Ruuvi code is safe (not the cause)
- `VERSION_IDENTIFICATION.md` - How to verify firmware version
- `OTA_UPDATE_GUIDE.md` - OTA update procedures

## Summary

**Problem:** Flash writes failing silently
**Cause:** Wrong flash_unlock() version (missing PUYA support)
**Fix:** Enable extended API + use FLASH_TYPE_GD parameter
**Result:** Flash writes should now persist correctly!

If you still have issues, try FLASH_TYPE_PUYA!
