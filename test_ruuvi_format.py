#!/usr/bin/env python3
"""
Test script to validate Ruuvi RAWv2 beacon packet structure and calculations
"""

import struct

def test_ruuvi_beacon_format():
    """
    Test the Ruuvi RAWv2 beacon packet structure with example values
    """
    
    # Example measured data
    # temp = 2350 (23.50°C in 0.01°C units)
    # humi = 5525 (55.25% in 0.01% units)
    # battery_mv = 2800 mV
    # MAC = [0x11, 0x22, 0x33, 0x44, 0x55, 0x66]
    # send_count = 123
    
    temp_measured = 2350  # 23.50°C in 0.01°C units
    humi_measured = 5525  # 55.25% in 0.01% units
    battery_mv = 2800
    mac_address = [0x11, 0x22, 0x33, 0x44, 0x55, 0x66]
    send_count = 123
    
    # Convert temperature to Ruuvi format
    # Ruuvi: int16, resolution 0.005°C
    # measured_data.temp is in 0.01°C units
    # Ruuvi format: temp_value = temperature / 0.005
    # Conversion: (temp * 0.01) / 0.005 = temp * 2
    temp_ruuvi = temp_measured * 2
    print(f"Temperature: {temp_measured} (0.01°C) -> {temp_ruuvi} (Ruuvi RAWv2)")
    print(f"  = {temp_ruuvi * 0.005:.3f}°C")
    
    # Convert humidity to Ruuvi format
    # Ruuvi: uint16, resolution 0.0025%
    # measured_data.humi is in 0.01% units
    # Ruuvi format: humi_value = humidity / 0.0025
    # Conversion: (humi * 0.01) / 0.0025 = humi * 4
    humi_ruuvi = humi_measured * 4
    print(f"Humidity: {humi_measured} (0.01%) -> {humi_ruuvi} (Ruuvi RAWv2)")
    print(f"  = {humi_ruuvi * 0.0025:.3f}%")
    
    # Pressure: not available, use sentinel
    pressure_ruuvi = 0xFFFF
    print(f"Pressure: 0xFFFF (sentinel for unavailable)")
    
    # Acceleration: not available, use 0
    accel_x = 0
    accel_y = 0
    accel_z = 0
    print(f"Acceleration: X={accel_x}, Y={accel_y}, Z={accel_z} (not available)")
    
    # Power info: bits 11..5: battery voltage above 1600mV in 1mV steps (11 bits)
    #             bits 4..0: TX power in 2dBm steps, offset -40dBm (5 bits)
    # Battery voltage in mV, subtract 1600 and shift to bits 11..5
    # TX power: 0 dBm -> (0 + 40) / 2 = 20
    if battery_mv < 1600:
        battery_mv = 1600
    if battery_mv > 3646:
        battery_mv = 3646
    battery_shifted = ((battery_mv - 1600) << 5) & 0xFFE0
    tx_power_bits = 20 & 0x1F  # TX power 0 dBm: (0 + 40) / 2 = 20
    power_info = battery_shifted | tx_power_bits
    print(f"Power info: 0x{power_info:04X}")
    print(f"  Battery: {battery_mv} mV -> {(battery_mv - 1600)} encoded -> bits 11..5: 0x{battery_shifted:04X}")
    print(f"  TX Power: 0 dBm in 2dBm steps -> encoded {tx_power_bits} -> bits 4..0: 0x{tx_power_bits:02X}")
    # Decode to verify
    decoded_battery = ((power_info >> 5) & 0x7FF) + 1600
    decoded_tx_power = ((power_info & 0x1F) * 2) - 40
    print(f"  Decoded battery: {decoded_battery} mV")
    print(f"  Decoded TX power: {decoded_tx_power} dBm")
    
    # Movement counter: not used
    movement_counter = 0
    
    # Measurement sequence: use send_count
    measurement_seq = send_count
    
    # Build the packet
    # Structure (24 bytes total):
    # Byte 0: size (23)
    # Byte 1: uid (0xFF - Manufacturer Specific)
    # Bytes 2-3: company_id (0x0499 little-endian -> 0x99, 0x04)
    # Byte 4: data_format (0x05)
    # Bytes 5-6: temperature (int16 little-endian)
    # Bytes 7-8: humidity (uint16 little-endian)
    # Bytes 9-10: pressure (uint16 little-endian)
    # Bytes 11-12: accel_x (int16 little-endian)
    # Bytes 13-14: accel_y (int16 little-endian)
    # Bytes 15-16: accel_z (int16 little-endian)
    # Bytes 17-18: power_info (uint16 little-endian)
    # Byte 19: movement_counter
    # Bytes 20-21: measurement_seq (uint16 little-endian)
    # Bytes 22-27: MAC address (6 bytes)
    
    packet = struct.pack(
        '<BB'  # size, uid
        'H'    # company_id (little-endian)
        'B'    # data_format
        'h'    # temperature (signed int16, little-endian)
        'H'    # humidity (uint16, little-endian)
        'H'    # pressure (uint16, little-endian)
        'hhh'  # accel_x, accel_y, accel_z (signed int16, little-endian)
        'H'    # power_info (uint16, little-endian)
        'B'    # movement_counter
        'H'    # measurement_seq (uint16, little-endian)
        '6B',  # MAC address (6 bytes)
        23,    # size
        0xFF,  # uid (GAP_ADTYPE_MANUFACTURER_SPECIFIC)
        0x0499,  # company_id (will be sent as 0x99 0x04 due to little-endian)
        0x05,  # data_format (RAWv2)
        temp_ruuvi,
        humi_ruuvi,
        pressure_ruuvi,
        accel_x,
        accel_y,
        accel_z,
        power_info,
        movement_counter,
        measurement_seq,
        *mac_address
    )
    
    print(f"\nPacket structure (24 bytes):")
    print(f"  Total size: {len(packet)} bytes")
    print(f"  Hex: {packet.hex(' ')}")
    
    # Verify company ID is in correct endianness
    company_id_bytes = packet[2:4]
    print(f"\nCompany ID bytes: {company_id_bytes.hex(' ')}")
    print(f"  Should be: 99 04 (0x0499 in little-endian)")
    assert company_id_bytes == b'\x99\x04', "Company ID endianness error"
    
    # Verify data format
    data_format = packet[4]
    print(f"Data format: 0x{data_format:02X}")
    assert data_format == 0x05, "Data format should be 0x05"
    
    # Verify MAC address
    mac_from_packet = packet[22:28]
    print(f"MAC address: {mac_from_packet.hex(':')}")
    
    print("\n✓ Packet structure validated successfully!")
    print("\nSummary:")
    print(f"  - Temperature: {temp_measured * 0.01:.2f}°C")
    print(f"  - Humidity: {humi_measured * 0.01:.2f}%")
    print(f"  - Battery: {battery_mv} mV")
    print(f"  - TX Power: 0 dBm")
    print(f"  - Sequence: {measurement_seq}")
    print(f"  - MAC: {':'.join(f'{b:02X}' for b in mac_address)}")

if __name__ == "__main__":
    test_ruuvi_beacon_format()
