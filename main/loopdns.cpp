#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFi.h>
#include "loopdns.h"

// DNS Header Packet
typedef struct __attribute__((__packed__)) {
  uint16_t id;
  uint16_t flags;
  uint16_t qd_count;
  uint16_t an_count;
  uint16_t ns_count;
  uint16_t ar_count;
} dns_header_t;

// DNS Question Packet
typedef struct __attribute__((__packed__)) {
  uint16_t type;
  uint16_t cls;
} dns_question_t;

// DNS Answer Packet
typedef struct __attribute__((__packed__)) {
  uint16_t ptr_offset;
  uint16_t type;
  uint16_t cls;
  uint32_t ttl;
  uint16_t addr_len;
  uint32_t ip_addr;
} dns_answer_t;

static const char *TAG = "LoopDNS";

static WiFiUDP *udp = nullptr;
static TaskHandle_t dns_task = nullptr;

static void dnsTask(void *pvParameters) {
  constexpr uint16_t DNS_PORT = 53;

  WiFiUDP *udp = (WiFiUDP*)pvParameters;

  while (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1)) == 0) {
    if (udp->begin(DNS_PORT)) {
      uint8_t buffer[288];
      int len;

      while (ulTaskNotifyTake(pdFALSE, pdMS_TO_TICKS(1)) == 0) {
        len = udp->parsePacket();
        if (len != 0) {
          if (len < 0) {
            ESP_LOGE(TAG, "UDP parse packet error!");
            break;
          } else if ((len > sizeof(dns_header_t) + sizeof(dns_question_t)) && (len <= sizeof(buffer) - sizeof(dns_answer_t)) && (udp->read(buffer, len) == len)) { // Data received
            dns_header_t *dns_header = (dns_header_t*)buffer;
            const dns_question_t *dns_question = (dns_question_t*)&buffer[len - sizeof(dns_question_t)];
            dns_answer_t *dns_answer = (dns_answer_t*)&buffer[len];
  
            if ((! (dns_header->flags & ~(uint16_t)0x0001)) && (dns_header->qd_count == 0x0100) && (dns_question->type == 0x0100) && (dns_question->cls == 0x0100)) {
              dns_header->flags |= 0x0080; // QR
              dns_header->an_count = 0x0100; // 1
              dns_answer->ptr_offset = 0x0CC0; // +12
              dns_answer->type = dns_question->type;
              dns_answer->cls = dns_question->cls;
              dns_answer->ttl = 0x3C000000; // 60 sec.
              dns_answer->addr_len = 0x0400; // 4
              dns_answer->ip_addr = WiFi.softAPIP();
              len += sizeof(dns_answer_t);
              if ((! udp->beginPacket()) || (udp->write(buffer, len) != len) || (! udp->endPacket())) {
                ESP_LOGE(TAG, "UDP send error!");
                break;
              }
            }
          } else {
            ESP_LOGE(TAG, "Wrong packet received!");
          }
        }
      }
      udp->stop();
    } else {
      ESP_LOGE(TAG, "UDP begin error!");
    }
  }
  vTaskDelete(NULL);
}

bool loopdns_start(void) {
  constexpr uint32_t TASK_STACK = 4096;
  constexpr UBaseType_t TASK_PRIORITY = tskIDLE_PRIORITY + 5;

  if ((! udp) && (! dns_task)) {
    udp = new WiFiUDP();
    if (udp) {
      if (xTaskCreate(dnsTask, "dns_task", TASK_STACK, udp, TASK_PRIORITY, &dns_task) == pdPASS) {
        return true;
      } else {
        dns_task = nullptr;
        ESP_LOGE(TAG, "Error creating task!");
      }
      delete udp;
      udp = nullptr;
    } else {
      ESP_LOGE(TAG, "Error creating UDP client instance!");
    }
  }
  return false;
}

void loopdns_stop(void) {
  if (dns_task) {
    xTaskNotifyGive(dns_task);
    dns_task = nullptr;
  }
  if (udp) {
    delete udp;
    udp = nullptr;
  }
}
