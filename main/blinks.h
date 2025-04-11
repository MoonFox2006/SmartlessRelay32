#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdbool.h>
#include <driver/gpio.h>

#ifndef MAX_BLINKS
#define MAX_BLINKS  1
#endif

#ifndef BLINK_TIME
#define BLINK_TIME  25 // 25 ms.
#endif

typedef enum blink_mode_t { BLINK_UNDEFINED, BLINK_OFF, BLINK_ON, BLINK_TOGGLE, BLINK_4HZ, BLINK_2HZ, BLINK_1HZ, BLINK_FADEIN, BLINK_FADEOUT, BLINK_BREATH } blink_mode_t;

typedef struct blinks_struct_t *blinks_handle_t;

blinks_handle_t blinks_init(void);
esp_err_t blinks_add(blinks_handle_t handle, gpio_num_t pin, bool level, uint8_t *index);
esp_err_t blinks_update(blinks_handle_t handle, uint8_t index, blink_mode_t mode, uint16_t bright);

#ifdef __cplusplus
}
#endif
