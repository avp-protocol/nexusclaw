/**
 * @file avp_hw.h
 * @brief Hardware abstraction for AVP on NexusClaw
 *
 * Provides hardware-specific implementations for RNG, timers, and
 * TROPIC01 communication.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025 AVP Protocol Contributors
 */

#ifndef AVP_HW_H
#define AVP_HW_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize AVP hardware (RNG, etc.)
 */
void avp_hw_init(void);

/**
 * @brief Generate random bytes using hardware RNG
 *
 * @param buf  Output buffer
 * @param len  Number of bytes to generate
 */
void avp_hw_random_bytes(uint8_t *buf, size_t len);

/**
 * @brief Get current timestamp in seconds
 *
 * @return Current time in seconds since boot
 */
uint32_t avp_hw_get_time(void);

#ifdef __cplusplus
}
#endif

#endif /* AVP_HW_H */
