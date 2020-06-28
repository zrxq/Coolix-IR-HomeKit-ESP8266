/*
 * simple_led_accessory.c
 * Define the accessory in pure C language using the Macro in characteristics.h
 *
 *  Created on: 2020-02-08
 *      Author: Mixiaoxiao (Wang Bin)
 *  Edited on: 2020-03-01
 *      Edited by: euler271 (Jonas Linn)
 */

#include <Arduino.h>
#include <homekit/types.h>
#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <stdio.h>
#include <port.h>

//const char * buildTime = __DATE__ " " __TIME__ " GMT";

#define ACCESSORY_NAME  ("A/C IR Remote")
#define ACCESSORY_SN  ("SN_8362459")  //SERIAL_NUMBER
#define ACCESSORY_MANUFACTURER ("zrslv")
#define ACCESSORY_MODEL  ("ESP8266-COOLIX")

#define PIN_LED  D0

extern bool is_ac_on();
extern void set_ac_on(bool on);
extern uint8_t get_current_hc_state();
extern void set_target_hc_state(uint8_t state);
extern void set_threshold(float t);
extern void set_fan_on(bool is_on);


homekit_value_t ac_active_get() {
	return HOMEKIT_UINT8(is_ac_on());
}

void ac_active_set(homekit_value_t value) {
	set_ac_on(value.uint8_value != 0);
}


homekit_value_t current_hc_state() {
	return HOMEKIT_UINT8(get_current_hc_state());
}

uint8_t _target_hc_state = 0;
homekit_value_t target_hc_state() {
	return HOMEKIT_UINT8(_target_hc_state);
}

void target_hc_state_set(homekit_value_t value) {
	_target_hc_state = value.uint8_value;
	set_target_hc_state(_target_hc_state);
}


extern float temperature;

homekit_value_t get_temperature() {
	return HOMEKIT_FLOAT(temperature);
}


void update_threshold();

homekit_characteristic_t ch_ac_name = HOMEKIT_CHARACTERISTIC_(NAME, ACCESSORY_NAME);
homekit_characteristic_t serial_number = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, ACCESSORY_SN);
homekit_characteristic_t ch_ac_active = HOMEKIT_CHARACTERISTIC_(ACTIVE, 0, .getter=ac_active_get, .setter=ac_active_set);
homekit_characteristic_t ch_temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0, .getter=get_temperature);
homekit_characteristic_t ch_current_hc_state = HOMEKIT_CHARACTERISTIC_(CURRENT_HEATER_COOLER_STATE, 0, .getter=current_hc_state);
homekit_characteristic_t ch_target_hc_state = HOMEKIT_CHARACTERISTIC_(TARGET_HEATER_COOLER_STATE, 2, 
																	.getter=target_hc_state, .setter=target_hc_state_set);
homekit_characteristic_t ch_cooling_threshold = HOMEKIT_CHARACTERISTIC_(COOLING_THRESHOLD_TEMPERATURE, 25.f, 
																.callback=HOMEKIT_CHARACTERISTIC_CALLBACK(update_threshold));
homekit_characteristic_t ch_heating_threshold = HOMEKIT_CHARACTERISTIC_(HEATING_THRESHOLD_TEMPERATURE, 25.f, 
																.callback=HOMEKIT_CHARACTERISTIC_CALLBACK(update_threshold));

void update_threshold() {
	set_threshold(ch_cooling_threshold.value.float_value);
}


// fan
void on_fan_update();

homekit_characteristic_t ch_fan_active = HOMEKIT_CHARACTERISTIC_(ACTIVE, 0, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(on_fan_update));

void on_fan_update() {
	set_fan_on(ch_fan_active.value.uint8_value != 0);
}

void accessory_identify(homekit_value_t _value) {
	printf("accessory identify\n");
	for (int j = 0; j < 3; j++) {
		digitalWrite(PIN_LED, HIGH);
		delay(100);
		digitalWrite(PIN_LED, LOW);
		delay(100);
	}
}

homekit_accessory_t *accessories[] =
		{
				HOMEKIT_ACCESSORY(
						.id = 1,
						.category = homekit_accessory_category_air_conditioner,
						.services=(homekit_service_t*[]){
						  HOMEKIT_SERVICE(ACCESSORY_INFORMATION,
						  .characteristics=(homekit_characteristic_t*[]){
						    &ch_ac_name,
						    HOMEKIT_CHARACTERISTIC(MANUFACTURER, ACCESSORY_MANUFACTURER),
						    &serial_number,
						    HOMEKIT_CHARACTERISTIC(MODEL, ACCESSORY_MODEL),
						    HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.0.1"),
						    HOMEKIT_CHARACTERISTIC(IDENTIFY, accessory_identify),
						    NULL
						  }),
						  HOMEKIT_SERVICE(HEATER_COOLER, .primary=true,
						  .characteristics=(homekit_characteristic_t*[]){
						    &ch_ac_active,
							&ch_temperature,
							&ch_current_hc_state,
							&ch_target_hc_state,
							&ch_cooling_threshold,
							// &ch_heating_threshold,
						    NULL
						  }),
						  HOMEKIT_SERVICE(FAN, .primary=false,
						  .characteristics=(homekit_characteristic_t*[]){
						    &ch_fan_active,
						    NULL
						  }),
						  NULL
						}),
				NULL
		};

homekit_server_config_t config = {
		.accessories = accessories,
		.password = "111-11-111",
};

void accessory_init() {
}
