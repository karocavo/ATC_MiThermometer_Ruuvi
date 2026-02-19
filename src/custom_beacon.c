/*
 * custom_beacon.c
 *
 *  Created on: 07.03.2022
 *      Author: pvvx
 */
#include "tl_common.h"
#include "app_config.h"
#if (USE_CUSTOM_BEACON || USE_ATC_BEACON)
#include "ble.h"
#include "app.h"
#include "trigger.h"
#if (DEV_SERVICES & SERVICE_RDS)
#include "rds_count.h"
#endif
#include "custom_beacon.h"
#include "ccm.h"

#if (DEV_SERVICES & SERVICE_BINDKEY)

/* Encrypted atc/custom nonce */
typedef struct __attribute__((packed)) _enc_beacon_nonce_t{
    u8  MAC[6];
    adv_cust_head_t head;
} enc_beacon_nonce_t;

#if USE_ATC_BEACON
/* Create encrypted custom beacon packet
 * https://github.com/pvvx/ATC_MiThermometer/issues/94#issuecomment-842846036 */

__attribute__((optimize("-Os")))
void atc_encrypt_data_beacon(void) {
	padv_atc_enc_t p = (padv_atc_enc_t)&adv_buf.data;
	enc_beacon_nonce_t cbn;
	adv_atc_data_t data;
	u8 aad = 0x11;
	adv_buf.update_count = -1; // next call if next measured
	p->head.size = sizeof(adv_atc_enc_t) - 1;
	p->head.uid = GAP_ADTYPE_SERVICE_DATA_UUID_16BIT; // 16-bit UUID
	p->head.UUID = ADV_CUSTOM_UUID16; // GATT Service 0x181A Environmental Sensing (little-endian) (or 0x181C 'User Data'?)
	p->head.counter = (u8)adv_buf.send_count;
	data.temp = (measured_data.temp + 25) / 50 + 4000 / 50;
	data.humi = (measured_data.humi + 25) / 50;
	data.bat = measured_data.battery_level
#if (DEV_SERVICES & SERVICE_TH_TRG)
	| ((trg.flg.trigger_on)? 0x80 : 0)
#endif
	;
	memcpy(cbn.MAC, mac_public, sizeof(cbn.MAC));
	memcpy(&cbn.head, p, sizeof(cbn.head));
	aes_ccm_encrypt_and_tag((const unsigned char *)&bindkey,
					   (u8*)&cbn, sizeof(cbn),
					   &aad, sizeof(aad),
					   (u8 *)&data, sizeof(data),
					   (u8 *)&p->data,
					   p->mic, 4);
}
#endif

#if USE_CUSTOM_BEACON
__attribute__((optimize("-Os")))
void pvvx_encrypt_data_beacon(void) {
	padv_cust_enc_t p = (padv_cust_enc_t)&adv_buf.data;
	enc_beacon_nonce_t cbn;
	adv_cust_data_t data;
	u8 aad = 0x11;
	adv_buf.update_count = -1; // next call only at next measurement
	p->head.size = sizeof(adv_cust_enc_t) - 1;
	p->head.uid = GAP_ADTYPE_SERVICE_DATA_UUID_16BIT; // 16-bit UUID
	p->head.UUID = ADV_CUSTOM_UUID16; // GATT Service 0x181A Environmental Sensing (little-endian) (or 0x181C 'User Data'?)
	p->head.counter = (u8)adv_buf.send_count;
	data.temp = measured_data.temp;
	data.humi = measured_data.humi;
	data.bat = measured_data.battery_level;
#if (DEV_SERVICES & SERVICE_TH_TRG)
	data.trg = trg.flg_byte;
#else
	data.trg = 0;
#endif
	memcpy(cbn.MAC, mac_public, sizeof(cbn.MAC));
	memcpy(&cbn.head, p, sizeof(cbn.head));
	aes_ccm_encrypt_and_tag((const unsigned char *)&bindkey,
					   (u8*)&cbn, sizeof(cbn),
					   &aad, sizeof(aad),
					   (u8 *)&data, sizeof(data),
					   (u8 *)&p->data,
					   p->mic, 4);
}

#endif
#endif // #if (DEV_SERVICES & SERVICE_BINDKEY)

