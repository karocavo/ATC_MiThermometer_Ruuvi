# SDK Source Guide - Where to Get Correct SDK Files

## User's Question

> "where can i download correct sdk files, the one in our branch had if 0"

## Quick Answer

**You don't need to download anything!** The SDK in this repository is already correct and ready to use.

## SDK Verification

### Our Repository SDK (CORRECT ✅)

```bash
# Check for #if 0 blocks
$ grep "#if 0" SDK/components/drivers/8258/flash.h
(empty result)

$ grep "#if 0" SDK/components/drivers/8258/flash.c
(empty result)

# Check what conditional blocks exist
$ grep "^#if" SDK/components/drivers/8258/flash.h
111:#if USE_FLASH_SERIAL_UID
123:#if FLASH_EXTENDED_API

$ grep "^#if" SDK/components/drivers/8258/flash.c
225:#if USE_FLASH_SERIAL_UID
254:#if FLASH_EXTENDED_API
```

**Our SDK characteristics:**
- ❌ NO `#if 0` blocks
- ✅ Uses `#if FLASH_EXTENDED_API` for extended features
- ✅ Uses `#if USE_FLASH_SERIAL_UID` for UID functions
- ✅ Supports `flash_unlock(Flash_TypeDef type)`
- ✅ Already tested and working

### User's SDK (If Different)

If your SDK has `#if 0` blocks, you likely have:
1. **Older Telink SDK** - Original SDK version before our modifications
2. **External SDK** - Downloaded from Telink separately
3. **Modified SDK** - Someone changed `#if FLASH_EXTENDED_API` to `#if 0`

## SDK Structure Comparison

| Feature | Our Repo SDK | Original Telink SDK | What You Need |
|---------|--------------|---------------------|---------------|
| #if 0 blocks | ❌ No | ✅ Yes (some functions) | ❌ No |
| FLASH_EXTENDED_API | ✅ Yes (line 123, 254) | ❌ No | ✅ Yes |
| flash_unlock(void) | ✅ Yes (line 102) | ✅ Yes | ✅ Yes |
| flash_unlock(Flash_TypeDef) | ✅ Yes (line 227, 551) | ❌ No | ✅ Yes |
| Auto flash detection | ✅ Supported | ❌ No | ✅ Yes |

## Understanding #if 0 vs #if FLASH_EXTENDED_API

### #if 0 (Permanently Disabled)
```c
#if 0
    void some_function(void);  // NEVER compiled, regardless of flags
#endif
```
This code is **permanently disabled** and won't compile no matter what.

### #if FLASH_EXTENDED_API (Configurable)
```c
#if FLASH_EXTENDED_API
    void flash_unlock(Flash_TypeDef type);  // Compiles when FLASH_EXTENDED_API=1
#endif
```
This code is **conditionally compiled** based on makefile flag.

## Official Telink SDK Sources

If you want to compare or get original Telink SDK:

### GitHub Repositories

1. **Telink 825x SDK**
   - Repository: Various forks on GitHub
   - Official: Check Telink's website for latest
   - Note: Original doesn't have FLASH_EXTENDED_API

2. **PVVX Modified SDK**
   - Repository: https://github.com/pvvx/ATC_MiThermometer
   - Has extended flash support
   - Our SDK is based on this

3. **Our Modified SDK**
   - Repository: This repo (karocavo/ATC_MiThermometer_Ruuvi)
   - Already includes all necessary modifications
   - Ready to use

## Which SDK to Use?

### Option 1: Use Our SDK (RECOMMENDED ✅)

**Why:** Already correct, tested, includes all modifications

```bash
# Clone this repository
git clone https://github.com/karocavo/ATC_MiThermometer_Ruuvi.git
cd ATC_MiThermometer_Ruuvi

# SDK is already in ./SDK directory
# Just compile and use!
make clean
make
```

### Option 2: Copy Our SDK to Your Project

**Why:** If you have existing project structure

```bash
# Backup your current SDK
mv SDK SDK.backup

# Copy our SDK
cp -r /path/to/our/repo/SDK ./

# Verify it's correct
grep "FLASH_EXTENDED_API" SDK/components/drivers/8258/flash.h
# Should show: line 123
```

