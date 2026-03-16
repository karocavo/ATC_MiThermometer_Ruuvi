# Master TODO - Victron + Ruuvi + DS18B20 Integration

Date: 2026-03-16
Branch target: master
Device goal: Boiler water control via Victron Cerbo GX / Venus OS using Ruuvi-style pairing.

## 1. Product Decision (must lock first)

- [ ] Decide telemetry mode:
  - [ ] Mode A: one Ruuvi identity only, switch payload source to DS18B20 when probe is connected.
  - [ ] Mode B: two Ruuvi identities (original + virtual second tag) for separate room/boiler channels.
- [ ] Default recommendation for reliability: start with Mode A, then add Mode B behind compile-time flag.
- [ ] Define Victron mapping:
  - [ ] Ruuvi A = original internal sensor channel.
  - [ ] Ruuvi B = external DS18B20 channel (virtual/fake tag).

## 2. Hardware Integration (DS18B20)

- [ ] Choose connector style (mini plug) and board pad routing.
- [ ] Use one-wire pad from config mapping (ONEWIRE1 first).
- [ ] Add 4.7k pull-up to VCC near MCU.
- [ ] Validate cable length/noise on immersed probe.
- [ ] Define disconnected state detection (open line, no ROM, CRC fail).

## 3. Firmware - DS18B20 Bring-up

- [ ] Enable DS18B20 build config for target hardware in app config.
- [ ] Confirm task scheduler calls read path every cycle (already present, verify timing).
- [ ] Add low-power sampling policy for external water probe:
  - [ ] DS18B20 conversion/read every 120 seconds (default for boiler use).
  - [ ] Keep last valid DS18B20 sample and timestamp between reads.
  - [ ] Add optional temporary fast mode (10-30 seconds) for commissioning/debug only.
- [ ] Add robust probe-present state machine:
  - [ ] present
  - [ ] conversion pending
  - [ ] valid sample
  - [ ] fault/disconnected
- [ ] Add hysteresis/limits for boiler safety use (sanity clamp, stale timeout).

## 4. Firmware - Victron Advertising Strategy

### Mode A (single identity)

- [ ] If DS18B20 connected: report external temp as primary advertised temp.
- [ ] If DS18B20 disconnected: fallback to internal sensor.
- [ ] Add flag bit in payload or metadata marker for source (internal/external) if possible.
- [ ] Decouple advertising from DS18B20 sampling:
  - [ ] Continue BLE advertising every 10 seconds using latest stored DS18B20 value.
  - [ ] Mark stale data if DS18B20 sample age exceeds threshold.

### Mode B (dual identity, virtual second tag)

- [ ] Create 5-second scheduler slotting:
  - [ ] t=0s: advertise A
  - [ ] t=5s: advertise B
  - [ ] repeat 10-second period per identity
- [ ] Keep independent counters for A/B payload freshness.
- [ ] Ensure stable identity behavior for Victron pairing (address/identifier consistency).
- [ ] Gate this mode with compile-time flag until long-run tested.
- [ ] Keep DS18B20 sampling at 120 seconds even in A/B mode; do not increase probe conversion rate just for BLE cadence.

## 5. LCD / UX

- [ ] Add LCD flag for external probe visibility.
- [ ] Extend display cycle to include external probe temp stage when probe valid.
- [ ] Keep existing clock/date cycle intact.
- [ ] Add simple on-screen indicator:
  - [ ] INT (internal)
  - [ ] EXT (dallas)
  - [ ] ERR (probe fault)

## 6. Optional: Fake Water Level Channel

- [ ] Decide source model:
  - [ ] derived value from temperature delta (not recommended for real level)
  - [ ] dedicated analog/digital input (recommended)
- [ ] If implemented, expose as separate virtual sensor channel for Cerbo GX.
- [ ] Mark clearly as virtual/estimated to avoid control hazards.

## 7. Safety / Control Considerations (boiler)

- [ ] Add stale-data timeout (do not control from old reading).
- [ ] Add plausible range checks for water probe.
- [ ] Add fallback behavior on probe error (safe default output).
- [ ] Add anti-flap filtering (minimum stable period before control decisions).
- [ ] Add control-specific freshness rule for slow thermal systems:
  - [ ] Default stale timeout for control logic: 300 seconds.
  - [ ] Warning state if data age > 180 seconds.

## 8. Testing Plan

- [ ] Bench test DS18B20 connect/disconnect during runtime.
- [ ] Validate CRC failure handling and automatic recovery.
- [ ] Verify Victron sees and pairs tag(s) after reboot.
- [ ] Verify 10-second update rate with 5-second A/B offset in dual mode.
- [ ] 24-hour endurance run (no lockups, no identity loss).
- [ ] Check battery impact of 5-second alternating advertisements.
- [ ] Verify DS18B20 read interval remains 120 seconds under all advertising modes.
- [ ] Measure average current with 120-second DS18B20 policy enabled.

## 9. Release Plan

- [ ] Stage 1 release: Mode A only + LCD indicator + probe fallback.
- [ ] Stage 2 release: Mode B virtual second tag behind feature flag.
- [ ] Stage 3 release: optional water-level channel after hardware validation.

## 10. Crash-Safe Resume Checklist

Use these first after reopening workspace:

- [ ] `git status -sb`
- [ ] `git branch --show-current`
- [ ] `git log --oneline --max-count=8`
- [ ] `make` (or VS Code Build task)
- [ ] Save notes in this file under Progress Notes before ending session.

## Progress Notes

- 2026-03-16: Branch tracking fixed for karocavo-ruuvi-rawv2. Workspace currently clean.
- 2026-03-16: Ruuvi time/date mod confirmed present on master history path.
