#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <esp_err.h>
#include <driver/gpio.h>

#ifndef MAX_BUTTONS
#define MAX_BUTTONS         1
#endif
#ifndef TICK_TIME
#define TICK_TIME           5
#endif
#ifndef CLICK_TIME
#define CLICK_TIME          50
#endif
#ifndef LONGCLICK_TIME
#define LONGCLICK_TIME      1000
#endif
#ifndef VERYLONGCLICK_TIME
#define VERYLONGCLICK_TIME  5000
#endif
#ifndef MULTICLICK_TIME
#define MULTICLICK_TIME     350
#endif

#define FOREVER     ((uint32_t)-1)

typedef struct buttons_struct_t *buttons_handle_t;

typedef struct __attribute__((__packed__)) buttons_config_t {
    uint16_t queue_size;
} buttons_config_t;

#define BUTTONS_CONFIG_DEFAULT()    {\
    .queue_size = 32\
}

typedef enum button_state_t { BTN_PRESS, BTN_LONGPRESS, BTN_VERYLONGPRESS, BTN_CLICK, BTN_LONGCLICK, BTN_VERYLONGCLICK } button_state_t;

typedef struct __attribute__((__packed__)) button_event_t {
#if MAX_BUTTONS > 1
    uint8_t index;
#endif
    button_state_t state : 4;
    uint8_t clicks : 4;
} button_event_t;

buttons_handle_t buttons_init(const buttons_config_t *buttons_cfg);
void buttons_done(buttons_handle_t handle);
esp_err_t buttons_add(buttons_handle_t handle, gpio_num_t pin, bool level, bool pull_up, uint8_t *index);
esp_err_t buttons_pause(buttons_handle_t handle, uint8_t index);
esp_err_t buttons_resume(buttons_handle_t handle, uint8_t index);
esp_err_t buttons_start(buttons_handle_t handle);
esp_err_t buttons_stop(buttons_handle_t handle);
void buttons_clear_events(buttons_handle_t handle);
bool buttons_has_events(buttons_handle_t handle);
bool buttons_get_event(buttons_handle_t handle, button_event_t *evt, uint32_t timeout);
bool buttons_peek_event(buttons_handle_t handle, button_event_t *evt, uint32_t timeout);

#ifdef __cplusplus
}
#endif
