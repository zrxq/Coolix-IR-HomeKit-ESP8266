#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DallasTemperature.h>

#include <arduino_homekit_server.h>
#include <WiFiManager.h>


#define PL(s) Serial.println(s)
#define P(s) Serial.print(s)

//D0 16 //led
//D3  0 //flash button
//D4  2 //led

#define PIN_LED D0

OneWire oneWire(D3);
DallasTemperature temp_sensor(&oneWire);
float temperature = 0.f;

void builtinledSetStatus(bool on);

void blink_led(int interval, int count) {
  for (int i = 0; i < count; i++) {
    builtinledSetStatus(true);
    delay(interval);
    builtinledSetStatus(false);
    delay(interval);
  }
}

extern "C" void ac_setup();
void homekit_setup();
void homekit_loop();

void setup() {
  Serial.begin(115200); while (!Serial); delay(200);
  Serial.setRxBufferSize(32);
  Serial.setDebugOutput(false);

  ac_setup();

  pinMode(PIN_LED, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.disconnect(false);
  WiFi.setAutoReconnect(true);

    WiFiManager wm;
    bool wifi_okay = wm.autoConnect();
    if (wifi_okay) { 
        Serial.print("\n");
        Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("WiFi failed, restarting.");
        ESP.restart();
    }

  printf("\n");
  printf("SketchSize: %d B\n", ESP.getSketchSize());
  printf("FreeSketchSpace: %d B\n", ESP.getFreeSketchSpace());
  printf("FlashChipSize: %d B\n", ESP.getFlashChipSize());
  printf("FlashChipRealSize: %d B\n", ESP.getFlashChipRealSize());
  printf("FlashChipSpeed: %d\n", ESP.getFlashChipSpeed());
  printf("SdkVersion: %s\n", ESP.getSdkVersion());
  printf("FullVersion: %s\n", ESP.getFullVersion().c_str());
  printf("CpuFreq: %dMHz\n", ESP.getCpuFreqMHz());
  printf("FreeHeap: %d B\n", ESP.getFreeHeap());
  printf("ResetInfo: %s\n", ESP.getResetInfo().c_str());
  printf("ResetReason: %s\n", ESP.getResetReason().c_str());
  DEBUG_HEAP();
  homekit_setup();
  DEBUG_HEAP();
  blink_led(200, 3);
}

void update_temp() {
  temp_sensor.requestTemperatures();
  temperature = temp_sensor.getTempCByIndex(0);
}


void loop() {
  update_temp();
  homekit_loop();
  delay(5);
}

void builtinledSetStatus(bool on) {
  digitalWrite(PIN_LED, on ? LOW : HIGH);
  digitalWrite(2, on ? LOW : HIGH);
}

//==============================
// Homekit setup and loop
//==============================

extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t ch_ac_name;
extern "C" void led_toggle();
extern "C" void accessory_init();

uint32_t next_heap_millis = 0;

void homekit_setup() {
  accessory_init();
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);
  int name_len = snprintf(NULL, 0, "%s_%02X%02X%02X", ch_ac_name.value.string_value, mac[3], mac[4], mac[5]);
  char *name_value = (char*)malloc(name_len + 1);
  snprintf(name_value, name_len + 1, "%s_%02X%02X%02X", ch_ac_name.value.string_value, mac[3], mac[4], mac[5]);
  ch_ac_name.value = HOMEKIT_STRING_CPP(name_value);

  arduino_homekit_setup(&config);

}

extern "C" void update_ac();

void homekit_loop() {
  arduino_homekit_loop();
  update_ac();
  uint32_t time = millis();
  if (time > next_heap_millis) {
    INFO("heap: %d, sockets: %d", ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
    next_heap_millis = time + 5000;
  }
}