#if USE_CUSTOM_BEACON
_attribute_ram_code_
__attribute__((optimize("-Os")))
void pvvx_data_beacon(void) {
	padv_custom_t p = (padv_custom_t)&adv_buf.data;
	memcpy(p->MAC, mac_public, 6);
#if (DEV_SERVICES & SERVICE_TH_TRG)
	p->size = sizeof(adv_custom_t) - 1;
#else
	p->size = sizeof(adv_custom_t) - 2;
#endif
	p->uid = GAP_ADTYPE_SERVICE_DATA_UUID_16BIT; // 16-bit UUID
	p->UUID = ADV_CUSTOM_UUID16; // GATT Service 0x181A Environmental Sensing (little-endian)
	p->temperature = measured_data.temp; // x0.01 C
	p->humidity = measured_data.humi; // x0.01 %
#if USE_AVERAGE_BATTERY
	p->battery_mv = measured_data.average_battery_mv; // x mV
#else
	p->battery_mv = measured_data.battery_mv; // x mV
#endif
	p->battery_level = measured_data.battery_level; // x1 %
	p->counter = (u8)adv_buf.send_count;
#if (DEV_SERVICES & SERVICE_TH_TRG)
	p->flags = trg.flg_byte;
#endif
}
#endif

#if USE_ATC_BEACON
_attribute_ram_code_
__attribute__((optimize("-Os")))
void atc_data_beacon(void) {
	padv_atc1441_t p = (padv_atc1441_t)&adv_buf.data;
	p->size = sizeof(adv_atc1441_t) - 1;
	p->uid = GAP_ADTYPE_SERVICE_DATA_UUID_16BIT; // 16-bit UUID
	p->UUID = ADV_CUSTOM_UUID16; // GATT Service 0x181A Environmental Sensing (little-endian)
#if 1
	SwapMacAddress(p->MAC, mac_public);
#else
	p->MAC[0] = mac_public[5];
	p->MAC[1] = mac_public[4];
	p->MAC[2] = mac_public[3];
	p->MAC[3] = mac_public[2];
	p->MAC[4] = mac_public[1];
	p->MAC[5] = mac_public[0];
#endif
	p->temperature[0] = (u8)(measured_data.temp_x01 >> 8);
	p->temperature[1] = (u8)measured_data.temp_x01; // x0.1 C
	p->humidity = measured_data.humi_x1; // x1 %
	p->battery_level = measured_data.battery_level; // x1 %
#if USE_AVERAGE_BATTERY
	p->battery_mv[0] = (u8)(measured_data.average_battery_mv >> 8);
	p->battery_mv[1] = (u8)measured_data.average_battery_mv; // x1 mV
#else
	p->battery_mv[0] = (u8)(measured_data.battery_mv >> 8);
	p->battery_mv[1] = (u8)measured_data.battery_mv; // x1 mV
#endif
	p->counter = (u8)adv_buf.send_count;
}
#endif

