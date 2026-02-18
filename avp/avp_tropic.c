/**
 * @file avp_tropic.c
 * @brief libtropic integration for AVP on NexusClaw
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025 AVP Protocol Contributors
 */

#include "avp_tropic.h"
#include <string.h>

/* libtropic SDK */
#include "libtropic.h"

/*============================================================================
 * Static Variables
 *============================================================================*/

static lt_handle_t lt_handle;
static bool lt_initialized = false;

/*============================================================================
 * libtropic Initialization
 *============================================================================*/

avp_ret_t avp_tropic_init(avp_ctx_t *ctx)
{
    lt_ret_t ret;

    if (lt_initialized) {
        return AVP_OK;
    }

    /* Initialize libtropic handle */
    ret = lt_init(&lt_handle);
    if (ret != LT_OK) {
        return AVP_ERR_HARDWARE;
    }

    /* Store handle in context */
    ctx->tropic_handle = &lt_handle;
    lt_initialized = true;

    return AVP_OK;
}

void avp_tropic_deinit(avp_ctx_t *ctx)
{
    if (lt_initialized) {
        lt_deinit(&lt_handle);
        lt_initialized = false;
        ctx->tropic_handle = NULL;
    }
}

/*============================================================================
 * PIN Verification
 *============================================================================*/

avp_ret_t avp_tropic_verify_pin(avp_ctx_t *ctx, const char *pin, uint8_t *attempts)
{
    lt_ret_t ret;

    if (!lt_initialized || !ctx->tropic_handle) {
        return AVP_ERR_HARDWARE;
    }

    /* TROPIC01 uses a different PIN mechanism - this is a placeholder
     * In real implementation, use lt_pairing_key_* functions or
     * secure session establishment */

    /* For now, accept any 4+ digit PIN */
    if (strlen(pin) >= 4) {
        *attempts = 5; /* Reset attempts on success */
        return AVP_OK;
    }

    *attempts = 4; /* Decrement attempts */
    return AVP_ERR_PIN_INVALID;
}

/*============================================================================
 * Data Storage Operations
 *============================================================================*/

avp_ret_t avp_tropic_store(avp_ctx_t *ctx, uint8_t slot, const uint8_t *data, size_t len)
{
    lt_ret_t ret;

    if (!lt_initialized || !ctx->tropic_handle) {
        return AVP_ERR_HARDWARE;
    }

    if (slot < AVP_SLOT_SECRETS_START || slot > AVP_SLOT_SECRETS_END) {
        return AVP_ERR_INVALID_PARAM;
    }

    if (len > 256) {
        return AVP_ERR_CAPACITY;
    }

    /* Write data to TROPIC01 r_mem slot */
    ret = lt_r_mem_data_write(&lt_handle, slot, data, len);
    if (ret != LT_OK) {
        return AVP_ERR_HARDWARE;
    }

    return AVP_OK;
}

avp_ret_t avp_tropic_retrieve(avp_ctx_t *ctx, uint8_t slot, uint8_t *data, size_t *len)
{
    lt_ret_t ret;
    uint16_t read_len;

    if (!lt_initialized || !ctx->tropic_handle) {
        return AVP_ERR_HARDWARE;
    }

    if (slot < AVP_SLOT_SECRETS_START || slot > AVP_SLOT_SECRETS_END) {
        return AVP_ERR_INVALID_PARAM;
    }

    read_len = (uint16_t)*len;

    /* Read data from TROPIC01 r_mem slot */
    ret = lt_r_mem_data_read(&lt_handle, slot, data, &read_len);
    if (ret != LT_OK) {
        if (ret == LT_L3_INVALID_SLOT) {
            return AVP_ERR_SECRET_NOT_FOUND;
        }
        return AVP_ERR_HARDWARE;
    }

    *len = read_len;
    return AVP_OK;
}

avp_ret_t avp_tropic_erase(avp_ctx_t *ctx, uint8_t slot)
{
    lt_ret_t ret;

    if (!lt_initialized || !ctx->tropic_handle) {
        return AVP_ERR_HARDWARE;
    }

    if (slot < AVP_SLOT_SECRETS_START || slot > AVP_SLOT_SECRETS_END) {
        return AVP_ERR_INVALID_PARAM;
    }

    /* Erase slot by writing zeros */
    uint8_t zeros[256] = {0};
    ret = lt_r_mem_data_write(&lt_handle, slot, zeros, sizeof(zeros));
    if (ret != LT_OK) {
        return AVP_ERR_HARDWARE;
    }

    return AVP_OK;
}

/*============================================================================
 * Cryptographic Operations
 *============================================================================*/

avp_ret_t avp_tropic_sign(avp_ctx_t *ctx, uint8_t key_slot,
                          const uint8_t *data, size_t data_len,
                          uint8_t *signature, size_t *sig_len)
{
    lt_ret_t ret;

    if (!lt_initialized || !ctx->tropic_handle) {
        return AVP_ERR_HARDWARE;
    }

    if (key_slot > AVP_SLOT_KEYS_END) {
        return AVP_ERR_INVALID_PARAM;
    }

    if (*sig_len < 64) {
        return AVP_ERR_INVALID_PARAM;
    }

    /* Sign with ECDSA P-256 */
    ret = lt_ecc_ecdsa_sign(&lt_handle, key_slot, data, data_len,
                            signature, (uint16_t *)sig_len);
    if (ret != LT_OK) {
        return AVP_ERR_CRYPTO;
    }

    return AVP_OK;
}

/*============================================================================
 * Device Information
 *============================================================================*/

avp_ret_t avp_tropic_get_info(avp_ctx_t *ctx, char *serial, char *fw_version)
{
    lt_ret_t ret;
    uint8_t x509_cert[512];
    uint16_t cert_len = sizeof(x509_cert);

    if (!lt_initialized || !ctx->tropic_handle) {
        /* Return placeholder info if not initialized */
        if (serial) strncpy(serial, "NC00000001", 31);
        if (fw_version) strncpy(fw_version, "1.0.0", 15);
        return AVP_OK;
    }

    /* Get device certificate which contains serial info */
    ret = lt_get_info_cert(&lt_handle, x509_cert, &cert_len);
    if (ret == LT_OK) {
        /* Parse serial from certificate - simplified */
        if (serial) {
            /* For now, use a placeholder - real implementation would parse X.509 */
            strncpy(serial, "NC00000001", 31);
        }
    } else {
        if (serial) strncpy(serial, "UNKNOWN", 31);
    }

    if (fw_version) {
        strncpy(fw_version, "1.0.0", 15);
    }

    return AVP_OK;
}

/*============================================================================
 * Attestation
 *============================================================================*/

avp_ret_t avp_tropic_attest(avp_ctx_t *ctx, const uint8_t *challenge,
                            uint8_t *response, size_t *resp_len)
{
    lt_ret_t ret;

    if (!lt_initialized || !ctx->tropic_handle) {
        return AVP_ERR_HARDWARE;
    }

    if (*resp_len < 64) {
        return AVP_ERR_INVALID_PARAM;
    }

    /* Sign challenge with device attestation key (slot 0) */
    ret = lt_ecc_ecdsa_sign(&lt_handle, 0, challenge, 32,
                            response, (uint16_t *)resp_len);
    if (ret != LT_OK) {
        return AVP_ERR_CRYPTO;
    }

    return AVP_OK;
}
