#include <stdlib.h>
#include <soc/gpio_reg.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "buttons.h"

static const char *TAG = "Buttons";

typedef struct buttons_struct_t {
    struct __attribute__((__packed__)) {
        uint8_t pin : 6;
        bool level : 1;
        bool paused : 1;
        volatile bool pressed : 1;
        volatile uint8_t clicks : 4;
        volatile uint32_t ticks : 19;
    } items[MAX_BUTTONS];
    esp_timer_handle_t timer;
    QueueHandle_t queue;
} buttons_struct_t;

static void timer_cb(void *user_data) {
    buttons_struct_t *buttons = (buttons_struct_t*)user_data;
#if GPIO_NUM_MAX > 32
    uint64_t inputs = (READ_PERI_REG(GPIO_IN1_REG) << 32) | READ_PERI_REG(GPIO_IN_REG);
#else
    uint32_t inputs = READ_PERI_REG(GPIO_IN_REG);
#endif

    for (uint8_t i = 0; i < MAX_BUTTONS; ++i) {
        if ((buttons->items[i].pin != 0x3F) && (! buttons->items[i].paused)) {
            button_event_t evt;
    
#if MAX_BUTTONS > 1
            evt.index = i;
#endif
            if (buttons->items[i].pressed) { // Was pressed
                if (buttons->items[i].ticks < 0x07FFFF) // 19 bit
                    ++buttons->items[i].ticks;
            } else { // Was released
                if (buttons->items[i].ticks > 0)
                    --buttons->items[i].ticks;
            }
            if (((inputs >> buttons->items[i].pin) & 0x01) == buttons->items[i].level) { // Button pressed
                if (buttons->items[i].pressed) { // Was pressed
                    evt.state = BTN_CLICK; // Illegal state
                    if (buttons->items[i].ticks == CLICK_TIME / TICK_TIME)
                        evt.state = BTN_PRESS;
                    else if (buttons->items[i].ticks == LONGCLICK_TIME / TICK_TIME)
                        evt.state = BTN_LONGPRESS;
                    else if (buttons->items[i].ticks == VERYLONGCLICK_TIME / TICK_TIME)
                        evt.state = BTN_VERYLONGPRESS;
                    if (evt.state != BTN_CLICK) {
                        evt.clicks = buttons->items[i].clicks;
                        xQueueSendFromISR(buttons->queue, &evt, NULL);
                    }
                } else { // Just pressed
                    if (buttons->items[i].ticks) {
                        if (buttons->items[i].clicks < 15) // 4 bits
                            ++buttons->items[i].clicks;
                    } else {
                        buttons->items[i].clicks = 0;
                    }
                    buttons->items[i].pressed = true;
                    buttons->items[i].ticks = 0;
                }
            } else { // Button released
                if (buttons->items[i].pressed) { // Just released
                    evt.state = BTN_PRESS; // Illegal state
                    if (buttons->items[i].ticks >= VERYLONGCLICK_TIME / TICK_TIME)
                        evt.state = BTN_VERYLONGCLICK;
                    else if (buttons->items[i].ticks >= LONGCLICK_TIME / TICK_TIME)
                        evt.state = BTN_LONGCLICK;
                    else if (buttons->items[i].ticks >= CLICK_TIME / TICK_TIME)
                        evt.state = BTN_CLICK;
                    if (evt.state != BTN_PRESS) {
                        evt.clicks = buttons->items[i].clicks;
                        xQueueSendFromISR(buttons->queue, &evt, NULL);
                    }
                    buttons->items[i].pressed = false;
                    buttons->items[i].ticks = MULTICLICK_TIME / TICK_TIME;
                }
            }
        }
    }
}

