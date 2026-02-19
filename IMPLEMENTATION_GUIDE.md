# Ruuvi Implementation Guide - Code Changes

This guide provides step-by-step code changes to fix the boot screen, update intervals, and set Ruuvi as default.

---

## CHANGE 1: Fix Boot Screen (Show "ATC")

**File**: `src/lcd_lywsd03mmc.c`  
**Location**: Lines 514-518  
**Current**: Blank screen (0xFF = all pixels on)

### Current Code:
```c
// #define SHOW_REBOOT_SCREEN()
void show_reboot_screen(void) {
	memset(&display_buff, 0xff, sizeof(display_buff));
	send_to_lcd();
}
```

### New Code (Simple ATC text):
```c
// #define SHOW_REBOOT_SCREEN()
void show_reboot_screen(void) {
	memset(&display_buff, 0, sizeof(display_buff));
	
	// Display "ATC" 
	// Note: These values depend on your LCD's segment layout
	// Replace with actual segment values for A, T, C from your display_* arrays
	display_buff[0] = 0xA3;  // "A" - adjust based on your LCD driver
	display_buff[1] = 0x86;  // "T" - adjust based on your LCD driver  
	display_buff[2] = 0x89;  // "C" - adjust based on your LCD driver
	
	send_to_lcd();
}
```

### Alternative: Show "ATC" + Last 2 MAC Hex Digits:
```c
void show_reboot_screen(void) {
	extern u8 mac_public[6];
	extern const u8 display_numbers[]; // Using existing display array
	
	memset(&display_buff, 0, sizeof(display_buff));
	
	// Display "ATC"
	display_buff[0] = 0xA3;  // "A"
	display_buff[1] = 0x86;  // "T"
	display_buff[2] = 0x89;  // "C"
	
	// Display last MAC byte as hex (e.g., 0x37 -> "37")
	u8 mac_last = mac_public[5];
	display_buff[3] = display_numbers[(mac_last >> 4)];     // High nibble
	display_buff[4] = display_numbers[(mac_last & 0x0F)];   // Low nibble
	
	send_to_lcd();
}
```

**Note**: You may need to adjust 0xA3, 0x86, 0x89 values based on your actual LCD segment mapping. Look for character defines in the file like `display_numbers[]` array to find correct segment codes.

---

## CHANGE 2: Update Advertising & Measurement Intervals

**File**: `src/app.c`  
**Location**: Lines 91-101 (LYWSD03MMC section of `def_cfg`)  
**Current**: 40 (2.5 sec), measure_interval = 4 (10 sec), averaging = 180 (30 min)

### Current Code:
```c
#elif DEVICE_TYPE == DEVICE_LYWSD03MMC
	.flg2.adv_flags = true,
	.advertising_interval = 40, // multiply by 62.5 ms = 2.5 sec
	.flg.comfort_smiley = true,
	.measure_interval = 4, // * advertising_interval = 10 sec
	.min_step_time_update_lcd = 49, //x0.05 sec,   2.45 sec
	.hw_ver = HW_VER_LYWSD03MMC_B14,
#if (DEV_SERVICES & SERVICE_HISTORY)
	.averaging_measurements = 180, // * measure_interval = 10 * 180 = 1800 sec = 30 minutes
#endif
```

### New Code (20 sec broadcasts, 80 sec measurements, 20 min logging):
```c
#elif DEVICE_TYPE == DEVICE_LYWSD03MMC
	.flg2.adv_flags = true,
	.advertising_interval = 320,  // 320 * 62.5 ms = 20 sec broadcasts
	.flg.comfort_smiley = true,
	.measure_interval = 4,         // 4 * 20 sec = 80 sec between measurements
	.min_step_time_update_lcd = 49, //x0.05 sec,   2.45 sec
	.hw_ver = HW_VER_LYWSD03MMC_B14,
#if (DEV_SERVICES & SERVICE_HISTORY)
	.averaging_measurements = 15,  // 15 * 80 sec = 1200 sec = 20 minutes (storage log)
#endif
```

**Verification**:
- Broadcast period: 320 × 62.5 ms = 20,000 ms = 20 sec ✓
- Measurement period: 4 × 20 sec = 80 sec ✓
- Storage log: 15 × 80 sec = 1,200 sec = 20 min ✓

---

## CHANGE 3: Set Ruuvi RAWv2 as Default Beacon Type

**File**: `src/app.c`  
**Location**: Line 82 (in `def_cfg` definition, first field)  
**Current**: `ADV_TYPE_DEFAULT`

### Current Code:
```c
const cfg_t def_cfg = {
	.flg.temp_F_or_C = false,
	.flg.comfort_smiley = true,
	.flg.lp_measures = true,
	.flg.advertising_type = ADV_TYPE_DEFAULT,
	.rf_tx_power = RF_POWER_P0p04dBm,
```

### New Code (Ruuvi as default):
```c
const cfg_t def_cfg = {
	.flg.temp_F_or_C = false,
	.flg.comfort_smiley = true,
	.flg.lp_measures = true,
	.flg.advertising_type = ADV_TYPE_PVVX,  // Changed to Ruuvi RAWv2
	.rf_tx_power = RF_POWER_P0p04dBm,
```

