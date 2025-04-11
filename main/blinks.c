#include <stdlib.h>
#include <soc/ledc_reg.h>
#include <driver/ledc.h>
#include <esp_log.h>
#include "blinks.h"

#if ! (defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C2) || defined(CONFIG_IDF_TARGET_ESP32C3))
    #error "Unsupported CPU!"
#endif

#if MAX_BLINKS > SOC_LEDC_CHANNEL_NUM
    #error "Too many blinks!"
#endif

#define BLINK_SPEED     LEDC_LOW_SPEED_MODE
#define BLINK_TIMER     (LEDC_TIMER_MAX - 1)

static const char *TAG = "Blinks";

typedef struct __attribute__((__packed__)) blinks_struct_t {
    struct __attribute__((__packed__)) {
        blink_mode_t mode: 4;
        uint16_t bright : 12;
    } items[MAX_BLINKS];
} blinks_struct_t;

static void blinks_isr(void *arg) {
    blinks_struct_t *blinks = (blinks_struct_t*)arg;
    uint32_t st = READ_PERI_REG(LEDC_INT_ST_REG);

    for (uint8_t i = 0; i < MAX_BLINKS; ++i) {
#if defined(CONFIG_IDF_TARGET_ESP32)
        if ((blinks->items[i].mode > BLINK_TOGGLE) && (st & (1 << (LEDC_DUTY_CHNG_END_LSCH0_INT_ST_S + i)))) {
            if (READ_PERI_REG(LEDC_LSCH0_DUTY_R_REG + 0x10 * i)) {
                if (blinks->items[i].mode != BLINK_FADEIN)
                    WRITE_PERI_REG(LEDC_LSCH0_DUTY_REG + 0x10 * i, blinks->items[i].bright << 4);
                else
                    WRITE_PERI_REG(LEDC_LSCH0_DUTY_REG + 0x10 * i, 0 << 4);
                if (blinks->items[i].mode < BLINK_FADEIN)
                    WRITE_PERI_REG(LEDC_LSCH0_CONF1_REG + 0x10 * i, (1 << LEDC_DUTY_START_LSCH0_S) | (0 << LEDC_DUTY_INC_LSCH0_S) | (1 << LEDC_DUTY_NUM_LSCH0_S) | ((BLINK_TIME - 1) << LEDC_DUTY_CYCLE_LSCH0_S) | (blinks->items[i].bright << LEDC_DUTY_SCALE_LSCH0_S));
                else {
                    if (blinks->items[i].mode == BLINK_FADEIN)
                        WRITE_PERI_REG(LEDC_LSCH0_CONF1_REG + 0x10 * i, (1 << LEDC_DUTY_START_LSCH0_S) | (1 << LEDC_DUTY_INC_LSCH0_S) | (blinks->items[i].bright << LEDC_DUTY_NUM_LSCH0_S) | (8 << LEDC_DUTY_CYCLE_LSCH0_S) | (1 << LEDC_DUTY_SCALE_LSCH0_S));
                    else // _blink == BLINK_BREATH
                        WRITE_PERI_REG(LEDC_LSCH0_CONF1_REG + 0x10 * i, (1 << LEDC_DUTY_START_LSCH0_S) | (0 << LEDC_DUTY_INC_LSCH0_S) | (blinks->items[i].bright << LEDC_DUTY_NUM_LSCH0_S) | (8 << LEDC_DUTY_CYCLE_LSCH0_S) | (1 << LEDC_DUTY_SCALE_LSCH0_S));
                }
            } else {
                if (blinks->items[i].mode != BLINK_FADEOUT)
                    WRITE_PERI_REG(LEDC_LSCH0_DUTY_REG + 0x10 * i, 0 << 4);
                else
                    WRITE_PERI_REG(LEDC_LSCH0_DUTY_REG + 0x10 * i, blinks->items[i].bright << 4);
                if (blinks->items[i].mode < BLINK_FADEIN)
                    WRITE_PERI_REG(LEDC_LSCH0_CONF1_REG + 0x10 * i, (1 << LEDC_DUTY_START_LSCH0_S) | (1 << LEDC_DUTY_INC_LSCH0_S) | (1 << LEDC_DUTY_NUM_LSCH0_S) | (((250 << (blinks->items[i].mode - BLINK_4HZ)) - BLINK_TIME - 1) << LEDC_DUTY_CYCLE_LSCH0_S) | (blinks->items[i].bright << LEDC_DUTY_SCALE_LSCH0_S));
                else {
                    if (blinks->items[i].mode == BLINK_FADEOUT)
                        WRITE_PERI_REG(LEDC_LSCH0_CONF1_REG + 0x10 * i, (1 << LEDC_DUTY_START_LSCH0_S) | (0 << LEDC_DUTY_INC_LSCH0_S) | (blinks->items[i].bright << LEDC_DUTY_NUM_LSCH0_S) | (8 << LEDC_DUTY_CYCLE_LSCH0_S) | (1 << LEDC_DUTY_SCALE_LSCH0_S));
                    else // _blink == BLINK_BREATH
                        WRITE_PERI_REG(LEDC_LSCH0_CONF1_REG + 0x10 * i, (1 << LEDC_DUTY_START_LSCH0_S) | (1 << LEDC_DUTY_INC_LSCH0_S) | (blinks->items[i].bright << LEDC_DUTY_NUM_LSCH0_S) | (8 << LEDC_DUTY_CYCLE_LSCH0_S) | (1 << LEDC_DUTY_SCALE_LSCH0_S));
                }
            }
            SET_PERI_REG_MASK(LEDC_LSCH0_CONF0_REG + 0x10 * i, (1 << LEDC_PARA_UP_LSCH0_S));
        }
#elif defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
        if ((blinks->items[i].mode > BLINK_TOGGLE) && (st & (1 << (LEDC_DUTY_CHNG_END_LSCH0_INT_ST_S + i)))) {
            if (READ_PERI_REG(LEDC_LSCH0_DUTY_R_REG + 0x14 * i)) {
                if (blinks->items[i].mode != BLINK_FADEIN)
                    WRITE_PERI_REG(LEDC_LSCH0_DUTY_REG + 0x14 * i, blinks->items[i].bright << 4);
                else
                    WRITE_PERI_REG(LEDC_LSCH0_DUTY_REG + 0x14 * i, 0 << 4);
                if (blinks->items[i].mode < BLINK_FADEIN)
                    WRITE_PERI_REG(LEDC_LSCH0_CONF1_REG + 0x14 * i, (1 << LEDC_DUTY_START_LSCH0_S) | (0 << LEDC_DUTY_INC_LSCH0_S) | (1 << LEDC_DUTY_NUM_LSCH0_S) | ((BLINK_TIME - 1) << LEDC_DUTY_CYCLE_LSCH0_S) | (blinks->items[i].bright << LEDC_DUTY_SCALE_LSCH0_S));
                else {
                    if (blinks->items[i].mode == BLINK_FADEIN)
                        WRITE_PERI_REG(LEDC_LSCH0_CONF1_REG + 0x14 * i, (1 << LEDC_DUTY_START_LSCH0_S) | (1 << LEDC_DUTY_INC_LSCH0_S) | (blinks->items[i].bright << LEDC_DUTY_NUM_LSCH0_S) | (8 << LEDC_DUTY_CYCLE_LSCH0_S) | (1 << LEDC_DUTY_SCALE_LSCH0_S));
                    else // _blink == BLINK_BREATH
                        WRITE_PERI_REG(LEDC_LSCH0_CONF1_REG + 0x14 * i, (1 << LEDC_DUTY_START_LSCH0_S) | (0 << LEDC_DUTY_INC_LSCH0_S) | (blinks->items[i].bright << LEDC_DUTY_NUM_LSCH0_S) | (8 << LEDC_DUTY_CYCLE_LSCH0_S) | (1 << LEDC_DUTY_SCALE_LSCH0_S));
                }
            } else {
                if (blinks->items[i].mode != BLINK_FADEOUT)
                    WRITE_PERI_REG(LEDC_LSCH0_DUTY_REG + 0x14 * i, 0 << 4);
                else
                    WRITE_PERI_REG(LEDC_LSCH0_DUTY_REG + 0x14 * i, blinks->items[i].bright << 4);
                if (blinks->items[i].mode < BLINK_FADEIN)
                    WRITE_PERI_REG(LEDC_LSCH0_CONF1_REG + 0x14 * i, (1 << LEDC_DUTY_START_LSCH0_S) | (1 << LEDC_DUTY_INC_LSCH0_S) | (1 << LEDC_DUTY_NUM_LSCH0_S) | (((250 << (blinks->items[i].mode - BLINK_4HZ)) - BLINK_TIME - 1) << LEDC_DUTY_CYCLE_LSCH0_S) | (blinks->items[i].bright << LEDC_DUTY_SCALE_LSCH0_S));
                else {
                    if (blinks->items[i].mode == BLINK_FADEOUT)
                        WRITE_PERI_REG(LEDC_LSCH0_CONF1_REG + 0x14 * i, (1 << LEDC_DUTY_START_LSCH0_S) | (0 << LEDC_DUTY_INC_LSCH0_S) | (blinks->items[i].bright << LEDC_DUTY_NUM_LSCH0_S) | (8 << LEDC_DUTY_CYCLE_LSCH0_S) | (1 << LEDC_DUTY_SCALE_LSCH0_S));
                    else // _blink == BLINK_BREATH
                        WRITE_PERI_REG(LEDC_LSCH0_CONF1_REG + 0x14 * i, (1 << LEDC_DUTY_START_LSCH0_S) | (1 << LEDC_DUTY_INC_LSCH0_S) | (blinks->items[i].bright << LEDC_DUTY_NUM_LSCH0_S) | (8 << LEDC_DUTY_CYCLE_LSCH0_S) | (1 << LEDC_DUTY_SCALE_LSCH0_S));
                }
            }
            SET_PERI_REG_MASK(LEDC_LSCH0_CONF0_REG + 0x14 * i, (1 << LEDC_PARA_UP_LSCH0_S));
        }
#else
        if ((blinks->items[i].mode > BLINK_TOGGLE) && (st & (1 << (LEDC_DUTY_CHNG_END_CH0_INT_ST_S + i)))) {
            if (READ_PERI_REG(LEDC_CH0_DUTY_R_REG + 0x14 * i)) {
                if (blinks->items[i].mode != BLINK_FADEIN)
                    WRITE_PERI_REG(LEDC_CH0_DUTY_REG + 0x14 * i, blinks->items[i].bright << 4);
                else
                    WRITE_PERI_REG(LEDC_CH0_DUTY_REG + 0x14 * i, 0 << 4);
                if (blinks->items[i].mode < BLINK_FADEIN)
                    WRITE_PERI_REG(LEDC_CH0_CONF1_REG + 0x14 * i, (1 << LEDC_DUTY_START_CH0_S) | (0 << LEDC_DUTY_INC_CH0_S) | (1 << LEDC_DUTY_NUM_CH0_S) | ((BLINK_TIME - 1) << LEDC_DUTY_CYCLE_CH0_S) | (blinks->items[i].bright << LEDC_DUTY_SCALE_CH0_S));
                else {
                    if (blinks->items[i].mode == BLINK_FADEIN)
                        WRITE_PERI_REG(LEDC_CH0_CONF1_REG + 0x14 * i, (1 << LEDC_DUTY_START_CH0_S) | (1 << LEDC_DUTY_INC_CH0_S) | (blinks->items[i].bright << LEDC_DUTY_NUM_CH0_S) | (8 << LEDC_DUTY_CYCLE_CH0_S) | (1 << LEDC_DUTY_SCALE_CH0_S));
                    else // _blink == BLINK_BREATH
                        WRITE_PERI_REG(LEDC_CH0_CONF1_REG + 0x14 * i, (1 << LEDC_DUTY_START_CH0_S) | (0 << LEDC_DUTY_INC_CH0_S) | (blinks->items[i].bright << LEDC_DUTY_NUM_CH0_S) | (8 << LEDC_DUTY_CYCLE_CH0_S) | (1 << LEDC_DUTY_SCALE_CH0_S));
                }
            } else {
                if (blinks->items[i].mode != BLINK_FADEOUT)
                    WRITE_PERI_REG(LEDC_CH0_DUTY_REG + 0x14 * i, 0 << 4);
                else
                    WRITE_PERI_REG(LEDC_CH0_DUTY_REG + 0x14 * i, blinks->items[i].bright << 4);
                if (blinks->items[i].mode < BLINK_FADEIN)
                    WRITE_PERI_REG(LEDC_CH0_CONF1_REG + 0x14 * i, (1 << LEDC_DUTY_START_CH0_S) | (1 << LEDC_DUTY_INC_CH0_S) | (1 << LEDC_DUTY_NUM_CH0_S) | (((250 << (blinks->items[i].mode - BLINK_4HZ)) - BLINK_TIME - 1) << LEDC_DUTY_CYCLE_CH0_S) | (blinks->items[i].bright << LEDC_DUTY_SCALE_CH0_S));
                else {
                    if (blinks->items[i].mode == BLINK_FADEOUT)
                        WRITE_PERI_REG(LEDC_CH0_CONF1_REG + 0x14 * i, (1 << LEDC_DUTY_START_CH0_S) | (0 << LEDC_DUTY_INC_CH0_S) | (blinks->items[i].bright << LEDC_DUTY_NUM_CH0_S) | (8 << LEDC_DUTY_CYCLE_CH0_S) | (1 << LEDC_DUTY_SCALE_CH0_S));
                    else // _blink == BLINK_BREATH
                        WRITE_PERI_REG(LEDC_CH0_CONF1_REG + 0x14 * i, (1 << LEDC_DUTY_START_CH0_S) | (1 << LEDC_DUTY_INC_CH0_S) | (blinks->items[i].bright << LEDC_DUTY_NUM_CH0_S) | (8 << LEDC_DUTY_CYCLE_CH0_S) | (1 << LEDC_DUTY_SCALE_CH0_S));
                }
            }
            SET_PERI_REG_MASK(LEDC_CH0_CONF0_REG + 0x14 * i, (1 << LEDC_PARA_UP_CH0_S));
        }
#endif
    }
    WRITE_PERI_REG(LEDC_INT_CLR_REG, st);
}

