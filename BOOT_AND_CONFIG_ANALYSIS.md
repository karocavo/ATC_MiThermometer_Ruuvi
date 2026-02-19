# Boot Section, MAC Address, and Configuration Analysis

## Device Info
**Current Device**: DEVICE_LYWSD03MMC (Xiaomi LYWSD03MMC)
**Build**: clean-atc56-ruuvi branch

---

## 1. BOOT/STARTUP SEQUENCE

### Boot Screen Location
- **File**: [src/app.c](src/app.c#L754-L868) - Function `user_init_normal()`
- **Screen Display**: Line 865 - `SHOW_REBOOT_SCREEN();`
- **Condition**: ENABLED for LYWSD03MMC (line 151 of [src/lcd.h](src/lcd.h))

### 🔴 ISSUE FOUND: Blank Boot Screen Instead of "ATC + MAC"

**Root Cause**: The `show_reboot_screen()` function for LYWSD03MMC just fills the display with white (0xFF), NOT showing text.

**Location**: [src/lcd_lywsd03mmc.c:515-518](src/lcd_lywsd03mmc.c#L515-L518)
```c
void show_reboot_screen(void) {
    memset(&display_buff, 0xff, sizeof(display_buff));  // All pixels on = blank/white screen
    send_to_lcd();
}
```

The function needs to display actual "ATC" text + MAC address like in the original ATC firmware.

---

## 2. MAC ADDRESS HANDLING

### Hardcoded MAC Address Location
- **File**: [src/blt_common.c](src/blt_common.c#L85-L170) - Function `blc_initMacAddress()`
- **Flash Address**: `CFG_ADR_MAC = 0x76000` (defined in SDK)

###🔴 ISSUE: Hardcoded MAC Address (4D...37)
For LYWSD03MMC and most devices, the MAC is generated with default company ID:

**LYWSD03MMC** (Line 143-144 of blt_common.c):
```c
generateRandomNum(3, mac_read);           // Random bytes 0-2
mac_read[3] = 0x38;                       // company id: 0xA4C138
mac_read[4] = 0xC1;
mac_read[5] = 0xA4;
generateRandomNum(2, &mac_read[6]);       // Random bytes 6-7
```
Results in: **XX:XX:XX:38:C1:A4** (but stored reversed as **A4:C1:38:XX:XX:XX**)

The problem: Every device gets a DIFFERENT random MAC on first boot, but if you're seeing **4D...37** consistently, it's either:
1. Flashed as a hardcoded value somewhere
2. Read from a stored I2C device that always has the same value

### Current Flow:
1. MAC is read from flash at 0x76000
2. If flash is empty (0xFFFFFFFF), it's initialized with hardcoded company ID + random bytes
3. The MAC is stored in `mac_public[]` global variable
4. Same MAC persists on reboot due to flash storage at 0x76000

---

## 3. DEFAULT CONFIGURATION SETTINGS

### Location
[src/app.c:76](src/app.c#L76) - Constant `def_cfg`

### Current Default for LYWSD03MMC (Lines 91-101):
```c
#elif DEVICE_TYPE == DEVICE_LYWSD03MMC
    .flg2.adv_flags = true,
    .advertising_interval = 40,           // 40 * 62.5 ms = 2.5 sec
    .flg.comfort_smiley = true,
    .measure_interval = 4,                 // 4 * 2.5 sec = 10 sec measurement cycle
    .min_step_time_update_lcd = 49,        // x0.05 sec = 2.45 sec
    .hw_ver = HW_VER_LYWSD03MMC_B14,
#if (DEV_SERVICES & SERVICE_HISTORY)
    .averaging_measurements = 180,         // 180 * 10 sec = 1800 sec = 30 MINUTES (⚠️ for storage)
#endif
```

### Calculation Formula:
- **Advertising Period**: `advertising_interval * 62.5 ms` (e.g., 40 × 62.5 = 2.5 sec)
- **Measurement Period**: `measure_interval * advertising_period = 4 × 2.5 sec = 10 sec`
- **Storage Log Interval**: `averaging_measurements * measurement_period = 180 × 10 sec = 1800 sec = 30 minutes`

### Current Behavior:
- Broadcasts sensor data every **2.5 seconds**
- Reads sensor every **10 seconds** (= 4 broadcasts)
- Logs to flash every **30 minutes** (180 measurements)

---

## 4. PROPOSED FIXES FOR RUUVI BUILD

### A. Fix Boot Screen (Blank Screen Instead of "ATC + MAC")

**File**: [src/lcd_lywsd03mmc.c:515-518](src/lcd_lywsd03mmc.c#L515-L518)

**Current (Blank Screen)**:
```c
void show_reboot_screen(void) {
    memset(&display_buff, 0xff, sizeof(display_buff));  // All white/blank
    send_to_lcd();
}
```

**Fix Option 1: Show "ATC" Text**
```c
void show_reboot_screen(void) {
    // Display "ATC" text
    memset(&display_buff, 0, sizeof(display_buff));  // Clear display
    
    // Set segments for "ATC" (example segment codes)
    display_buff[0] = 0xF3;  // "A"
    display_buff[1] = 0xE3;  // "T"  
    display_buff[2] = 0xE9;  // "C"
    
    send_to_lcd();
}
```

**Fix Option 2: Show Last 2 Digits of MAC + "ATC"**
```c
void show_reboot_screen(void) {
    extern u8 mac_public[6];
    memset(&display_buff, 0, sizeof(display_buff));
    
    // Display "ATC" in first 3 segments
    display_buff[0] = 0xF3;  // "A"
    display_buff[1] = 0xE3;  // "T"
    display_buff[2] = 0xE9;  // "C"
    
    // Display last 2 MAC digits
    u8 mac_byte = mac_public[5];
    display_buff[3] = hex_to_segment(mac_byte >> 4);    // High nibble
    display_buff[4] = hex_to_segment(mac_byte & 0x0F);  // Low nibble
    
    send_to_lcd();
}
```

**⚠️ Note**: Segment codes depend on your LCD's segment layout. Check the display_numbers array in your LCD driver for actual values.

---

### B. Change MAC to Use I2C-Based Unique IDs (Optional)

**File**: [src/blt_common.c:85-170](src/blt_common.c#L85-L170) - Function `blc_initMacAddress()`

**Current**: Uses random bytes for MAC on first boot, then stores in flash at 0x76000

**Option 1: Use I2C Sensor Chip ID** (if available)
```c
void blc_initMacAddress(int flash_addr, u8 *mac_pub, u8 *mac_rand) {
    u8 mac_read[8];
    u32 * p = (u32 *) &mac_read;
    flash_read_page(flash_addr, sizeof(mac_read), mac_read);

    if (p[0] == 0xffffffff && p[1] == 0xffffffff) {
        // No MAC in flash - generate from I2C sensor or use default
        
        #ifdef USE_I2C_MAC_SOURCE
        // Try to read from I2C device (example: I2C EEPROM at 0x50)
        u8 i2c_id[6];
        if (i2c_read_bytes(0x50, 0, i2c_id, 6) == 6) {
            memcpy(mac_read, i2c_id, 6);
            mac_read[0] = 0x4D;  // Ruuvi company prefix
        } else {
            // Fallback: generate random MAC
            generateRandomNum(3, mac_read);
            mac_read[3] = 0x38;
            mac_read[4] = 0xC1;
            mac_read[5] = 0xA4;
            generateRandomNum(2, &mac_read[6]);
        }
        #else
        // Default: Standard random generation
        generateRandomNum(3, mac_read);
        mac_read[3] = 0x38;
        mac_read[4] = 0xC1;
        mac_read[5] = 0xA4;
        generateRandomNum(2, &mac_read[6]);
        #endif
        
        flash_write_page(flash_addr, sizeof(mac_read), mac_read);
    }
    
    // copy public address
    memcpy(mac_pub, mac_read, 6);
    // ... rest of function
}
```

**Option 2: Use Fixed Ruuvi Company ID (0x4D)**
```c
// In blt_common.c, line ~143 - change from:
mac_read[3] = 0x38;  // company id: 0xA4C138
// To:
mac_read[3] = 0x4D;  // Ruuvi company prefix
```

---

### C. Change Advertisement & Measurement Intervals to YOUR SPEC

**File**: [src/app.c:76-144](src/app.c#L76-L144) - Update for LYWSD03MMC section

**Current** (Lines 91-101):
```c
#elif DEVICE_TYPE == DEVICE_LYWSD03MMC
    .flg2.adv_flags = true,
    .advertising_interval = 40,           // 2.5 sec
    .measure_interval = 4,                 // 10 sec
    .averaging_measurements = 180,         // 30 min log
```

**Change To** (20 sec broadcast + 4×20 sec = 80 sec measurements + 20 min log):
```c
#elif DEVICE_TYPE == DEVICE_LYWSD03MMC
    .flg2.adv_flags = true,
    .advertising_interval = 320,          // 320 * 62.5ms = 20 sec
    .measure_interval = 4,                 // 4 * 20sec = 80 sec measurement cycle
    .averaging_measurements = 15,          // 15 * 80 sec = 1200 sec = 20 MINUTES (log interval)
```

### Calculation Check:
- Broadcast: 320 × 62.5 ms = 20 sec ✓
- Measurement: 4 × 20 sec = 80 sec ✓  
- Log interval: 15 × 80 sec = 1200 sec = 20 min ✓

---

### D. Set Ruuvi as Default Beacon Type

**File**: [src/app.c:76](src/app.c#L76)

**Current** (line 82):
```c
.flg.advertising_type = ADV_TYPE_DEFAULT,
```

**Change To** (to set Ruuvi RAWv2 as default):
```c
.flg.advertising_type = ADV_TYPE_PVVX,  // Ruuvi RAWv2
```

---

## 5. QUICK IMPLEMENTATION CHECKLIST

### ✅ Recommended Fixes in Order:

**Priority 1: Fix Boot Screen (Most Visible)**
- [ ] Modify `show_reboot_screen()` in [src/lcd_lywsd03mmc.c:515-518](src/lcd_lywsd03mmc.c#L515-L518)
- [ ] Display "ATC" or "ATC" + last 2 MAC digits

**Priority 2: Change Intervals to Your Spec**
- [ ] Update LYWSD03MMC defaults in [src/app.c:91-101](src/app.c#L91-L101)
- [ ] Set: `advertising_interval = 320`, `averaging_measurements = 15`

**Priority 3: Set Ruuvi as Default**
- [ ] Change `flg.advertising_type` in [src/app.c:82](src/app.c#L82) to `ADV_TYPE_PVVX`

**Priority 4: Fix MAC Address (If Needed)**
- [ ] Check if 4D...37 is stored in flash at 0x76000
- [ ] Implement I2C-based MAC reading if available
- [ ] Or just use standard Ruuvi company prefix (0x4D)

---

## Summary of Changes

| Issue | Location | Fix | Difficulty |
|-------|----------|-----|-----------|
| **Blank boot screen** | [lcd_lywsd03mmc.c:515](src/lcd_lywsd03mmc.c#L515) | Display "ATC" text on boot | ⭐ Easy |
| **Advertisement interval** | [app.c:93](src/app.c#L93) | Change 40 → 320 | ⭐ Easy |
| **Temperature log interval** | [app.c:100](src/app.c#L100) | Change 180 → 15 | ⭐ Easy |
| **Set Ruuvi as default** | [app.c:82](src/app.c#L82) | Change to ADV_TYPE_PVVX | ⭐ Easy |
| **Hardcoded MAC** | [blt_common.c:143](src/blt_common.c#L143) | Change 0x38 → 0x4D (if needed) | ⭐ Easy |
| **Dynamic MAC from I2C** | [blt_common.c:85](src/blt_common.c#L85) | Add I2C read logic | ⭐⭐ Medium |

---

## Current Settings vs. Your Requirements

| Setting | Current | Your Spec | Status |
|---------|---------|-----------|--------|
| **Broadcast Interval** | 2.5 sec (40 × 62.5ms) | 20 sec | ❌ NEEDS CHANGE |
| **Measurement Interval** | 10 sec (4 × 2.5sec) | 80 sec | ❌ NEEDS CHANGE |
| **Storage Log Interval** | 30 min (180 × 10sec) | 20 min | ❌ NEEDS CHANGE |
| **Boot Screen** | Blank (0xFF all pixels) | ATC + MAC | ❌ NEEDS IMPLEMENTATION |
| **Beacon Type** | MI (default) | Ruuvi RAWv2 | ❌ NEEDS CHANGE |
| **MAC Address** | Random/0xA4C138 | Ruuvi 0x4D | ❌ NEEDS CHANGE |
