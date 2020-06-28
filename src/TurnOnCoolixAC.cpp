/* Copyright 2016, 2018 David Conran
*
* An IR LED circuit *MUST* be connected to the ESP8266 on a pin
* as specified by kIrLed below.
*
* TL;DR: The IR LED needs to be driven by a transistor for a good result.
*
* Suggested circuit:
*     https://github.com/crankyoldgit/IRremoteESP8266/wiki#ir-sending
*
* Common mistakes & tips:
*   * Don't just connect the IR LED directly to the pin, it won't
*     have enough current to drive the IR LED effectively.
*   * Make sure you have the IR LED polarity correct.
*     See: https://learn.sparkfun.com/tutorials/polarity/diode-and-led-polarity
*   * Typical digital camera/phones can be used to see if the IR LED is flashed.
*     Replace the IR LED with a normal LED if you don't have a digital camera
*     when debugging.
*   * Avoid using the following pins unless you really know what you are doing:
*     * Pin 0/D3: Can interfere with the boot/program mode & support circuits.
*     * Pin 1/TX/TXD0: Any serial transmissions from the ESP8266 will interfere.
*     * Pin 3/RX/RXD0: Any serial transmissions to the ESP8266 will interfere.
*   * ESP-01 modules are tricky. We suggest you use a module with more GPIOs
*     for your first time. e.g. ESP-12 etc.
*/
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Coolix.h>
#include <limits.h>

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRCoolixAC ac(kIrLed);      // Set the GPIO to be used for sending messages.

void set_needs_ir();

extern "C" void ac_setup() {
  Serial.println("Starting Coolix A/C");
  pinMode(kIrLed, OUTPUT);
  ac.begin();
}

extern "C" bool is_ac_on() {
  printf("[AC] Get power = \%d\n", ac.getPower());
  set_needs_ir();
  return ac.getPower();
}

extern "C" void set_ac_on(bool is_on) {
  if (is_on) {
    printf("[AC] Turning ON.\n");
    ac.on();
  } else {
    if (ac.getMode() != kCoolixFan) {
      printf("[AC] Turning OFF.\n");
      ac.off();
    }
  }
  set_needs_ir();
}

extern "C" uint8_t get_current_hc_state() {
    if (!ac.getPower()) {
      return 0;
    }
    uint8_t mode = ac.getMode();
    switch (mode)
    {
    case kCoolixCool:
      return 3;

    case kCoolixHeat:
      return 2;
    
    default:
      return 1; // "idle", since dry, fan and auto modes don't map onto HomeKit's heater/cooler state.
    }
}

extern "C" void set_target_hc_state(uint8_t state) {
  printf("[AC] Set state = %d.\n", state);
  switch (state)
  {
    case 0:
      ac.setMode(kCoolixAuto);
    break;

    case 1:
      ac.setMode(kCoolixHeat);
    break;

    case 2:
      ac.setMode(kCoolixCool);
    break;    
  
  default:
      ac.setMode(kCoolixCool);
    break;
  }
  set_needs_ir();
}

extern "C" void set_threshold(float t) {
  printf("[AC] Set threshold = %f\n", t);
  ac.setTemp((uint8_t)t);
  set_needs_ir();
}

extern "C" float get_threshold() {
  return ac.getTemp();
}

extern "C" void set_fan_on(bool is_on) {
  if (is_on) {
    ac.setMode(kCoolixFan);
    ac.on();
  } else {
    if (ac.getMode() == kCoolixFan) {
      ac.off();
    }
  }
  set_needs_ir();
}

unsigned long prev_time = ULONG_MAX;

unsigned long ir_dirty_since = ULONG_MAX;
const unsigned long ir_debounce = 300;

extern "C" void update_ac() {
  unsigned long time = millis();
  if (ir_dirty_since != ULONG_MAX && (time - ir_dirty_since > ir_debounce || prev_time > time)) {
    ac.send();
    ir_dirty_since = ULONG_MAX;
  }
  prev_time = time;
}

void set_needs_ir() {
  ir_dirty_since = millis();
}

extern "C" void fan_on() {
  ac.on();
  ac.setMode(kCoolixFan);  
  ac.setFan(kCoolixFanMax);
  ac.send();
}
