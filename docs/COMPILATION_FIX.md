# Compilation Fix: FLASH_EXTENDED_API

## Problem

Compilation failed with error:

```
src/app.c:775:2: error: too many arguments to function 'flash_unlock'
./SDK/components/drivers/8258/flash.h:102:6: note: declared here
```

## Root Cause

The `FLASH_EXTENDED_API=1` preprocessor flag was defined too late in the makefile:

```makefile
# Line 84 - TOO LATE!
GCC_FLAGS += -DFLASH_EXTENDED_API=1
```

**Why this caused failure:**

1. SDK headers (flash.h) are included early during compilation
2. When flash.h is parsed, `FLASH_EXTENDED_API` is not yet defined
3. The `#if FLASH_EXTENDED_API` block at line 123 evaluates to false (0)
4. Extended API block is excluded from compilation
5. Only basic version visible: `void flash_unlock(void);` (line 102)
6. Our code calls: `flash_unlock(flash_type)` with parameter
7. **Compiler error:** Too many arguments!

## Solution

Move the flag definition to `TEL_CHIP` at the top of makefile:

```makefile
# Line 15 - EARLY DEFINITION
TEL_CHIP := -DCHIP_TYPE=CHIP_TYPE_8258 -DFLASH_EXTENDED_API=1
```

**Why this works:**

1. `TEL_CHIP` defined at line 15 (very early)
2. `TEL_CHIP` added to `GCC_FLAGS` at line 88
3. Flag is available when SDK headers are parsed
4. `#if FLASH_EXTENDED_API` evaluates to true (1)
5. Extended API block is compiled
6. Parameterized version available: `void flash_unlock(Flash_TypeDef type);`
7. Our code compiles correctly ✅

## Makefile Changes

### Before (Broken)
```makefile
TEL_CHIP := -DCHIP_TYPE=CHIP_TYPE_8258

# ...much later at line 84...

GCC_FLAGS += -DFLASH_EXTENDED_API=1
```

### After (Fixed)
```makefile
TEL_CHIP := -DCHIP_TYPE=CHIP_TYPE_8258 -DFLASH_EXTENDED_API=1

# No separate GCC_FLAGS line needed
```

## Important Lessons

### Preprocessor Flag Timing

Preprocessor flags must be defined **before** the headers that use them are included!

**Wrong:**
```makefile
# Define flag late
GCC_FLAGS += -DSOME_FLAG=1

# Headers already processed by this point
```

**Correct:**
```makefile
# Define flag early
TEL_CHIP := ... -DSOME_FLAG=1

# Flag available for all compilation
```

### TEL_CHIP vs GCC_FLAGS

**TEL_CHIP:**
- Defined at top of makefile
- Chip-specific and fundamental flags
- Available for all compilation units
- Best for SDK-level flags

**GCC_FLAGS:**
- Can be extended with `+=`
- Good for compiler options
- May be too late for SDK header flags

### SDK Integration

When working with SDK code:
1. Understand SDK's conditional compilation
2. Define flags early
3. Test compilation thoroughly
4. Check what preprocessor sees

## Testing

After this fix:

```bash
make clean
make
```

Should compile without errors!

## Related Files

- `makefile` - Flag definition moved to line 15
- `SDK/components/drivers/8258/flash.h` - Conditional API at line 123
- `src/app.c` - Uses `flash_unlock(flash_type)` at line 775
- `src/ext_ota.c` - Uses `flash_unlock(flash_type)` at line 88

## Summary

**Problem:** Flag defined too late → extended API not compiled → compilation error

**Solution:** Define flag early in TEL_CHIP → extended API compiles → success!

This is a common makefile issue when integrating with SDKs that use conditional compilation.
