/*
 * custom_beacon.h
 *
 *  Created on: 07.03.2022
 *      Author: pvvx
 */

#ifndef CUSTOM_BEACON_H_
#define CUSTOM_BEACON_H_

#define ADV_CUSTOM_UUID16 0x181A // 16-bit UUID Service 0x181A Environmental Sensing

#define ADV_RUUVI_COMPANY_ID 0x0499 // Ruuvi Innovations Ltd. Company ID (transmitted as 0x99 0x04 little-endian)

// Helper macros for converting 16-bit values to big-endian (MSB first) for Ruuvi RAWv2
// Ruuvi spec requires all multi-byte fields to be big-endian
#define U16_TO_BIG_ENDIAN(val, arr) do { \
	(arr)[0] = (u8)((val) >> 8); \
	(arr)[1] = (u8)(val); \
} while(0)

#define S16_TO_BIG_ENDIAN(val, arr) do { \
	(arr)[0] = (u8)((val) >> 8); \
	(arr)[1] = (u8)(val); \
} while(0)

#define ADV_UUID16_DigitalStateBits	0x2A56 // 16-bit UUID Digital bits, Out bits control (LEDs control)
#define ADV_UUID16_AnalogOutValues	0x2A58 // 16-bit UUID Analog values (DACs control)
#define ADV_UUID16_Aggregate		0x2A5A // 16-bit UUID Aggregate, The Aggregate Input is an aggregate of the Digital Input Characteristic value (if available) and ALL Analog Inputs available.
#define ADV_UUID16_Count24bits		0x2AEB // 16-bit UUID Count 24 bits
#define ADV_UUID16_Count16bits 		0x2AEA // 16-bit UUID Count 16 bits


// GATT Service 0x181A Environmental Sensing
// All data little-endian
typedef struct __attribute__((packed)) _adv_custom_t {
	u8		size;	// = 18
	u8		uid;	// = 0x16, 16-bit UUID
	u16		UUID;	// = 0x181A, GATT Service 0x181A Environmental Sensing
	u8		MAC[6]; // [0] - lo, .. [6] - hi digits
	s16		temperature; // x 0.01 degree
	u16		humidity; // x 0.01 %
	u16		battery_mv; // mV
	u8		battery_level; // 0..100 %
	u8		counter; // measurement count
	u8		flags;
} adv_custom_t, * padv_custom_t;

// GATT Service 0x181A Environmental Sensing
// mixture of little-endian and big-endian!
typedef struct __attribute__((packed)) _adv_atc1441_t {
	u8		size;	// = 16
	u8		uid;	// = 0x16, 16-bit UUID
	u16		UUID;	// = 0x181A, GATT Service 0x181A Environmental Sensing (little-endian)
	u8		MAC[6]; // [0] - hi, .. [6] - lo digits (big-endian!)
	u8		temperature[2]; // x 0.1 degree (big-endian!)
	u8		humidity; // x 1 %
	u8		battery_level; // 0..100 %
	u8		battery_mv[2]; // mV (big-endian!)
	u8		counter; // measurement count
} adv_atc1441_t, * padv_atc1441_t;

/* Encrypted custom beacon structs */
// https://github.com/pvvx/ATC_MiThermometer/issues/94#issuecomment-842846036

typedef struct __attribute__((packed)) _adv_cust_head_t {
	u8		size;		//@0 = 11
	u8		uid;		//@1 = 0x16, 16-bit UUID https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile/
	u16		UUID;		//@2..3 = GATT Service 0x181A Environmental Sensing (little-endian) (or 0x181C 'User Data'?)
	u8		counter;	//@4 0..0xff Measurement count, Serial number, used for de-duplication, different event or attribute reporting requires different Frame Counter
} adv_cust_head_t, * padv_cust_head_t;

typedef struct __attribute__((packed)) _adv_cust_data_t {
	s16		temp;		//@0
	u16		humi;		//@2
	u8		bat;		//@4
	u8		trg;		//@5
} adv_cust_data_t, * padv_cust_data_t;

typedef struct __attribute__((packed)) _adv_cust_enc_t {
	adv_cust_head_t head;
	adv_cust_data_t data;	//@5
	u8		mic[4];		//@8..11
} adv_cust_enc_t, * padv_cust_enc_t;

/* Encrypted atc beacon structs */

typedef struct __attribute__((packed)) _adv_atc_data_t {
	u8		temp;		//@0
	u8		humi;		//@1
	u8		bat;		//@2
} adv_atc_data_t, * padv_atc_data_t;

typedef struct __attribute__((packed)) _adv_atc_enc_t {
	adv_cust_head_t head;
	adv_atc_data_t data;   //@5
	u8		mic[4];		//@8..11
} adv_atc_enc_t, * padv_atc_enc_t;

/* Ruuvi RAWv2 (Data Format 5) beacon struct */
// Manufacturer Specific Data format for Ruuvi
// https://github.com/ruuvi/ruuvi-sensor-protocols
// IMPORTANT: All multi-byte fields are BIG-ENDIAN (MSB first) per Ruuvi specification
typedef struct __attribute__((packed)) _adv_ruuvi_rawv2_t {
	u8		size;			// total size - 1
	u8		uid;			// = 0xFF, GAP_ADTYPE_MANUFACTURER_SPECIFIC
	u16		company_id;		// = 0x0499 (transmitted as 0x99 0x04 little-endian)
	u8		data_format;	// = 0x05 (RAWv2 / Data Format 5)
	u8		temperature[2];	// int16 BIG-ENDIAN, resolution 0.005 °C, range -163.835..+163.835 °C
	u8		humidity[2];	// uint16 BIG-ENDIAN, resolution 0.0025%, range 0..163.835%
	u8		pressure[2];	// uint16 BIG-ENDIAN, offset -50000 Pa (not used, set to 0xFFFF)
	u8		accel_x[2];		// int16 BIG-ENDIAN, 0.001 g (not used, set to 0)
	u8		accel_y[2];		// int16 BIG-ENDIAN, 0.001 g (not used, set to 0)
	u8		accel_z[2];		// int16 BIG-ENDIAN, 0.001 g (not used, set to 0)
	u8		power_info[2];	// uint16 BIG-ENDIAN, bits 11..5: battery voltage above 1600mV (in 1mV steps), bits 4..0: TX power (dBm+40)/2
	u8		movement_counter; // movement counter (not used, set to 0)
	u8		measurement_seq[2]; // uint16 BIG-ENDIAN, measurement sequence number
	u8		MAC[6];			// MAC address [0] - lo, .. [5] - hi
} adv_ruuvi_rawv2_t, * padv_ruuvi_rawv2_t;


void pvvx_data_beacon(void);
void atc_data_beacon(void);
void ruuvi_data_beacon(void);
#if (DEV_SERVICES & SERVICE_RDS)
void pvvx_event_beacon(u8 n); // n = RDS_TYPES
void default_event_beacon(void);
#endif

#if (DEV_SERVICES & SERVICE_BINDKEY)
void pvvx_encrypt_data_beacon(void); // n = RDS_TYPES
void atc_encrypt_data_beacon(void);

#if (DEV_SERVICES & SERVICE_RDS)
void pvvx_encrypt_event_beacon(u8 n); // n = RDS_TYPES
#endif
#endif // #if (DEV_SERVICES & SERVICE_BINDKEY)

#endif /* CUSTOM_BEACON_H_ */
