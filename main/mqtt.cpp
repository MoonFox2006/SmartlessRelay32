#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "config.h"
#include "mqtt.h"

#ifdef USE_MQTT
static const char TAG[] = "MQTT";

static NetworkClient *tcp = nullptr;
static PubSubClient *mqtt = nullptr;
static TaskHandle_t mqtt_task = nullptr;

static void mqttCallback(char *topic, uint8_t *payload, unsigned int length) {
  char *buf;

  buf = (char*)malloc(length + 1);
  if (buf) {
    memcpy(buf, payload, length);
    buf[length] = '\0';
    ESP_LOGI(TAG, "MQTT topic \"%s\" with payload \"%s\" received", topic, buf);
    free(buf);
  }
}

static void mqttTask(void *arg) {
  constexpr uint32_t RECONNECT_TIMEOUT = 5000; // 5 sec.

  PubSubClient *mqtt = (PubSubClient*)arg;

  while (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1)) == 0) {
    if (mqtt->connected()) {
      mqtt->loop();
    } else {
      if (mqtt->connect(config->mqtt_client, *config->mqtt_user ? config->mqtt_user : nullptr, *config->mqtt_pswd ? config->mqtt_pswd : nullptr)) {
        ESP_LOGI(TAG, "Connect to broker successful");
        if (*config->mqtt_sub) {
          if (mqtt->subscribe(config->mqtt_sub, config->mqtt_qos)) {
            ESP_LOGI(TAG, "Subscribe to topic \"%s\" successful", config->mqtt_sub);
          } else {
            ESP_LOGE(TAG, "Subscribe to topic \"%s\" fail!", config->mqtt_sub);
          }
        }
      } else {
        ESP_LOGE(TAG, "Connect to broker fail (%d)!", mqtt->state());
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(RECONNECT_TIMEOUT)))
          break;
      }
    }
  }
  vTaskDelete(NULL);
}

bool mqttIsActive(void) {
  return (mqtt != nullptr) && (mqtt_task != nullptr) && mqtt->connected();
}

bool mqttInit(void) {
  constexpr uint32_t TASK_STACK = 4096;
  constexpr UBaseType_t TASK_PRIORITY = tskIDLE_PRIORITY + 5;

  if ((! mqtt) && (! mqtt_task)) {
    if (config->mqtt_ssl)
      tcp = new NetworkClientSecure();
    else
      tcp = new NetworkClient();
    if (tcp) {
      if (config->mqtt_ssl)
        ((NetworkClientSecure*)tcp)->setInsecure();
      mqtt = new PubSubClient(*tcp);
      if (mqtt) {
        mqtt->setServer(config->mqtt_server, config->mqtt_port);
        mqtt->setCallback(mqttCallback);
        if (xTaskCreate(mqttTask, "mqtt_task", TASK_STACK, mqtt, TASK_PRIORITY, &mqtt_task) == pdPASS) {
          return true;
        } else {
          mqtt_task = nullptr;
          ESP_LOGE(TAG, "Error creating task!");
        }
        delete mqtt;
        mqtt = nullptr;
      } else {
        ESP_LOGE(TAG, "Error creating PubSubClient instance!");
      }
      delete tcp;
      tcp = nullptr;
    } else {
      ESP_LOGE(TAG, "Error creating SSL client instance!");
    }
  }
  return false;
}

void mqttDone(void) {
  if (mqtt_task) {
    xTaskNotifyGive(mqtt_task);
    mqtt_task = nullptr;
  }
  if (mqtt) {
    delete mqtt;
    mqtt = nullptr;
  }
  if (tcp) {
    delete tcp;
    tcp = nullptr;
  }
}

bool mqttPublish(const char *payload) {
  if (mqtt && *config->mqtt_pub) {
    if (mqtt->publish(config->mqtt_pub, payload, config->mqtt_retain)) {
      return true;
    } else {
      ESP_LOGE(TAG, "Error publishing topic!");
    }
  }
  return false;
}
#endif
