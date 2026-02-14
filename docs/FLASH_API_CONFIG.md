# Flash API Configuration

## Overview

The SDK's flash driver (`SDK/components/drivers/8258/flash.h` and `flash.c`) includes both basic and extended API functions. However, different SDK versions may have different defaults.

## Issue

The SDK files include two versions of `flash_unlock`:
- `void flash_unlock(void)` - Basic version
- `void flash_unlock(Flash_TypeDef type)` - Extended version with flash type parameter

Having both declarations would cause C compilation errors since C doesn't support function overloading.

## SDK Version Differences

### Local SDK (in repository)
- Uses `FLASH_EXTENDED_API` flag to control which version is available
- Set to 0 by default in makefile → basic API active

### External Telink SDK
- May be located at `C:/telink_sdk/telink-825x-sdk/` or similar
- May have extended API as the default or only version
- Cannot be modified per project requirements

## Solution

Our code uses conditional compilation to work with **both SDK versions**:

```c
#if defined(FLASH_EXTENDED_API) && (FLASH_EXTENDED_API == 0)
    flash_unlock(); // Basic API
#else
    flash_unlock(FLASH_TYPE_GD); // Extended API with default flash type
#endif
```

### Build Configuration

In `makefile`, we set:
```makefile
GCC_FLAGS += -DFLASH_EXTENDED_API=0
```

This flag is used in both `flash.h` and `flash.c`:
```c
/* according to your appliaction */
#if FLASH_EXTENDED_API
// Extended API functions including flash_unlock(Flash_TypeDef type)
// are only compiled if FLASH_EXTENDED_API is enabled
#endif
```

### How It Works

1. **With local SDK** (`FLASH_EXTENDED_API=0`):
   - Calls `flash_unlock()` without parameters
   - Only basic API is compiled in SDK

2. **With external Telink SDK** (no `FLASH_EXTENDED_API` or different version):
   - Calls `flash_unlock(FLASH_TYPE_GD)`
   - Works with SDK that has extended API as default
   - Uses `FLASH_TYPE_GD` (value 0) as safe default

## Flash Types

```c
typedef enum {
    FLASH_TYPE_GD = 0,   // Default, safe for most cases
    FLASH_TYPE_XTX,
    FLASH_TYPE_PUYA
} Flash_TypeDef;
```

## Files Modified

1. **makefile** - Added `-DFLASH_EXTENDED_API=0` to `GCC_FLAGS`
2. **SDK/components/drivers/8258/flash.h** - Changed `#if 0` to `#if FLASH_EXTENDED_API`
3. **SDK/components/drivers/8258/flash.c** - Changed `#if 0` to `#if FLASH_EXTENDED_API`
4. **src/app.c** - Added conditional call to `flash_unlock`
5. **src/ext_ota.c** - Added conditional call to `flash_unlock`

## Benefits

- ✅ Works with both local and external Telink SDK versions
- ✅ No modification needed to external SDK files
- ✅ Explicit and self-documenting configuration
- ✅ Easy to maintain and update
- ✅ No compilation errors
- ✅ Compatible with different SDK releases
