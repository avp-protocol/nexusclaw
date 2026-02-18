/**
 * @file avp_hw.c
 * @brief Hardware abstraction for AVP on NexusClaw
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025 AVP Protocol Contributors
 */

#include "avp_hw.h"
#include "stm32u5xx_hal.h"
#include "time.h"

/*============================================================================
 * Hardware RNG
 *============================================================================*/

static RNG_HandleTypeDef hrng;
static uint8_t rng_initialized = 0;

void avp_hw_init(void)
{
    if (rng_initialized) {
        return;
    }

    /* Enable RNG clock */
    __HAL_RCC_RNG_CLK_ENABLE();

    /* Configure RNG */
    hrng.Instance = RNG;
    hrng.Init.ClockErrorDetection = RNG_CED_ENABLE;

    if (HAL_RNG_Init(&hrng) != HAL_OK) {
        /* RNG init failed - fall back to pseudo-random */
        rng_initialized = 0;
        return;
    }

    rng_initialized = 1;
}

void avp_hw_random_bytes(uint8_t *buf, size_t len)
{
    uint32_t random_word;
    size_t i;

    if (!rng_initialized) {
        /* Fallback: use timer-based pseudo-random (not secure!) */
        uint32_t seed = timer_get_time();
        for (i = 0; i < len; i++) {
            seed = seed * 1103515245 + 12345;
            buf[i] = (uint8_t)(seed >> 16);
        }
        return;
    }

    /* Generate random bytes using hardware RNG */
    for (i = 0; i < len; ) {
        if (HAL_RNG_GenerateRandomNumber(&hrng, &random_word) == HAL_OK) {
            /* Copy up to 4 bytes from each random word */
            size_t remaining = len - i;
            size_t copy_len = (remaining >= 4) ? 4 : remaining;

            for (size_t j = 0; j < copy_len; j++) {
                buf[i + j] = (uint8_t)(random_word >> (j * 8));
            }
            i += copy_len;
        } else {
            /* RNG error - use timer fallback for remaining bytes */
            uint32_t seed = timer_get_time();
            while (i < len) {
                seed = seed * 1103515245 + 12345;
                buf[i++] = (uint8_t)(seed >> 16);
            }
            break;
        }
    }
}

uint32_t avp_hw_get_time(void)
{
    /* Convert milliseconds to seconds */
    return timer_get_time() / 1000;
}
