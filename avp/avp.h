/**
 * @file avp.h
 * @brief Agent Vault Protocol (AVP) implementation for NexusClaw
 *
 * This module implements the AVP protocol over USB CDC, translating
 * JSON commands to TROPIC01 secure element operations.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025 AVP Protocol Contributors
 */

#ifndef AVP_H
#define AVP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Configuration
 *============================================================================*/

/** Maximum length of AVP command/response JSON */
#define AVP_MAX_JSON_LEN        1024

/** Maximum length of secret name */
#define AVP_MAX_NAME_LEN        64

/** Maximum length of secret value (base64 encoded) */
#define AVP_MAX_VALUE_LEN       512

/** Maximum number of secrets */
#define AVP_MAX_SECRETS         32

/** Default session TTL in seconds */
#define AVP_DEFAULT_TTL         300

/** Session ID length */
#define AVP_SESSION_ID_LEN      32

/*============================================================================
 * Return Codes
 *============================================================================*/

typedef enum {
    AVP_OK = 0,
    AVP_ERR_PARSE,              /**< JSON parse error */
    AVP_ERR_INVALID_OP,         /**< Unknown operation */
    AVP_ERR_INVALID_PARAM,      /**< Invalid parameter */
    AVP_ERR_NOT_AUTHENTICATED,  /**< Session not established */
    AVP_ERR_SESSION_EXPIRED,    /**< Session timed out */
    AVP_ERR_SECRET_NOT_FOUND,   /**< Secret does not exist */
    AVP_ERR_CAPACITY,           /**< Storage full */
    AVP_ERR_HARDWARE,           /**< TROPIC01 error */
    AVP_ERR_CRYPTO,             /**< Cryptographic error */
    AVP_ERR_PIN_INVALID,        /**< Wrong PIN */
    AVP_ERR_PIN_LOCKED,         /**< Too many failed attempts */
    AVP_ERR_INTERNAL,           /**< Internal error */
} avp_ret_t;

/*============================================================================
 * Operations
 *============================================================================*/

typedef enum {
    AVP_OP_UNKNOWN = 0,
    AVP_OP_DISCOVER,
    AVP_OP_AUTHENTICATE,
    AVP_OP_STORE,
    AVP_OP_RETRIEVE,
    AVP_OP_DELETE,
    AVP_OP_LIST,
    AVP_OP_ROTATE,
    AVP_OP_HW_CHALLENGE,
    AVP_OP_HW_SIGN,
    AVP_OP_HW_ATTEST,
} avp_op_t;

/*============================================================================
 * Data Structures
 *============================================================================*/

/** Secret metadata */
typedef struct {
    char name[AVP_MAX_NAME_LEN];        /**< Secret name */
    uint8_t slot_index;                  /**< TROPIC01 slot index */
    uint32_t created_at;                 /**< Creation timestamp */
    uint32_t updated_at;                 /**< Last update timestamp */
    bool in_use;                         /**< Slot is allocated */
} avp_secret_meta_t;

/** Session state */
typedef struct {
    bool active;                                    /**< Session is active */
    char session_id[AVP_SESSION_ID_LEN + 1];       /**< Session ID (hex string) */
    char workspace[AVP_MAX_NAME_LEN];              /**< Workspace name */
    uint32_t created_at;                           /**< Session creation time */
    uint32_t ttl;                                  /**< Time-to-live in seconds */
    uint8_t pin_attempts;                          /**< Failed PIN attempts */
} avp_session_t;

/** AVP context */
typedef struct {
    avp_session_t session;                          /**< Current session */
    avp_secret_meta_t secrets[AVP_MAX_SECRETS];    /**< Secret metadata table */
    uint8_t secret_count;                          /**< Number of stored secrets */
    void *tropic_handle;                           /**< TROPIC01 device handle */
    uint32_t (*get_time)(void);                    /**< Get current timestamp */
    void (*random_bytes)(uint8_t *, size_t);       /**< Random number generator */
} avp_ctx_t;

/** Command structure (parsed from JSON) */
typedef struct {
    avp_op_t op;                            /**< Operation type */
    char session_id[AVP_SESSION_ID_LEN + 1];/**< Session ID (for authenticated ops) */
    char workspace[AVP_MAX_NAME_LEN];       /**< Workspace name (AUTHENTICATE) */
    char name[AVP_MAX_NAME_LEN];            /**< Secret name */
    char value[AVP_MAX_VALUE_LEN];          /**< Secret value (base64) */
    char auth_method[16];                   /**< "pin" */
    char pin[16];                           /**< PIN value */
    uint32_t ttl;                           /**< Session TTL */
    char key_name[AVP_MAX_NAME_LEN];        /**< Key name for HW_SIGN */
    uint8_t data[256];                      /**< Data for HW_SIGN */
    size_t data_len;                        /**< Data length */
} avp_cmd_t;