blinks_handle_t blinks_init(void) {
    blinks_handle_t result;

    result = (blinks_handle_t)malloc(sizeof(blinks_struct_t));
    if (result) {
        ledc_isr_handle_t isr_handle;

        if (ledc_isr_register(&blinks_isr, result, 0, &isr_handle) == ESP_OK) {
            const ledc_timer_config_t timer_cfg = {
                .speed_mode = BLINK_SPEED,
                .duty_resolution = LEDC_TIMER_8_BIT,
                .timer_num = BLINK_TIMER,
                .freq_hz = 1000,
                .clk_cfg = LEDC_USE_RC_FAST_CLK, // LEDC_AUTO_CLK
//                .deconfigure = false
            };

            if (ledc_timer_config(&timer_cfg) == ESP_OK) {
                for (uint8_t i = 0; i < MAX_BLINKS; ++i) {
                    result->items[i].mode = BLINK_UNDEFINED;
                }
                return result;
            }
            esp_intr_free(isr_handle);
        }
        free(result);
    } else {
        ESP_LOGE(TAG, "Not enough memory!");
    }
    return NULL;
}

esp_err_t blinks_add(blinks_handle_t handle, gpio_num_t pin, bool level, uint8_t *index) {
    esp_err_t result = ESP_ERR_INVALID_ARG;

    if (handle) {
        for (uint8_t i = 0; i < MAX_BLINKS; ++i) {
            if (handle->items[i].mode == BLINK_UNDEFINED) {
                const ledc_channel_config_t channel_cfg = {
                    .gpio_num = pin,
                    .speed_mode = BLINK_SPEED,
                    .channel = LEDC_CHANNEL_0 + i,
                    .intr_type = LEDC_INTR_DISABLE,
                    .timer_sel = BLINK_TIMER,
                    .duty = 0,
                    .hpoint = 0,
                    .sleep_mode = LEDC_SLEEP_MODE_KEEP_ALIVE,
                    .flags.output_invert = ! level
                };
    
                result = ledc_channel_config(&channel_cfg);
                if (result == ESP_OK) {
                    handle->items[i].mode = BLINK_OFF;
                    handle->items[i].bright = 0;
                    if (index)
                        *index = i;
                }
                return ESP_OK;
            }
        }
        ESP_LOGE(TAG, "Maximum blinks already added!");
    }
    return result;
}

