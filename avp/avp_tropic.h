/**
 * @file avp_tropic.h
 * @brief libtropic integration for AVP on NexusClaw
 *
 * Provides the bridge between AVP protocol operations and
 * libtropic TROPIC01 secure element functions.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025 AVP Protocol Contributors
 */

#ifndef AVP_TROPIC_H
#define AVP_TROPIC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "avp.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * TROPIC01 Slot Allocation
 *============================================================================*/

/* Slot ranges for different purposes */
#define AVP_SLOT_SECRETS_START      96   /* Data slots for secrets */
#define AVP_SLOT_SECRETS_END        127
#define AVP_SLOT_KEYS_START         0    /* ECC key slots */
#define AVP_SLOT_KEYS_END           31

/*============================================================================
 * TROPIC01 Interface
 *============================================================================*/

/**
 * @brief Initialize TROPIC01 connection
 *
 * @param ctx   AVP context to attach TROPIC01 handle
 * @return AVP_OK on success
 */
avp_ret_t avp_tropic_init(avp_ctx_t *ctx);

/**
 * @brief Deinitialize TROPIC01 connection
 *
 * @param ctx   AVP context
 */
void avp_tropic_deinit(avp_ctx_t *ctx);

/**
 * @brief Verify PIN with TROPIC01
 *
 * @param ctx       AVP context
 * @param pin       PIN string (4-6 digits)
 * @param attempts  Remaining attempts on failure
 * @return AVP_OK on success, AVP_ERR_PIN_INVALID or AVP_ERR_PIN_LOCKED on failure
 */
avp_ret_t avp_tropic_verify_pin(avp_ctx_t *ctx, const char *pin, uint8_t *attempts);

/**
 * @brief Store data in TROPIC01 slot
 *
 * @param ctx       AVP context
 * @param slot      Slot index (96-127 for data)
 * @param data      Data to store
 * @param len       Data length
 * @return AVP_OK on success
 */
avp_ret_t avp_tropic_store(avp_ctx_t *ctx, uint8_t slot, const uint8_t *data, size_t len);

/**
 * @brief Retrieve data from TROPIC01 slot
 *
 * @param ctx       AVP context
 * @param slot      Slot index
 * @param data      Output buffer
 * @param len       Input: buffer size, Output: data length
 * @return AVP_OK on success
 */
avp_ret_t avp_tropic_retrieve(avp_ctx_t *ctx, uint8_t slot, uint8_t *data, size_t *len);

/**
 * @brief Erase TROPIC01 slot
 *
 * @param ctx       AVP context
 * @param slot      Slot index
 * @return AVP_OK on success
 */
avp_ret_t avp_tropic_erase(avp_ctx_t *ctx, uint8_t slot);

/**
 * @brief Sign data with TROPIC01 ECC key
 *
 * @param ctx           AVP context
 * @param key_slot      Key slot index (0-31)
 * @param data          Data to sign (or hash)
 * @param data_len      Data length
 * @param signature     Output signature buffer (64 bytes for P-256/Ed25519)
 * @param sig_len       Input: buffer size, Output: signature length
 * @return AVP_OK on success
 */
avp_ret_t avp_tropic_sign(avp_ctx_t *ctx, uint8_t key_slot,
                          const uint8_t *data, size_t data_len,
                          uint8_t *signature, size_t *sig_len);

/**
 * @brief Get TROPIC01 device information
 *
 * @param ctx           AVP context
 * @param serial        Output: Serial number string (32 bytes max)
 * @param fw_version    Output: Firmware version string (16 bytes max)
 * @return AVP_OK on success
 */
avp_ret_t avp_tropic_get_info(avp_ctx_t *ctx, char *serial, char *fw_version);

/**
 * @brief Perform device attestation
 *
 * @param ctx           AVP context
 * @param challenge     32-byte challenge
 * @param response      Output: Attestation response (signature)
 * @param resp_len      Output: Response length
 * @return AVP_OK on success
 */
avp_ret_t avp_tropic_attest(avp_ctx_t *ctx, const uint8_t *challenge,
                            uint8_t *response, size_t *resp_len);

#ifdef __cplusplus
}
#endif

#endif /* AVP_TROPIC_H */
