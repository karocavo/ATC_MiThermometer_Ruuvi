# SDK Debug Guide - Verifying FLASH_EXTENDED_API

## User's Concern

> "in our master ruuvi SDK folder, is a flash.c (unchanged) where flash_unlock(Flash_TypeDef type) is in a block #if 0... it has if 0, so never used"

## Reality Check: NOT #if 0!

The SDK files do **NOT** use `#if 0`. They use `#if FLASH_EXTENDED_API`.

### Verified SDK Structure

**flash.h:**
```c
Line 102: void flash_unlock(void);              // Basic version - always compiled
Line 123: #if FLASH_EXTENDED_API                // Extended API block starts
Line 227: void flash_unlock(Flash_TypeDef type); // Extended version - conditional
Line 229: #endif                                 // Extended API block ends
```

**flash.c:**
```c
Line 254: #if FLASH_EXTENDED_API                           // Extended API block starts
Line 551: _attribute_ram_code_ void flash_unlock(Flash_TypeDef type) { ... }
Line 574: #endif                                            // Extended API block ends
```

## How to Verify

### 1. Check SDK Files for #if 0

```bash
# Search for #if 0 in SDK files
grep "#if 0" SDK/components/drivers/8258/flash.h
grep "#if 0" SDK/components/drivers/8258/flash.c

# Result should be: (empty)
# There are NO #if 0 blocks!
```

### 2. Check for FLASH_EXTENDED_API

```bash
# Search for FLASH_EXTENDED_API
grep -n "FLASH_EXTENDED_API" SDK/components/drivers/8258/flash.h
# Output: 123:#if FLASH_EXTENDED_API

grep -n "FLASH_EXTENDED_API" SDK/components/drivers/8258/flash.c
# Output: 254:#if FLASH_EXTENDED_API
```

### 3. Verify Makefile Flag

```bash
# Check makefile for flag definition
grep "FLASH_EXTENDED_API" makefile
# Output: TEL_CHIP := -DCHIP_TYPE=CHIP_TYPE_8258 -DFLASH_EXTENDED_API=1
```

### 4. Test Preprocessor

Create a test file to verify preprocessor sees the flag:

```bash
# Create test file
cat > /tmp/test_flash.c << 'EOF'
#include <stdio.h>

#if FLASH_EXTENDED_API
    void flash_unlock_with_param(int type) {
        printf("Extended API active\n");
    }
#else
    void flash_unlock_no_param(void) {
        printf("Basic API only\n");
    }
#endif

int main() {
    #if FLASH_EXTENDED_API
        printf("FLASH_EXTENDED_API = 1\n");
    #else
        printf("FLASH_EXTENDED_API = 0\n");
    #endif
    return 0;
}
EOF

# Compile with flag
gcc -DFLASH_EXTENDED_API=1 /tmp/test_flash.c -o /tmp/test_flash

# Run
/tmp/test_flash
# Should output: FLASH_EXTENDED_API = 1
#                Extended API active
```

### 5. Check Compiled Objects

After building, check if extended API is in object files:

```bash
# Build
make clean
make

# Check symbols in app.o
tc32-elf-nm out/src/app.o | grep flash_unlock
# Should show flash_unlock references

# Disassemble to see calls
tc32-elf-objdump -d out/src/app.o | grep -A5 "flash_unlock"
```

### 6. Verify Preprocessor Output

```bash
# Generate preprocessor output for app.c
tc32-elf-gcc -E src/app.c \
    -DCHIP_TYPE=CHIP_TYPE_8258 \
    -DFLASH_EXTENDED_API=1 \
    -I./SDK/components/drivers/8258 \
    -o /tmp/app_preprocessed.c

# Check if extended API is visible
grep "flash_unlock.*Flash_TypeDef" /tmp/app_preprocessed.c
# Should find the extended version declaration
```

## Common Issues and Solutions

### Issue 1: "too many arguments to function 'flash_unlock'"

**Symptoms:**
```
error: too many arguments to function 'flash_unlock'
note: declared here void flash_unlock(void);
```

**Cause:** FLASH_EXTENDED_API is not being defined during compilation

**Solutions:**
1. Check makefile line 15: Should have `-DFLASH_EXTENDED_API=1` in TEL_CHIP
2. Clean build: `make clean && make`
3. Verify flag is early in makefile (in TEL_CHIP, not later in GCC_FLAGS)

