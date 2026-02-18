/**
 * @file avp_cmd.c
 * @brief AVP command handler for NexusClaw USB interface
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025 AVP Protocol Contributors
 */

#include "avp_cmd.h"
#include "avp.h"
#include "os.h"
#include <string.h>

/*============================================================================
 * External Dependencies
 *============================================================================*/

/* From main firmware */
extern uint32_t timer_get_time(void);

/* Hardware RNG - implemented elsewhere */
extern void hw_random_bytes(uint8_t *buf, size_t len);

/*============================================================================
 * Static Variables
 *============================================================================*/

static avp_ctx_t avp_ctx;
static char avp_response[AVP_MAX_JSON_LEN];

/*============================================================================
 * Platform Callbacks
 *============================================================================*/

static uint32_t get_timestamp(void)
{
    /* Convert milliseconds to seconds */
    return timer_get_time() / 1000;
}

static void get_random(uint8_t *buf, size_t len)
{
    /* Use hardware RNG */
    hw_random_bytes(buf, len);
}

/*============================================================================
 * Public API
 *============================================================================*/

void avp_cmd_init(void)
{
    /* Initialize AVP context */
    avp_init(&avp_ctx, NULL, get_timestamp, get_random);

    OS_PRINTF("# AVP Protocol v%s initialized\r\n", "0.1.0");
}

bool avp_cmd_is_avp(const char *data)
{
    /* Skip leading whitespace */
    while (*data == ' ' || *data == '\t' || *data == '\r' || *data == '\n') {
        data++;
    }

    /* AVP commands are JSON objects */
    return (*data == '{');
}

void avp_cmd_process(const char *data)
{
    avp_ret_t ret;

    /* Process the command */
    ret = avp_process(&avp_ctx, data, avp_response, sizeof(avp_response));

    if (ret != AVP_OK) {
        OS_PRINTF("{\"ok\":false,\"error\":\"INTERNAL_ERROR\"}\r\n");
        return;
    }

    /* Output the response */
    OS_PRINTF("%s\r\n", avp_response);
}