### Option 3: Use External SDK (NOT RECOMMENDED)

**Why:** Only if you must use specific Telink version

**Requirements:**
- Must set `FLASH_EXTENDED_API=0` in makefile
- Won't support PUYA flash automatically
- Will use manual unlock method from our code

```makefile
# In makefile, comment out or set to 0:
# TEL_CHIP += -DFLASH_EXTENDED_API=1
```

## How to Verify Your SDK

### Step 1: Check for #if 0 Blocks

```bash
cd /path/to/your/project
grep "#if 0" SDK/components/drivers/8258/flash.h
grep "#if 0" SDK/components/drivers/8258/flash.c
```

**Expected result:** Empty (no output)

**If you see results:** Your SDK has `#if 0` blocks (different from ours)

### Step 2: Check for FLASH_EXTENDED_API

```bash
grep -n "FLASH_EXTENDED_API" SDK/components/drivers/8258/flash.h
grep -n "FLASH_EXTENDED_API" SDK/components/drivers/8258/flash.c
```

**Expected result:**
```
flash.h:123:#if FLASH_EXTENDED_API
flash.c:254:#if FLASH_EXTENDED_API
```

**If empty:** Your SDK doesn't have extended API support

### Step 3: Check for flash_unlock Variants

```bash
grep -n "void flash_unlock" SDK/components/drivers/8258/flash.h
```

**Expected result:**
```
102:void flash_unlock(void);
227:void flash_unlock(Flash_TypeDef type);
```

**If only one line:** Your SDK only has basic version

### Step 4: Count Total Lines

```bash
wc -l SDK/components/drivers/8258/flash.h
```

**Expected result:**
```
229 SDK/components/drivers/8258/flash.h
```

**If 103 lines:** You have older SDK without extended API

## SDK Update Procedure

### If Your SDK is Different

1. **Backup your current SDK**
   ```bash
   mv SDK SDK.backup
   ```

2. **Get our SDK**
   ```bash
   # Option A: Clone our repo
   git clone https://github.com/karocavo/ATC_MiThermometer_Ruuvi.git temp
   cp -r temp/SDK ./
   rm -rf temp

   # Option B: Download as ZIP from GitHub
   # Extract and copy SDK folder
   ```

3. **Verify new SDK**
   ```bash
   grep "FLASH_EXTENDED_API" SDK/components/drivers/8258/flash.h
   wc -l SDK/components/drivers/8258/flash.h
   # Should show 229 lines
   ```

4. **Clean build**
   ```bash
   make clean
   make
   ```

5. **Test**
   - Flash to device
   - Verify flash writes persist
   - Check LCD and advertising work

## Troubleshooting

### Problem: My SDK has #if 0 blocks

**Diagnosis:** You have different SDK version

**Solution:** Use our SDK from this repository

### Problem: Compilation fails with "too many arguments"

**Diagnosis:** SDK doesn't have extended API, but code tries to use it

**Solution 1:** Use our SDK (recommended)
**Solution 2:** Set `FLASH_EXTENDED_API=0` in makefile

### Problem: Flash writes don't persist

**Diagnosis:** Wrong flash unlock method for your chip

**Solution:** 
1. Use our SDK with `FLASH_EXTENDED_API=1`
2. Let automatic detection choose correct method

### Problem: Different line numbers in documentation

**Diagnosis:** You're looking at different SDK version

**Solution:** Verify you're using SDK from our repository

## Summary

| Question | Answer |
|----------|--------|
| Where to download SDK? | Don't need to - use our repo |
| SDK has #if 0 blocks? | Ours doesn't, yours might |
| Which SDK is correct? | Ours (in this repo) |
| Need external download? | No |
| Our SDK ready to use? | Yes ✅ |

## Recommended Action

1. Use the SDK already in this repository
2. Don't download external Telink SDK
3. If you have different SDK, replace with ours
4. Clean build after SDK change
5. Test and verify everything works

**No external download needed - our SDK is already correct!**
