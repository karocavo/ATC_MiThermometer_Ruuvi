# SDK Version Compatibility Guide

## Problem: Different SDK Versions

Users have reported different SDK versions with different flash.h structures:

### SDK Version A (229 lines)
- **File:** `SDK/components/drivers/8258/flash.h`
- **Size:** 229 lines
- **Features:** Has extended API with `#if FLASH_EXTENDED_API` block
- **Functions:**
  - Line 102: `void flash_unlock(void);`
  - Line 227: `void flash_unlock(Flash_TypeDef type);` (inside #if block)

### SDK Version B (103 lines)
- **File:** `SDK/components/drivers/8258/flash.h`  
- **Size:** 103 lines
- **Features:** NO extended API block
- **Functions:**
  - Line 102: `void flash_unlock(void);` only
  - No parameterized version

## Solution: Universal Compatibility

Our code now works with **BOTH** SDK versions automatically!

### How It Works

**Conditional compilation:**
```c
#if FLASH_EXTENDED_API
    // SDK with extended API (229 lines)
    Flash_TypeDef flash_type = detect_flash_type();
    flash_unlock(flash_type);  // Parameterized version
#else
    // SDK without extended API (103 lines)
    if (is_puya_flash()) {
        manual_puya_unlock();  // 16-bit status register
    } else {
        flash_unlock();  // Basic version
    }
#endif
```

## Configuration

### For 229-Line SDK (Extended API Available)

**In makefile line 15:**
```makefile
TEL_CHIP += -DFLASH_EXTENDED_API=1
```

**Result:**
- Uses parameterized `flash_unlock(Flash_TypeDef type)`
- Automatically detects GD/XTX/PUYA
- SDK handles 8-bit vs 16-bit status register

### For 103-Line SDK (No Extended API)

**In makefile line 15:**
```makefile
# Option 1: Comment out the line
# TEL_CHIP += -DFLASH_EXTENDED_API=1

# Option 2: Set to 0
TEL_CHIP += -DFLASH_EXTENDED_API=0
```

**Result:**
- Uses basic `flash_unlock()` for GD/XTX
- Manual 16-bit unlock for PUYA chips
- Still auto-detects flash type by MID
- Works correctly with all flash types!

## How to Check Your SDK Version

**Method 1: Count lines**
```bash
wc -l SDK/components/drivers/8258/flash.h
```

**Output interpretation:**
- `103` or similar → 103-line SDK, set `FLASH_EXTENDED_API=0`
- `229` or similar → 229-line SDK, keep `FLASH_EXTENDED_API=1`

**Method 2: Search for extended API**
```bash
grep -n "FLASH_EXTENDED_API" SDK/components/drivers/8258/flash.h
```

**Output interpretation:**
- No output → 103-line SDK, set `FLASH_EXTENDED_API=0`
- Shows line 123 → 229-line SDK, keep `FLASH_EXTENDED_API=1`

**Method 3: Check for parameterized version**
```bash
grep -n "void flash_unlock(Flash_TypeDef" SDK/components/drivers/8258/flash.h
```

**Output interpretation:**
- No output → 103-line SDK, set `FLASH_EXTENDED_API=0`
- Shows line 227 → 229-line SDK, keep `FLASH_EXTENDED_API=1`

## Manual PUYA Unlock (103-Line SDK)

For SDKs without extended API, we manually unlock PUYA flash:

```c
if (flash_mid[0] == 0x85) {  // PUYA
    // Write 16-bit status register
    mspi_high();
    mspi_wait();
    mspi_write(FLASH_WRITE_ENABLE_CMD);  // 0x06
    mspi_high();
    mspi_wait();
    mspi_write(FLASH_WRITE_STATUS_CMD);  // 0x01
    mspi_write(0x00);  // Status Register 1
    mspi_write(0x00);  // Status Register 2 (PUYA needs this!)
    mspi_high();
    mspi_wait();
} else {
    // GD/XTX: 8-bit status register
    flash_unlock();  // Basic SDK function
}
```

### Why PUYA Needs This

| Flash Type | Status Register | Unlock Method |
|------------|----------------|---------------|
| GD (0xC8) | 8-bit | `flash_unlock()` |
| XTX (0x0B) | 8-bit | `flash_unlock()` |
| PUYA (0x85) | **16-bit** | **Manual unlock** |

PUYA chips have **two** status registers that must both be written to unlock flash for writing.

## Flash Type Detection

**Works with both SDK versions:**

```c
u8 flash_mid[3];
flash_read_id(flash_mid);

// Manufacturer ID (MID) in flash_mid[0]:
// 0xC8 = GigaDevice (GD)
// 0x0B = XTX
// 0x85 = PUYA
```

## Compilation

### 229-Line SDK
```bash
make clean
make  # FLASH_EXTENDED_API=1 by default
```

### 103-Line SDK
```bash
# Edit makefile line 15: Comment out or set to 0
make clean
make
```

## Testing

**For all SDK versions:**

1. Compile and flash firmware
2. Connect via BLE
3. Try config changes (advertising interval, LCD mode, etc.)
4. **Power cycle device**
5. Verify changes persist

**If changes don't persist:**
- Check your SDK line count
- Verify FLASH_EXTENDED_API setting matches SDK
- Check flash type (might be PUYA)

## Benefits

✅ **Universal compatibility** - One codebase for all SDKs
✅ **Auto-detection** - Detects flash type automatically
✅ **Correct unlock** - PUYA gets 16-bit, GD/XTX get 8-bit
✅ **No manual config** - Just set FLASH_EXTENDED_API correctly
✅ **Future-proof** - Works with old and new SDKs

## FAQ

### Q: How do I know which SDK I have?

**A:** Count lines in flash.h:
```bash
wc -l SDK/components/drivers/8258/flash.h
```
- 103 lines → Old SDK
- 229 lines → New SDK

### Q: What if I set FLASH_EXTENDED_API wrong?

**A:** You'll get compilation errors:
- Set to 1 with 103-line SDK → "too many arguments to function 'flash_unlock'"
- Set to 0 with 229-line SDK → Will compile but use manual unlock (still works!)

### Q: Can I just use manual unlock for all SDKs?

**A:** Yes! Set `FLASH_EXTENDED_API=0` even with 229-line SDK. The manual method works universally, but extended API is more elegant if available.

### Q: Does this affect flash writes?

**A:** Yes! This fix is specifically for flash write persistence. Without proper unlock:
- PUYA chips: Flash stays write-protected
- GD/XTX chips: May work with basic unlock
- Config changes: Won't persist after reboot

### Q: What if I have a different flash chip?

**A:** Code defaults to GD unlock for unknown chips (MID not 0x85 or 0x0B). Most chips use GD-compatible 8-bit unlock. If you have issues, check your flash datasheet.

## History

1. **Initial approach:** Hardcoded `FLASH_TYPE_GD`
   - ❌ Failed on PUYA chips

2. **Extended API approach:** Auto-detect + parameterized unlock
   - ✅ Works with 229-line SDK
   - ❌ Failed on 103-line SDK

3. **Current (universal):** Conditional compilation
   - ✅ Works with 229-line SDK (extended API)
   - ✅ Works with 103-line SDK (manual unlock)
   - ✅ Auto-detects all flash types

## Related Documentation

- `FLASH_WRITE_FIX.md` - Flash write persistence issue
- `FLASH_AUTO_DETECT.md` - Automatic flash type detection
- `SDK_FLASH_H_EXPLAINED.md` - SDK flash.h structure (229-line)
- `SDK_FLASH_H_LINE_VERIFICATION.md` - Line number verification

## Summary

**Set FLASH_EXTENDED_API based on your SDK:**
- 229 lines → `FLASH_EXTENDED_API=1` (default)
- 103 lines → `FLASH_EXTENDED_API=0` (comment out)

**Both configurations work correctly with all flash types!**
