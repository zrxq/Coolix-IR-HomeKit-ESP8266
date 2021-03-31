#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRac.h>
#include <IRutils.h>
#include <limits.h>

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRac ac(kIrLed);      // Set the GPIO to be used for sending messages.

void set_needs_ir();

void ac_reset() {
  ac.next.protocol = decode_type_t::TCL112AC;  // Set a protocol to use.
  ac.next.mode = stdAc::opmode_t::kHeat;  // Run in cool mode initially.
  ac.next.celsius = true;  // Use Celsius for temp units. False = Fahrenheit
  ac.next.degrees = 25;  // 25 degrees.
  ac.next.fanspeed = stdAc::fanspeed_t::kMax;  // Start the fan at medium.
  ac.next.swingv = stdAc::swingv_t::kOff;  // Don't swing the fan up or down.
  ac.next.swingh = stdAc::swingh_t::kOff;  // Don't swing the fan left or right.
  ac.next.light = false;  // Turn off any LED/Lights/Display that we can.
  ac.next.beep = false;  // Turn off any beep from the A/C if we can.
  ac.next.econo = false;  // Turn off any economy modes if we can.
  ac.next.filter = false;  // Turn off any Ion/Mold/Health filters if we can.
  ac.next.turbo = false;  // Don't use any turbo/powerful/etc modes.
  ac.next.quiet = false;  // Don't use any quiet/silent/etc modes.
  ac.next.sleep = -1;  // Don't set any sleep time or modes.
  ac.next.clean = false;  // Turn off any Cleaning options if we can.
  ac.next.clock = -1;  // Don't set any current time if we can avoid it.
}

extern "C" void ac_setup() {
  Serial.println("Starting A/C");
  pinMode(kIrLed, OUTPUT);
  
  ac_reset();
  ac.next.power = false;   
  set_needs_ir();
}

extern "C" bool is_ac_on() {
  bool power = ac.getState().power;
  printf("[AC] Get power = \%d\n", power);
  return power;
}

extern "C" void set_fan_on(bool);
extern "C" void set_target_hc_state(uint8_t state);

extern "C" void set_ac_on(bool is_on) {
  ac.next.power = is_on;
  set_needs_ir();
}

extern "C" uint8_t get_current_hc_state() {
    uint8_t s;
    if (!ac.getState().power) {
      s = 0;
    } else 
    {
      stdAc::opmode_t mode = ac.getState().mode;
      switch (mode)
      {
      case stdAc::opmode_t::kHeat:
        s = 2; // NOTE: The constants have different meaning from target H/C state constants.

      case stdAc::opmode_t::kCool:
        s = 3;
      
      default:
        s = 1; // "idle", since dry, fan etc modes don't map onto HomeKit's heater/cooler state.
      }
    }

    printf("Get current H/C state (%d)\n", s);
    return s;
}

extern "C" void set_target_hc_state(uint8_t state) {
  printf("[AC] Set state = %d.\n", state);
  switch (state) {
    case 0:
      ac.next.mode = stdAc::opmode_t::kAuto;

    case 1:
      ac.next.mode = stdAc::opmode_t::kHeat;
      break;

    case 2:
      ac.next.mode = stdAc::opmode_t::kCool;
      break;
  }
  set_needs_ir();
}

extern "C" void set_threshold(float t) {
  if ((int)t == ac.getState().degrees) {
    printf("Same temperature, ignoring.\n");
    return;
  }
  printf("[AC] Set threshold = %f\n", t);
  ac.next.degrees = t;
  set_needs_ir();
}

extern "C" float get_threshold() {
  int temp = ac.getState().degrees;
  printf("Get threshold (=%d).\n", temp);
  return temp;
}

extern "C" void set_fan_on(bool fan_on) {
  if (ac.getState().power == true && ac.getState().mode == stdAc::opmode_t::kFan && fan_on == false) {
    // HomeKit asked to turn off the fan, the AC is in the fan mode, turn off the AC altogether.
    ac.next.power = false;
  }
  if (fan_on == true && ac.getState().power == false) {
    // AC was of, requested to turn on the fan, turn on the AC as well
    ac.next.power = true;
  }
  ac.next.mode = stdAc::opmode_t::kFan;
  ac.next.fanspeed = stdAc::fanspeed_t::kMax;

  set_needs_ir();
}

unsigned long prev_time = ULONG_MAX;

unsigned long ir_dirty_since = ULONG_MAX;
const unsigned long ir_debounce = 500;

extern "C" void update_ac() {
  unsigned long time = millis();
  if (ir_dirty_since != ULONG_MAX && (time - ir_dirty_since > ir_debounce || prev_time > time)) {
    ac.sendAc();
    printf("Current state: %s, next: %s\n", ac.opmodeToString(ac.getState().mode).c_str(), ac.opmodeToString(ac.next.mode).c_str());
    printf("Sending IR.\n");
    ir_dirty_since = ULONG_MAX;
  }
  prev_time = time;
}

void set_needs_ir() {
  ir_dirty_since = millis();
}