### Issue 2: Extended API Not Compiling

**Symptoms:** Only basic `void flash_unlock(void)` is available

**Cause:** 
- Flag defined too late
- Stale build artifacts
- Wrong SDK version

**Solutions:**
```bash
# 1. Clean everything
make clean
rm -rf out/

# 2. Verify makefile
head -20 makefile | grep FLASH_EXTENDED_API

# 3. Rebuild
make

# 4. Verify compilation
make VERBOSE=1 2>&1 | grep FLASH_EXTENDED_API
```

### Issue 3: Different SDK Version

**Symptoms:** User sees different line numbers or file structure

**Cause:** Using external SDK path instead of local ./SDK

**Solutions:**
1. Check makefile TEL_PATH: Should be `./SDK`
2. Verify SDK files:
   ```bash
   wc -l SDK/components/drivers/8258/flash.h
   # Should be 229 lines
   
   wc -l SDK/components/drivers/8258/flash.c
   # Should be 574 lines
   ```
3. If different, user has different SDK version - use SDK compatibility mode

### Issue 4: Flash Writes Not Persisting

**Symptoms:** Config writes accepted but don't persist after reboot

**Cause:** Flash not properly unlocked (wrong flash type)

**Solutions:**
1. Verify automatic flash detection is working
2. Check flash MID:
   ```c
   u8 flash_mid[3];
   flash_read_id(flash_mid);
   printf("Flash MID: 0x%02X\n", flash_mid[0]);
   // GD: 0xC8, XTX: 0x0B, PUYA: 0x85
   ```
3. Ensure correct unlock method for flash type

## Step-by-Step Debugging

### Step 1: Verify SDK Structure

```bash
echo "=== Checking SDK files ==="
wc -l SDK/components/drivers/8258/flash.{h,c}
grep -c "#if FLASH_EXTENDED_API" SDK/components/drivers/8258/flash.{h,c}
grep -c "#if 0" SDK/components/drivers/8258/flash.{h,c}
```

Expected output:
```
229 SDK/components/drivers/8258/flash.h
574 SDK/components/drivers/8258/flash.c
1 SDK/components/drivers/8258/flash.h
1 SDK/components/drivers/8258/flash.c
0 SDK/components/drivers/8258/flash.h
0 SDK/components/drivers/8258/flash.c
```

### Step 2: Verify Makefile Configuration

```bash
echo "=== Checking makefile ==="
grep "FLASH_EXTENDED_API" makefile
grep "TEL_CHIP" makefile | head -1
```

Expected:
```
TEL_CHIP := -DCHIP_TYPE=CHIP_TYPE_8258 -DFLASH_EXTENDED_API=1
```

### Step 3: Clean Build

```bash
echo "=== Clean build ==="
make clean
make 2>&1 | tee build.log
```

### Step 4: Check Compilation

```bash
echo "=== Checking if FLASH_EXTENDED_API was used ==="
grep "FLASH_EXTENDED_API" build.log
```

Should see the flag in gcc commands.

### Step 5: Verify Extended API in Binary

```bash
echo "=== Checking symbols ==="
tc32-elf-nm out/ATC_Thermometer.elf | grep flash_unlock
```

Should show flash_unlock function(s).

### Step 6: Test on Device

1. Flash firmware to device
2. Connect via BLE
3. Change config (e.g., advertising interval)
4. Power cycle device
5. Reconnect and verify config persisted

## Summary

| User's Concern | Reality | Status |
|----------------|---------|--------|
| "flash_unlock in #if 0 block" | It's in #if FLASH_EXTENDED_API | ✅ Correct |
| "api switch not in sdk" | It IS in SDK (lines 123, 254) | ✅ Present |
| "should be according to TEL_CHIP" | TEL_CHIP has -DFLASH_EXTENDED_API=1 | ✅ Configured |
| "why is it not working" | Need to debug build process | ⚠️ Debug needed |

## Conclusion

The SDK files are **correct** and **properly structured** with `#if FLASH_EXTENDED_API` (not `#if 0`).

If extended API is not working:
1. Do clean build
2. Verify flag is defined early in makefile
3. Check preprocessor output
4. Verify compilation with VERBOSE=1
5. Test on device

The infrastructure is correct - if not working, it's a build process issue, not an SDK issue.