buttons_handle_t buttons_init(const buttons_config_t *buttons_cfg) {
    buttons_handle_t result;

    result = (buttons_handle_t)malloc(sizeof(buttons_struct_t));
    if (result) {
        for (uint8_t i = 0; i < MAX_BUTTONS; ++i) {
            result->items[i].pin = 0x3F; // 6 bit
        }
        result->queue = xQueueCreate(buttons_cfg->queue_size, sizeof(button_event_t));
        if (result->queue) {
            const esp_timer_create_args_t timer_args = {
                .callback = timer_cb,
                .arg = result
            };

            if (esp_timer_create(&timer_args, &result->timer) == ESP_OK) {
                return result;
            }
            vQueueDelete(result->queue);
        } else {
            ESP_LOGE(TAG, "Error creating event queue!");
        }
        free(result);
    } else {
        ESP_LOGE(TAG, "Not enough memory!");
    }
    return NULL;
}

void buttons_done(buttons_handle_t handle) {
    if (handle) {
        if (handle->timer) {
            esp_timer_stop(handle->timer);
            esp_timer_delete(handle->timer);
        }
        if (handle->queue) {
            vQueueDelete(handle->queue);
        }
        free(handle);
    }
}

esp_err_t buttons_add(buttons_handle_t handle, gpio_num_t pin, bool level, bool pull_up, uint8_t *index) {
    if (handle) {
        for (uint8_t i = 0; i < MAX_BUTTONS; ++i) {
            if (handle->items[i].pin == 0x3F) { // 6 bit
                gpio_reset_pin(pin);
                gpio_set_direction(pin, GPIO_MODE_INPUT);
                if (pull_up)
                    gpio_pullup_en(pin);
                else
                    gpio_pullup_dis(pin);
                handle->items[i].pin = pin;
                handle->items[i].level = level;
                handle->items[i].paused = false;
                handle->items[i].pressed = gpio_get_level(pin) == level;
                handle->items[i].clicks = 0;
                handle->items[i].ticks = 0;
                if (index)
                    *index = i;
                return ESP_OK;
            }
        }
        ESP_LOGE(TAG, "Maximum buttons already added!");
    }
    return ESP_ERR_INVALID_ARG;
}

esp_err_t buttons_pause(buttons_handle_t handle, uint8_t index) {
    if (handle) {
        if (index < MAX_BUTTONS) {
            handle->items[index].paused = true;
            return ESP_OK;
        }
    }
    return ESP_ERR_INVALID_ARG;
}

esp_err_t buttons_resume(buttons_handle_t handle, uint8_t index) {
    if (handle) {
        if (index < MAX_BUTTONS) {
            handle->items[index].paused = false;
            return ESP_OK;
        }
    }
    return ESP_ERR_INVALID_ARG;
}

esp_err_t buttons_start(buttons_handle_t handle) {
    if (handle) {
        if (handle->timer) {
            return esp_timer_start_periodic(handle->timer, TICK_TIME * 1000);
        }
    }
    return ESP_ERR_INVALID_ARG;
}

esp_err_t buttons_stop(buttons_handle_t handle) {
    if (handle) {
        if (handle->timer) {
            return esp_timer_stop(handle->timer);
        }
    }
    return ESP_ERR_INVALID_ARG;
}

void buttons_clear_events(buttons_handle_t handle) {
    if (handle) {
        if (handle->queue) {
            xQueueReset(handle->queue);
        }
    }
}

bool buttons_has_events(buttons_handle_t handle) {
    if (handle) {
        if (handle->queue) {
            return uxQueueMessagesWaiting(handle->queue) > 0;
        }
    }
    return false;
}

bool buttons_get_event(buttons_handle_t handle, button_event_t *evt, uint32_t timeout) {
    if (handle) {
        if (handle->queue) {
            return xQueueReceive(handle->queue, evt, timeout == FOREVER ? portMAX_DELAY : pdMS_TO_TICKS(timeout)) == pdTRUE;
        }
    }
    return false;
}

bool buttons_peek_event(buttons_handle_t handle, button_event_t *evt, uint32_t timeout) {
    if (handle) {
        if (handle->queue) {
            return xQueuePeek(handle->queue, evt, timeout == FOREVER ? portMAX_DELAY : pdMS_TO_TICKS(timeout)) == pdTRUE;
        }
    }
    return false;
}
