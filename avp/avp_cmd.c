/**
 * @file avp_cmd.c
 * @brief AVP command handler for NexusClaw USB interface
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025 AVP Protocol Contributors
 */

#include "avp_cmd.h"
#include "avp.h"
#include "avp_hw.h"
#include "os.h"
#include <string.h>

/*============================================================================
 * Static Variables
 *============================================================================*/

static avp_ctx_t avp_ctx;
static char avp_response[AVP_MAX_JSON_LEN];

/*============================================================================
 * Public API
 *============================================================================*/

void avp_cmd_init(void)
{
    /* Initialize AVP hardware (RNG, etc.) */
    avp_hw_init();

    /* Initialize AVP context */
    avp_init(&avp_ctx, NULL, avp_hw_get_time, avp_hw_random_bytes);

    OS_PRINTF("# AVP Protocol v%s initialized\r\n", "0.1.0");
    OS_PRINTF("# NexusClaw ready\r\n");
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