esp_err_t blinks_update(blinks_handle_t handle, uint8_t index, blink_mode_t mode, uint16_t bright) {
    esp_err_t result = ESP_ERR_INVALID_ARG;

    if (handle) {
        if ((index < MAX_BLINKS) && (handle->items[index].mode != BLINK_UNDEFINED) && (mode > BLINK_UNDEFINED) && (mode <= BLINK_BREATH)) {
            if (bright > 255) // 8 bit
                bright = 255;
            if ((mode == BLINK_TOGGLE) || (handle->items[index].mode != mode) || (handle->items[index].bright != bright)) {
                if (mode <= BLINK_TOGGLE) {
#if defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
                    CLEAR_PERI_REG_MASK(LEDC_INT_ENA_REG, 1 << (LEDC_DUTY_CHNG_END_LSCH0_INT_ENA_S + index));
#else
                    CLEAR_PERI_REG_MASK(LEDC_INT_ENA_REG, 1 << (LEDC_DUTY_CHNG_END_CH0_INT_ENA_S + index));
#endif
                    if (mode == BLINK_OFF)
                        result = ledc_set_duty(BLINK_SPEED, LEDC_CHANNEL_0 + index, 0);
                    else if (mode == BLINK_ON)
                        result = ledc_set_duty(BLINK_SPEED, LEDC_CHANNEL_0 + index, bright);
                    else // mode == BLINK_TOGGLE
                        result = ledc_set_duty(BLINK_SPEED, LEDC_CHANNEL_0 + index, ledc_get_duty(BLINK_SPEED, LEDC_CHANNEL_0 + index) ? 0 : bright);
                    if (result != ESP_OK)
                        ESP_LOGE(TAG, "LEDC set duty error %d!", result);
                } else {
                    if (mode < BLINK_FADEIN)
                        result = ledc_set_fade(BLINK_SPEED, LEDC_CHANNEL_0 + index, bright, LEDC_DUTY_DIR_DECREASE, 1, BLINK_TIME - 1, bright);
                    else if (mode == BLINK_FADEOUT)
                        result = ledc_set_fade(BLINK_SPEED, LEDC_CHANNEL_0 + index, bright, LEDC_DUTY_DIR_DECREASE, bright, 8, 1);
                    else // (_blink == BLINK_FADEIN) || (_blink == BLINK_BREATH)
                        result = ledc_set_fade(BLINK_SPEED, LEDC_CHANNEL_0 + index, 0, LEDC_DUTY_DIR_INCREASE, bright, 8, 1);
                    if (result != ESP_OK) 
                        ESP_LOGE(TAG, "LEDC set fade error %d!", result);
#if defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
                    SET_PERI_REG_MASK(LEDC_INT_ENA_REG, 1 << (LEDC_DUTY_CHNG_END_LSCH0_INT_ENA_S + index));
#else
                    SET_PERI_REG_MASK(LEDC_INT_ENA_REG, 1 << (LEDC_DUTY_CHNG_END_CH0_INT_ENA_S + index));
#endif
                }
                if (result == ESP_OK)
                    result = ledc_update_duty(BLINK_SPEED, LEDC_CHANNEL_0 + index);
                else
                    ESP_LOGE(TAG, "LEDC update duty error %d!", result);
                handle->items[index].mode = mode;
                handle->items[index].bright = bright;
            } else
                result = ESP_OK;
        }
    }
    return result;
}
