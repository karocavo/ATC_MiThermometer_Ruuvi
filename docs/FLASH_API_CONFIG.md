# Flash API Configuration

## Overview

The SDK's flash driver (`SDK/components/drivers/8258/flash.h` and `flash.c`) may include both basic and extended API functions. This project uses the basic API only.

## SDK Files

The local SDK files in this repository contain:
- **Basic API**: `void flash_unlock(void)` - Always available
- **Extended API**: `void flash_unlock(Flash_TypeDef type)` - Conditionally compiled with `#if FLASH_EXTENDED_API`

## Application Code

The application code (`src/app.c`, `src/ext_ota.c`) simply calls:
```c
flash_unlock();
```

This works because:
1. When `FLASH_EXTENDED_API` is not defined (default), it evaluates to 0
2. The `#if FLASH_EXTENDED_API` blocks in SDK files are excluded
3. Only the basic `flash_unlock(void)` function is compiled
4. The application's call to `flash_unlock()` matches perfectly

## No Configuration Needed

**Important**: Do NOT define `FLASH_EXTENDED_API` in the makefile or anywhere else. Leave it undefined for the basic API to work correctly.

## Safety

This approach is safe because:
- ✅ No conditional compilation in application code
- ✅ No makefile flags to configure
- ✅ Works exactly like the original code
- ✅ SDK files remain untouched from user perspective
- ✅ Cannot accidentally call wrong function version

## Troubleshooting

If you get a compilation error about `flash_unlock`, it means your SDK has a different version:

### Error: "too few arguments to function 'flash_unlock'"

Your SDK only has `flash_unlock(Flash_TypeDef type)`. 

**Solution**: In `src/app.c` and `src/ext_ota.c`, change:
```c
flash_unlock();
```

to:
```c
flash_unlock(FLASH_TYPE_GD);  // Use GD flash type (value 0)
```

### Error: "too many arguments to function 'flash_unlock'"

This shouldn't happen with the current code, but if it does, your SDK only has `flash_unlock(void)` which is what we're already calling.

## History

**Previous Approach (REMOVED)**:  
We previously used conditional compilation with `FLASH_EXTENDED_API` flags. This was removed after a user reported their device became completely non-functional (dead LCD, device bricked) after flashing. The simpler approach is much safer.
