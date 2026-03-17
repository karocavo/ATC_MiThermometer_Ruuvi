# Ruuvi Current Behavior (LYWSD03MMC + MJWSD06MMC)

This note describes the current custom behavior in this workspace for the Ruuvi-oriented builds on LYWSD03MMC and MJWSD06MMC.

## Firmware identity

- Firmware version is `V5.8` (`VERSION = 0x58`).
- Build target is selected by make flag (for example `-DDEVICE_TYPE=DEVICE_LYWSD03MMC` or `-DDEVICE_TYPE=DEVICE_MJWSD06MMC`).
- Current default timing for this workspace is:
  - Advertising interval: 10 seconds
  - Measurement interval: 40 seconds
  - LCD stage update threshold: 5 seconds, with practical stage changes following the wake cadence

## Display behavior

### LYWSD03MMC normal behavior

- On boot, the firmware clears any stale external LCD override state.
- After boot, clock cycling follows config (`cfg.flg.show_time_smile`) instead of being force-enabled.
- If UTC time is not valid yet, the LCD stays on temperature and humidity only.

### `30-10-10` cycle

When enabled and time is valid, the LYWSD03MMC display uses this cycle:

- 30 seconds: temperature and humidity
- 10 seconds: local time
- 10 seconds: local date

The cycle is controlled at runtime by `lcd_flg.show_clock_after_disconnect`, with boot default now synced from `show_time_smile`.

### MJWSD06MMC normal behavior

- `POWERUP_SCREEN` is enabled.
- Boot screen shows Ruuvi branding + MAC sequence:
  - big digits show `r u u`
  - small digits show MAC byte hex pairs (cycling low 3 MAC bytes)
- MJ6 LCD send path uses correct compare-buffer layout (`display_cmp_buff[0]` header, payload from `[1]`).
- MJ6 clock/date helpers are present and use the same UTC/tz/DST logic as LYWSD03 path.

### Clock OFF vs Clock ON

- Clock OFF behavior: temperature and humidity only.
- Clock ON behavior for this custom build: the runtime `30-10-10` cycle above.

## WebUI command behavior

### Set device time

- The WebUI time payload is treated as host local wall-clock time.
- Firmware converts that incoming value to internal UTC using:
  - `cfg.tz_offset`
  - EU DST rules
- This keeps internal time consistent while still matching the user-visible local clock.

### `Show clock` / `Send clock data on LCD`

- In this build, that action is repurposed for device-clock workflow on LYWSD03MMC.
- While connected, it forces the device to show its own local clock stage immediately.
- It also arms the post-disconnect `30-10-10` cycle.
- Host raw LCD digits from the WebUI clock test are ignored in this mode, so the LCD does not keep showing PC time after disconnect.

### `Repair LCD`

- `Repair LCD` explicitly disables the runtime `30-10-10` override.
- It also clears the persistent legacy clock-cycle flag if it was set.
- After disconnect, the device returns to stable temperature/humidity display only.

## Time and date handling

- LCD time/date rendering now uses RTC conversion helpers instead of fragile manual date math in the display path.
- Display time prefers the running software UTC clock and falls back to RTC when the runtime clock is not initialized.
- EU DST activation is recalculated from the current date and reflected in the display path.
- Date mode shows month/day on the main digits and year on the small digits, with a DST indicator segment when active.
- Invalid epoch states such as `01.01.1970` are avoided by gating the clock/date cycle on valid UTC.

## BLE and boot stability

- The earlier risky boot-time `go_sleep(...)` path was replaced with `pm_wait_ms(500)`.
- This avoids disturbing BLE initialization and improves connectability after boot.
- External LCD override flags are reset during initialization so stale connected-session display state does not survive into normal operation.

## Main code areas changed

- `src/app.c`
  - safer boot wait
  - LCD override reset on boot
  - runtime clock-cycle boot default follows config flag
- `src/app_config.h`
  - firmware version bump to `V5.8`
- `src/cmd_parser.c`
  - WebUI time normalization to UTC
  - `Show clock` and `Repair LCD` runtime handling
- `src/lcd.c`
  - LYWSD03MMC runtime `30-10-10` logic
  - temp/humidity fallback when clock mode is disabled or time is invalid
- `src/lcd.h`
  - runtime LCD state field `show_clock_after_disconnect`
- `src/lcd_lywsd03mmc.c`
  - local time/date display helpers
  - DST-aware rendering using RTC conversion helpers
- `src/lcd_mjwsd06mmc.c`
  - MJ6 boot-screen `ruu` + MAC sequence
  - MJ6 buffer/send fixes for LCD payload transmission
  - MJ6 local time/date display helpers

## Practical result

The current workspace behavior is aimed at this workflow:

- LYWSD03 fresh boot with clock enabled and valid time: `30-10-10`
- Fresh boot without valid time: temp/humidity only
- Set time in WebUI: device shows correct local clock/date behavior
- Use `Show clock`: device clock appears immediately and `30-10-10` remains active after disconnect
- Use `Repair LCD`: runtime clock cycle is disabled and temp/humidity-only mode persists after disconnect
- MJ6 fresh boot: `ruu` + MAC sequence, then normal measurement display