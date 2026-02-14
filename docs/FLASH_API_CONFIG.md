# Flash API Configuration

## Overview

The SDK's flash driver (`SDK/components/drivers/8258/flash.h` and `flash.c`) includes both basic and extended API functions. However, the current SDK version only supports the basic API.

## Issue

The SDK files include two versions of `flash_unlock`:
- `void flash_unlock(void)` - Basic version, **supported and used**
- `void flash_unlock(Flash_TypeDef type)` - Extended version, **not supported**

Having both declarations would cause C compilation errors since C doesn't support function overloading.

## Solution

Instead of modifying the SDK files (which would make future SDK updates difficult), we use a preprocessor flag to control which functions are compiled:

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

### Current Configuration

- **FLASH_EXTENDED_API = 0** (disabled)
  - Only basic functions are compiled
  - `flash_unlock(void)` is available
  - `flash_unlock(Flash_TypeDef type)` is excluded

### Future Use

If you need the extended API functions:
1. Change `FLASH_EXTENDED_API=0` to `FLASH_EXTENDED_API=1` in the makefile
2. Ensure your SDK version supports the extended functions
3. Update your code to use the appropriate function variant

## Files Modified

1. **makefile** - Added `-DFLASH_EXTENDED_API=0` to `GCC_FLAGS`
2. **SDK/components/drivers/8258/flash.h** - Changed `#if 0` to `#if FLASH_EXTENDED_API`
3. **SDK/components/drivers/8258/flash.c** - Changed `#if 0` to `#if FLASH_EXTENDED_API`

## Benefits

- ✅ Preserves SDK files for future updates
- ✅ Explicit and self-documenting configuration
- ✅ Easy to enable extended API if needed
- ✅ No compilation errors
- ✅ Compatible with current SDK version
