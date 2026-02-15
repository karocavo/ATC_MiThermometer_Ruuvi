# Simple Instructions for Master Branch Users

## If Your SDK Has 120 Lines (Master Branch)

### Quick Check

```bash
wc -l SDK/components/drivers/8258/flash.h
```

If output shows **~120 lines**, follow these instructions.

## 4 Simple Steps

### Step 1: Pull Latest Changes

```bash
git checkout copilot/fix-flash-unlock-duplicate
git pull origin copilot/fix-flash-unlock-duplicate
```

### Step 2: Edit Makefile

Open `makefile` and find line 15:

**Change FROM:**
```makefile
TEL_CHIP := -DCHIP_TYPE=CHIP_TYPE_8258 -DFLASH_EXTENDED_API=1
```

**Change TO:**
```makefile
TEL_CHIP := -DCHIP_TYPE=CHIP_TYPE_8258
```

(Just remove `-DFLASH_EXTENDED_API=1`)

### Step 3: Clean Build

```bash
make clean
make
```

### Step 4: Flash to Device

Flash the compiled firmware to your device using your usual method.

## What This Does

Your code will automatically:

1. **Detect flash type** at boot (GD, XTX, or PUYA)
2. **Use correct unlock method:**
   - PUYA (MID 0x85): Manual 16-bit status register unlock
   - GD (MID 0xC8): Basic flash_unlock()
   - XTX (MID 0x0B): Basic flash_unlock()
3. **Flash writes persist** correctly after reboot
4. **Config changes work** via BLE

## Testing

After flashing, verify:

- ✅ Device boots normally
- ✅ LCD shows temperature/humidity
- ✅ BLE advertising works
- ✅ Version shows "V5.8"
- ✅ Manufacturer shows "DIY.Ruuvi-v58"
- ✅ Change config via BLE
- ✅ Power cycle device
- ✅ Config persists

## That's It!

No SDK download needed.
No complicated configuration.
Just edit makefile and compile.

---

## Troubleshooting

### If Compilation Fails

**Error:** "too many arguments to function 'flash_unlock'"

**Fix:** Make sure you removed `-DFLASH_EXTENDED_API=1` from makefile line 15.

### If Flash Writes Don't Persist

**Possible causes:**
1. Battery too low
2. Different flash chip type
3. Need to erase all first

**Try:**
```bash
# Use web flasher with "Erase All" option
# Then reflash
```

### Need Help?

See these documents:
- `docs/APOLOGY_AND_CLARIFICATION.md` - Explanation of SDK versions
- `docs/SDK_VERSION_COMPATIBILITY.md` - Detailed compatibility guide
- `docs/FLASH_WRITE_FIX.md` - Flash persistence details
- `docs/OTA_UPDATE_GUIDE.md` - Complete OTA guide

---

**Your master branch SDK (120 lines) is fully supported!**
