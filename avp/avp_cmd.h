/**
 * @file avp_cmd.h
 * @brief AVP command handler for NexusClaw USB interface
 *
 * This module integrates AVP protocol processing with the USB CDC command
 * interface, allowing the device to accept AVP JSON commands over serial.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025 AVP Protocol Contributors
 */

#ifndef AVP_CMD_H
#define AVP_CMD_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize AVP command handler
 *
 * Should be called once during startup after TROPIC01 is initialized.
 */
void avp_cmd_init(void);

/**
 * @brief Check if input is an AVP JSON command
 *
 * AVP commands start with '{' and are valid JSON.
 *
 * @param data Input string
 * @return true if this looks like an AVP command
 */
bool avp_cmd_is_avp(const char *data);

/**
 * @brief Process an AVP command and send response
 *
 * Parses the JSON command, executes the operation, and prints
 * the JSON response to stdout (USB CDC).
 *
 * @param data JSON command string
 */
void avp_cmd_process(const char *data);

#ifdef __cplusplus
}
#endif

#endif /* AVP_CMD_H */
