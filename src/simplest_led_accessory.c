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

#define ACCESSORY_NAME  ("Кондиционер")
#define ACCESSORY_SN  ("SN_8362459")  //SERIAL_NUMBER
#define ACCESSORY_MANUFACTURER ("zrslv")
#define ACCESSORY_MODEL  ("ESP8266-COOLIX")

#define PIN_LED  D0

bool ac_power = false; //true or flase

homekit_value_t ac_active_get() {
	return HOMEKIT_UINT8(ac_power);
}

void ac_active_set(homekit_value_t value) {
	ac_power = value.uint8_value == 1;
	// led_update();
}

void on_update();
void on_fan_update();

homekit_characteristic_t ch_ac_name = HOMEKIT_CHARACTERISTIC_(NAME, ACCESSORY_NAME);
homekit_characteristic_t serial_number = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, ACCESSORY_SN);
homekit_characteristic_t ac_active = HOMEKIT_CHARACTERISTIC_(ACTIVE, 0, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(on_update));
homekit_characteristic_t cha_temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 25.f);
homekit_characteristic_t cha_current_hc_state = HOMEKIT_CHARACTERISTIC_(CURRENT_HEATER_COOLER_STATE, 3);
homekit_characteristic_t cha_target_hc_state = HOMEKIT_CHARACTERISTIC_(TARGET_HEATER_COOLER_STATE, 2, .valid_values={.count=1, .values=(uint8_t[]) {2}}, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(on_update));
homekit_characteristic_t cooling_threshold = HOMEKIT_CHARACTERISTIC_(
    COOLING_THRESHOLD_TEMPERATURE, 17.f, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(on_update)
);

// fan
homekit_characteristic_t ch_fan_active = HOMEKIT_CHARACTERISTIC_(ACTIVE, 0, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(on_fan_update));

extern void ac_on(float temp);
extern void ac_off();
extern void fan_on();

void on_update() {
	printf("-- on_update()\n");
	if (ac_active.value.uint8_value == 0 && ch_fan_active.value.uint8_value == 0) {
		printf("Both cooler and fan are off, turning off the unit.\n");
		ac_off();
	} else {
		printf("Turning on the cooler (and turning off the fan).\n");
		ac_on(cooling_threshold.value.float_value);
		ch_fan_active.value.uint8_value = 0;
		homekit_characteristic_notify(&ch_fan_active, ch_fan_active.value);
	}
}

void on_fan_update() {
	printf("-- on_fan_update()\n");
	if (ch_fan_active.value.uint8_value == 0 && cha_current_hc_state.value.uint8_value == 0) {
		printf("Both fan and cooler are off, turning off the unit.\n");
		ac_off();
	} else {
		printf("Turning on the fan (and turning off the cooler).\n");
		fan_on();
		cha_current_hc_state.value.uint8_value = 0;
		homekit_characteristic_notify(&cha_current_hc_state, cha_current_hc_state.value);
	}
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
						    HOMEKIT_CHARACTERISTIC(NAME, "Кондиционер"),
						    &ac_active,
							&cha_temperature,
							&cha_current_hc_state,
							&cha_target_hc_state,
							&cooling_threshold,
						    NULL
						  }),
						  HOMEKIT_SERVICE(FAN, .primary=false,
						  .characteristics=(homekit_characteristic_t*[]){
							HOMEKIT_CHARACTERISTIC(NAME, "Проветривание"),
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
	ac_off(); // Power off the unit upon startup to sync states.
}
