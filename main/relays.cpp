#include <Arduino.h>
#include "defs.h"
#include "relays.h"
#include "global.h"
#ifdef USE_MQTT
#include "mqtt.h"
#endif

static const char TAG[] = "Relay";

#ifdef USE_MQTT
static void relays_report(void) {
  if (mqttIsActive()) {
    char *payload;

    payload = (char*)malloc(ARRAY_SIZE(RELAY_PINS) * 6 + 13 + 1); // {"relays":[false,...]}
    if (payload) {
      strcpy(payload, "{\"relays\":[");
      for (uint8_t i = 0; i < ARRAY_SIZE(RELAY_PINS); ++i) {
        if (i)
          strcat(payload, ",");
        strcat(payload, BOOLS[relay_get(i)]);
      }
      strcat(payload, "]}");
      if (mqttPublish(payload)) {
        ESP_LOGI(TAG, "MQTT topic with payload \"%s\" was published", payload);
      }
      free(payload);
    }
  }
}
#endif

void relays_init(void) {
  for (uint8_t i = 0; i < ARRAY_SIZE(RELAY_PINS); ++i) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], ! RELAY_LEVEL);
  }
}

bool relay_get(uint8_t index) {
  return digitalRead(RELAY_PINS[index]) == RELAY_LEVEL;
}

void relay_set(uint8_t index, bool on, const char *source) {
  digitalWrite(RELAY_PINS[index], on == RELAY_LEVEL);
  ESP_LOGI(TAG, "Relay #%u turns o%s%s", index + 1, on ? "n" : "ff", source ? source : "");
#ifdef USE_MQTT
  relays_report();
#endif
}

void relay_toggle(uint8_t index, const char *source) {
  digitalWrite(RELAY_PINS[index], ! digitalRead(RELAY_PINS[index]));
  ESP_LOGI(TAG, "Relay #%u toggles%s", index + 1, source ? source : "");
#ifdef USE_MQTT
  relays_report();
#endif
}