**Reference**: Look at [src/app.h:33-36](src/app.h#L33-L36) for beacon type enum:
- ADV_TYPE_DEFAULT = original ATC
- ADV_TYPE_PVVX = Ruuvi RAWv2
- ADV_TYPE_MI = Xiaomi Mi
- ADV_TYPE_CUSTOM = BTHome

---

## CHANGE 4: Update MAC Company ID to Ruuvi (0x4D) - OPTIONAL

**File**: `src/blt_common.c`  
**Location**: Lines 143-145 (in `blc_initMacAddress()` for non-CGDK2/CGG1 devices)  
**Current**: Company ID = 0xA4C138

### Current Code:
```c
	} else {
		generateRandomNum(3, mac_read);
		mac_read[3] = 0x38;             //company id: 0xA4C138
		mac_read[4] = 0xC1;
		mac_read[5] = 0xA4;
		generateRandomNum(2, &mac_read[6]);
		flash_write_page(flash_addr, sizeof(mac_read), mac_read);
	}
```

### Option A: Use Ruuvi OUI (0x4D)
```c
	} else {
		generateRandomNum(3, mac_read);
		mac_read[3] = 0x4D;             // Ruuvi company prefix
		mac_read[4] = 0x55;             // Ruuvi middle byte
		mac_read[5] = 0x55;             // Ruuvi last company byte  
		generateRandomNum(2, &mac_read[6]);
		flash_write_page(flash_addr, sizeof(mac_read), mac_read);
	}
```

**Note**: This creates MAC like XX:XX:XX:55:55:4D (reversed due to byte order)

### Option B: Use Different Ruuvi Format
If you want MAC starting with 4D instead:
```c
	} else {
		generateRandomNum(0, mac_read);      // Random for byte 0
		generateRandomNum(0, &mac_read[1]);  // Random for byte 1
		generateRandomNum(0, &mac_read[2]);  // Random for byte 2
		mac_read[3] = 0x4D;                  // Ruuvi company prefix
		generateRandomNum(2, &mac_read[4]);  // Random last 2 bytes
		flash_write_page(flash_addr, sizeof(mac_read), mac_read);
	}
```

---

## CHANGE 5: Use I2C Unique ID for MAC (ADVANCED - Optional)

**File**: `src/blt_common.c`  
**Location**: Lines 101-150 (in `blc_initMacAddress()` function)  
**Prerequisite**: You need to know the I2C address of your ID source

### New Enhanced Function:
```c
__attribute__((optimize("-Os")))
void blc_initMacAddress_with_i2c(int flash_addr, u8 *mac_pub, u8 *mac_rand) {
	u8 mac_read[8];
	u32 * p = (u32 *) &mac_read;
	flash_read_page(flash_addr, sizeof(mac_read), mac_read);

	if (p[0] == 0xffffffff && p[1] == 0xffffffff) {
		// Flash is empty - try I2C first, then random fallback
		
		// Try to read from I2C device at address 0x50 (example EEPROM)
		u8 i2c_data[6];
		int i2c_result = 0;
		
		#ifdef USE_I2C_MAC_SOURCE
		init_i2c();  // Make sure I2C is initialized
		i2c_result = send_i2c_buf(0x50, 0, 0, i2c_data, 6);  // Read 6 bytes from I2C
		#endif
		
		if (i2c_result == 6) {
			// Successfully read from I2C - use as MAC base
			memcpy(mac_read, i2c_data, 6);
		} else {
			// Fall back to random with Ruuvi prefix
			generateRandomNum(3, mac_read);
			mac_read[3] = 0x4D;       // Ruuvi company prefix
			mac_read[4] = 0x55;
			mac_read[5] = 0x55;
			generateRandomNum(2, &mac_read[6]);
		}
		
		flash_write_page(flash_addr, sizeof(mac_read), mac_read);
	}
	
	// copy public address
	memcpy(mac_pub, mac_read, 6);
	// copy random address
	mac_rand[0] = mac_pub[0];
	mac_rand[1] = mac_pub[1];
	mac_rand[2] = mac_pub[2];
	mac_rand[3] = mac_read[6];
	mac_rand[4] = mac_read[7];
	mac_rand[5] = 0xC0;  // for random static
}
```

Then in your code calling the function:
```c
// In ble.c around line 624, change from:
blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);

// To:
blc_initMacAddress_with_i2c(CFG_ADR_MAC, mac_public, mac_random_static);
```

---

## Testing Your Changes

After applying these changes, **REBUILD**:

```bash
make clean
make
```

### Verify Each Change:

1. **Boot Screen**: Insert battery - should now show "ATC" for ~5 seconds
2. **Broadcast Interval**: Monitor BLE advertisements - should be 20 sec apart (instead of 2.5 sec)
3. **Ruuvi Beacon**: Use Ruuvi Station app - should now recognize as Ruuvi beacon (vs Mi/ATC)
4. **MAC Address**: Check BLE advertisement - should show new MAC format if I2C changes applied

---

## Troubleshooting

### Boot Screen Still Blank?
- Verify segment codes (0xA3, 0x86, 0x89) match your LCD driver
- Check that `POWERUP_SCREEN = 1` in [src/lcd.h:151](src/lcd.h#L151)
- Ensure `init_lcd()` completes before `SHOW_REBOOT_SCREEN()`

### Intervals Not Changing?
- Confirm makefile defines `DEVICE_TYPE=DEVICE_LYWSD03MMC`
- Check you modified the CORRECT section in def_cfg (not another device type)
- RAM config may override flash config - check UI settings

### MAC Still Old Format?
- Flash at 0x76000 may be cached - do full chip erase
- MAC is stored on first boot - subsequent boots use stored value
- To force regeneration: erase sector at 0x76000 in flash or update `blc_initMacAddress()`

---

## Build & Flash

```bash
# Clean rebuild
make clean
make

# Flash to device (adjust port as needed)
python3 TlsrPgm.py -p /dev/ttyUSB0 out/ATC_Thermometer.bin
```