/* Ruuvi RAWv2 beacon - manufacturer specific data format */
_attribute_ram_code_
__attribute__((optimize("-Os")))
void ruuvi_data_beacon(void) {
	padv_ruuvi_t p = (padv_ruuvi_t)&adv_buf.data;
	s16 temp_raw;
	u16 humi_raw;
	u16 pressure_raw;
	u16 power_info;
	u16 meas_seq;
	s8 tx_dbm;
	u16 batt_mv;
	
	p->size = sizeof(adv_ruuvi_t) - 1;
	p->uid = 0xFF; // GAP_ADTYPE_MANUFACTURER_SPECIFIC_DATA
	p->mfr_id = 0x0499; // Ruuvi Innovations (little-endian in BLE)
	p->data_type = 0x05; // RAWv2 format

	/* Temperature: measured_data.temp is in 0.01 C, RAWv2 uses 0.005 C */
	temp_raw = (s16)(measured_data.temp * 2);
	p->temp[0] = (u8)((temp_raw >> 8) & 0xFF);
	p->temp[1] = (u8)(temp_raw & 0xFF);

	/* Humidity: measured_data.humi is in 0.01%, RAWv2 uses 0.0025% */
	humi_raw = (u16)(measured_data.humi * 4);
	p->humi[0] = (u8)(humi_raw >> 8);
	p->humi[1] = (u8)(humi_raw & 0xFF);

	/* Pressure: no sensor, fake standard atmospheric pressure */
	pressure_raw = 51325; // 101325 Pa - 50000 Pa offset
	p->pressure[0] = (u8)(pressure_raw >> 8);
	p->pressure[1] = (u8)(pressure_raw & 0xFF);

	/* Acceleration: not available, fake 0 mg */
	p->accel_x[0] = 0;
	p->accel_x[1] = 0;
	p->accel_y[0] = 0;
	p->accel_y[1] = 0;
	p->accel_z[0] = 0;
	p->accel_z[1] = 0;

	/* Power info: battery (mV, 11 bits) + TX power (5 bits, -40..+20 dBm) */
#if USE_AVERAGE_BATTERY
	batt_mv = measured_data.average_battery_mv;
#else
	batt_mv = measured_data.battery_mv;
#endif
	if (batt_mv > 2047) {
		batt_mv = 2047;
	}
	tx_dbm = (s8)(cfg.rf_tx_power - 60); // rough conversion from 0-191 range to dBm
	if (tx_dbm < -40) {
		tx_dbm = -40;
	} else if (tx_dbm > 20) {
		tx_dbm = 20;
	}
	power_info = (u16)((batt_mv & 0x07FF) << 5) | ((u8)tx_dbm & 0x1F);
	p->power_info[0] = (u8)(power_info >> 8);
	p->power_info[1] = (u8)(power_info & 0xFF);

	/* Movement counter and measurement sequence */
	p->move_count = (u8)adv_buf.send_count;
	meas_seq = (u16)adv_buf.send_count;
	p->meas_seq[0] = (u8)(meas_seq >> 8);
	p->meas_seq[1] = (u8)(meas_seq & 0xFF);

	/* MAC address */
	memcpy(p->MAC, mac_public, 6);
}

#if (DEV_SERVICES & SERVICE_RDS)

typedef struct __attribute__((packed)) _ext_adv_cnt_t {
	u8		size;	// = 6
	u8		uid;	// = 0x16, 16-bit UUID https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile/
	u16		UUID;	// = 0x2AEB - Count 24
	u8		cnt[3];
} ext_adv_cnt_t, * pext_adv_cnt_t;

typedef struct __attribute__((packed)) _ext_adv_digt_t {
	u8		size;	// = 4
	u8		uid;	// = 0x16, 16-bit UUID https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile/
	u16		UUID;	// = 0x2A56 - Digital State Bits
	u8		bits;
} ext_adv_dig_t, * pext_adv_dig_t;

typedef struct __attribute__((packed)) _adv_event_t {
	ext_adv_dig_t dig;
	ext_adv_cnt_t cnt;
} adv_event_t, * padv_event_t;

void default_event_beacon(void){
	padv_event_t p = (padv_event_t)&adv_buf.data;
	p->dig.size = sizeof(p->dig) - sizeof(p->dig.size);
	p->dig.uid = GAP_ADTYPE_SERVICE_DATA_UUID_16BIT; // 16-bit UUID
	p->dig.UUID = ADV_UUID16_DigitalStateBits;
	p->dig.bits = trg.flg_byte;
	p->cnt.size = sizeof(p->cnt) - sizeof(p->cnt.size);
	p->cnt.uid = GAP_ADTYPE_SERVICE_DATA_UUID_16BIT; // 16-bit UUID
	p->cnt.UUID = ADV_UUID16_Count24bits;
	p->cnt.cnt[0] = rds.count1_byte[2];
	p->cnt.cnt[1] = rds.count1_byte[1];
	p->cnt.cnt[2] = rds.count1_byte[0];
	adv_buf.data_size = sizeof(adv_event_t);
}

void pvvx_event_beacon(u8 n){
	if (n == RDS_SWITCH) {
		pvvx_data_beacon();
		adv_buf.data_size = adv_buf.data[0] + 1;
	} else
		default_event_beacon();
}

#if (DEV_SERVICES & SERVICE_BINDKEY)

void pvvx_encrypt_event_beacon(u8 n){
	if (n == RDS_SWITCH) {
		pvvx_encrypt_data_beacon();
		adv_buf.data_size = adv_buf.data[0] + 1;
	} else
		default_event_beacon();
}

#endif // (DEV_SERVICES & SERVICE_RDS)
#endif // #if (DEV_SERVICES & SERVICE_BINDKEY)
#endif // (USE_CUSTOM_BEACON || USE_ATC_BEACON)