/** Response structure */
typedef struct {
    bool ok;                                /**< Success flag */
    avp_ret_t error_code;                   /**< Error code (if !ok) */
    char error_msg[128];                    /**< Error message (if !ok) */

    /* DISCOVER response */
    struct {
        char version[16];
        char backend_type[16];
        char manufacturer[32];
        char model[32];
        char serial[32];
        bool supports_hw_sign;
        bool supports_hw_attest;
        uint32_t max_secrets;
        uint32_t max_secret_size;
    } discover;

    /* AUTHENTICATE response */
    struct {
        char session_id[AVP_SESSION_ID_LEN + 1];
        uint32_t expires_in;
        char workspace[AVP_MAX_NAME_LEN];
    } auth;

    /* RETRIEVE response */
    struct {
        char value[AVP_MAX_VALUE_LEN];
    } retrieve;

    /* LIST response */
    struct {
        char names[AVP_MAX_SECRETS][AVP_MAX_NAME_LEN];
        uint8_t count;
    } list;

    /* HW_CHALLENGE response */
    struct {
        char challenge[64];
        char response_sig[128];
        bool verified;
        char model[32];
        char serial[32];
    } hw_challenge;

    /* HW_SIGN response */
    struct {
        char signature[128];
    } hw_sign;

    /* HW_ATTEST response */
    struct {
        char attestation[512];
    } hw_attest;
} avp_resp_t;

/*============================================================================
 * API Functions
 *============================================================================*/

/**
 * @brief Initialize AVP context
 *
 * @param ctx       AVP context to initialize
 * @param tropic    TROPIC01 device handle
 * @param get_time  Function to get current timestamp
 * @param rng       Function to generate random bytes
 * @return AVP_OK on success
 */
avp_ret_t avp_init(avp_ctx_t *ctx, void *tropic,
                   uint32_t (*get_time)(void),
                   void (*rng)(uint8_t *, size_t));

/**
 * @brief Process an AVP JSON command
 *
 * @param ctx       AVP context
 * @param json_in   Input JSON command string
 * @param json_out  Output buffer for JSON response
 * @param out_len   Size of output buffer
 * @return AVP_OK on success
 */
avp_ret_t avp_process(avp_ctx_t *ctx, const char *json_in,
                      char *json_out, size_t out_len);

/**
 * @brief Check if current session is valid
 *
 * @param ctx       AVP context
 * @return true if session is active and not expired
 */
bool avp_session_valid(avp_ctx_t *ctx);

/**
 * @brief Invalidate current session
 *
 * @param ctx       AVP context
 */
void avp_session_invalidate(avp_ctx_t *ctx);

/*============================================================================
 * Internal Functions (for advanced use)
 *============================================================================*/

/**
 * @brief Parse JSON command string
 */
avp_ret_t avp_parse_cmd(const char *json, avp_cmd_t *cmd);

/**
 * @brief Format JSON response string
 */
avp_ret_t avp_format_resp(const avp_resp_t *resp, char *json, size_t len);

/**
 * @brief Execute DISCOVER operation
 */
avp_ret_t avp_op_discover(avp_ctx_t *ctx, avp_resp_t *resp);

/**
 * @brief Execute AUTHENTICATE operation
 */
avp_ret_t avp_op_authenticate(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp);

/**
 * @brief Execute STORE operation
 */
avp_ret_t avp_op_store(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp);

/**
 * @brief Execute RETRIEVE operation
 */
avp_ret_t avp_op_retrieve(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp);

/**
 * @brief Execute DELETE operation
 */
avp_ret_t avp_op_delete(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp);

/**
 * @brief Execute LIST operation
 */
avp_ret_t avp_op_list(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp);

/**
 * @brief Execute ROTATE operation
 */
avp_ret_t avp_op_rotate(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp);

/**
 * @brief Execute HW_CHALLENGE operation
 */
avp_ret_t avp_op_hw_challenge(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp);

/**
 * @brief Execute HW_SIGN operation
 */
avp_ret_t avp_op_hw_sign(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp);

/**
 * @brief Execute HW_ATTEST operation
 */
avp_ret_t avp_op_hw_attest(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp);

/*============================================================================
 * Error Strings
 *============================================================================*/

/**
 * @brief Get error message for error code
 */
const char *avp_error_str(avp_ret_t err);

#ifdef __cplusplus
}
#endif

#endif /* AVP_H */
