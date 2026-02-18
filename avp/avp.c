/**
 * @file avp.c
 * @brief Agent Vault Protocol (AVP) implementation for NexusClaw
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025 AVP Protocol Contributors
 */

#include "avp.h"
#include <string.h>
#include <stdio.h>

/* For TROPIC01 operations - include libtropic */
/* #include "libtropic.h" */

/*============================================================================
 * Constants
 *============================================================================*/

#define AVP_VERSION         "0.1.0"
#define AVP_BACKEND_TYPE    "hardware"
#define AVP_MANUFACTURER    "AVP Protocol"
#define AVP_MODEL           "NexusClaw"
#define AVP_MAX_PIN_ATTEMPTS 5

/* TROPIC01 slot allocation */
#define SLOT_SECRETS_START  96
#define SLOT_SECRETS_END    127

/*============================================================================
 * Helper Functions
 *============================================================================*/

static void hex_encode(const uint8_t *data, size_t len, char *out)
{
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        out[i * 2] = hex[data[i] >> 4];
        out[i * 2 + 1] = hex[data[i] & 0x0F];
    }
    out[len * 2] = '\0';
}

static int hex_decode(const char *hex, uint8_t *out, size_t max_len)
{
    size_t len = strlen(hex);
    if (len % 2 != 0 || len / 2 > max_len) {
        return -1;
    }

    for (size_t i = 0; i < len / 2; i++) {
        unsigned int byte;
        if (sscanf(&hex[i * 2], "%2x", &byte) != 1) {
            return -1;
        }
        out[i] = (uint8_t)byte;
    }
    return len / 2;
}

static void generate_session_id(avp_ctx_t *ctx, char *out)
{
    uint8_t random[16];
    ctx->random_bytes(random, sizeof(random));
    hex_encode(random, sizeof(random), out);
}

static int find_secret_by_name(avp_ctx_t *ctx, const char *name)
{
    for (int i = 0; i < AVP_MAX_SECRETS; i++) {
        if (ctx->secrets[i].in_use &&
            strcmp(ctx->secrets[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_free_slot(avp_ctx_t *ctx)
{
    for (int i = 0; i < AVP_MAX_SECRETS; i++) {
        if (!ctx->secrets[i].in_use) {
            return i;
        }
    }
    return -1;
}

/*============================================================================
 * Error Messages
 *============================================================================*/

const char *avp_error_str(avp_ret_t err)
{
    switch (err) {
        case AVP_OK:                    return "OK";
        case AVP_ERR_PARSE:             return "PARSE_ERROR";
        case AVP_ERR_INVALID_OP:        return "INVALID_OPERATION";
        case AVP_ERR_INVALID_PARAM:     return "INVALID_PARAMETER";
        case AVP_ERR_NOT_AUTHENTICATED: return "NOT_AUTHENTICATED";
        case AVP_ERR_SESSION_EXPIRED:   return "SESSION_EXPIRED";
        case AVP_ERR_SECRET_NOT_FOUND:  return "SECRET_NOT_FOUND";
        case AVP_ERR_CAPACITY:          return "CAPACITY_EXCEEDED";
        case AVP_ERR_HARDWARE:          return "HARDWARE_ERROR";
        case AVP_ERR_CRYPTO:            return "CRYPTO_ERROR";
        case AVP_ERR_PIN_INVALID:       return "PIN_INVALID";
        case AVP_ERR_PIN_LOCKED:        return "PIN_LOCKED";
        case AVP_ERR_INTERNAL:          return "INTERNAL_ERROR";
        default:                        return "UNKNOWN_ERROR";
    }
}

/*============================================================================
 * JSON Parsing (minimal implementation)
 *============================================================================*/

static const char *json_find_string(const char *json, const char *key, char *out, size_t max_len)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);

    const char *pos = strstr(json, search);
    if (!pos) return NULL;

    /* Find the colon */
    pos = strchr(pos, ':');
    if (!pos) return NULL;
    pos++;

    /* Skip whitespace */
    while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r') pos++;

    /* Check for string start */
    if (*pos != '"') return NULL;
    pos++;

    /* Copy until end quote */
    size_t i = 0;
    while (*pos && *pos != '"' && i < max_len - 1) {
        out[i++] = *pos++;
    }
    out[i] = '\0';

    return out;
}

static int json_find_int(const char *json, const char *key, uint32_t *out)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);

    const char *pos = strstr(json, search);
    if (!pos) return -1;

    pos = strchr(pos, ':');
    if (!pos) return -1;
    pos++;

    while (*pos == ' ' || *pos == '\t') pos++;

    if (sscanf(pos, "%u", out) != 1) return -1;

    return 0;
}

avp_ret_t avp_parse_cmd(const char *json, avp_cmd_t *cmd)
{
    memset(cmd, 0, sizeof(*cmd));
    cmd->ttl = AVP_DEFAULT_TTL;

    /* Parse operation */
    char op_str[32];
    if (!json_find_string(json, "op", op_str, sizeof(op_str))) {
        return AVP_ERR_PARSE;
    }

    if (strcmp(op_str, "DISCOVER") == 0) {
        cmd->op = AVP_OP_DISCOVER;
    } else if (strcmp(op_str, "AUTHENTICATE") == 0) {
        cmd->op = AVP_OP_AUTHENTICATE;
    } else if (strcmp(op_str, "STORE") == 0) {
        cmd->op = AVP_OP_STORE;
    } else if (strcmp(op_str, "RETRIEVE") == 0) {
        cmd->op = AVP_OP_RETRIEVE;
    } else if (strcmp(op_str, "DELETE") == 0) {
        cmd->op = AVP_OP_DELETE;
    } else if (strcmp(op_str, "LIST") == 0) {
        cmd->op = AVP_OP_LIST;
    } else if (strcmp(op_str, "ROTATE") == 0) {
        cmd->op = AVP_OP_ROTATE;
    } else if (strcmp(op_str, "HW_CHALLENGE") == 0) {
        cmd->op = AVP_OP_HW_CHALLENGE;
    } else if (strcmp(op_str, "HW_SIGN") == 0) {
        cmd->op = AVP_OP_HW_SIGN;
    } else if (strcmp(op_str, "HW_ATTEST") == 0) {
        cmd->op = AVP_OP_HW_ATTEST;
    } else {
        return AVP_ERR_INVALID_OP;
    }

    /* Parse optional fields */
    json_find_string(json, "session_id", cmd->session_id, sizeof(cmd->session_id));
    json_find_string(json, "workspace", cmd->workspace, sizeof(cmd->workspace));
    json_find_string(json, "name", cmd->name, sizeof(cmd->name));
    json_find_string(json, "value", cmd->value, sizeof(cmd->value));
    json_find_string(json, "auth_method", cmd->auth_method, sizeof(cmd->auth_method));
    json_find_string(json, "pin", cmd->pin, sizeof(cmd->pin));
    json_find_string(json, "key_name", cmd->key_name, sizeof(cmd->key_name));
    json_find_int(json, "ttl", &cmd->ttl);
    json_find_int(json, "requested_ttl", &cmd->ttl);

    /* Parse data field (hex encoded for HW_SIGN) */
    char data_hex[512];
    if (json_find_string(json, "data", data_hex, sizeof(data_hex))) {
        int len = hex_decode(data_hex, cmd->data, sizeof(cmd->data));
        if (len > 0) cmd->data_len = len;
    }

    return AVP_OK;
}

/*============================================================================
 * JSON Response Formatting
 *============================================================================*/

avp_ret_t avp_format_resp(const avp_resp_t *resp, char *json, size_t len)
{
    int n;

    if (!resp->ok) {
        n = snprintf(json, len,
            "{\"ok\":false,\"error\":\"%s\",\"message\":\"%s\"}",
            avp_error_str(resp->error_code),
            resp->error_msg[0] ? resp->error_msg : avp_error_str(resp->error_code));
    } else {
        /* Format based on which fields are populated */
        if (resp->discover.version[0]) {
            n = snprintf(json, len,
                "{\"ok\":true,"
                "\"version\":\"%s\","
                "\"backend_type\":\"%s\","
                "\"manufacturer\":\"%s\","
                "\"model\":\"%s\","
                "\"serial\":\"%s\","
                "\"capabilities\":{"
                "\"hw_sign\":%s,"
                "\"hw_attest\":%s,"
                "\"max_secrets\":%u,"
                "\"max_secret_size\":%u"
                "}}",
                resp->discover.version,
                resp->discover.backend_type,
                resp->discover.manufacturer,
                resp->discover.model,
                resp->discover.serial,
                resp->discover.supports_hw_sign ? "true" : "false",
                resp->discover.supports_hw_attest ? "true" : "false",
                resp->discover.max_secrets,
                resp->discover.max_secret_size);
        } else if (resp->auth.session_id[0]) {
            n = snprintf(json, len,
                "{\"ok\":true,"
                "\"session_id\":\"%s\","
                "\"expires_in\":%u,"
                "\"workspace\":\"%s\"}",
                resp->auth.session_id,
                resp->auth.expires_in,
                resp->auth.workspace);
        } else if (resp->retrieve.value[0]) {
            n = snprintf(json, len,
                "{\"ok\":true,\"value\":\"%s\"}",
                resp->retrieve.value);
        } else if (resp->list.count > 0) {
            n = snprintf(json, len, "{\"ok\":true,\"secrets\":[");
            for (int i = 0; i < resp->list.count && n < (int)len - 10; i++) {
                if (i > 0) n += snprintf(json + n, len - n, ",");
                n += snprintf(json + n, len - n, "\"%s\"", resp->list.names[i]);
            }
            n += snprintf(json + n, len - n, "]}");
        } else if (resp->hw_challenge.challenge[0]) {
            n = snprintf(json, len,
                "{\"ok\":true,"
                "\"verified\":%s,"
                "\"model\":\"%s\","
                "\"serial\":\"%s\"}",
                resp->hw_challenge.verified ? "true" : "false",
                resp->hw_challenge.model,
                resp->hw_challenge.serial);
        } else if (resp->hw_sign.signature[0]) {
            n = snprintf(json, len,
                "{\"ok\":true,\"signature\":\"%s\"}",
                resp->hw_sign.signature);
        } else {
            n = snprintf(json, len, "{\"ok\":true}");
        }
    }

    if (n >= (int)len) {
        return AVP_ERR_INTERNAL;
    }
    return AVP_OK;
}

/*============================================================================
 * Operation Implementations
 *============================================================================*/

avp_ret_t avp_op_discover(avp_ctx_t *ctx, avp_resp_t *resp)
{
    (void)ctx;

    resp->ok = true;
    strncpy(resp->discover.version, AVP_VERSION, sizeof(resp->discover.version) - 1);
    strncpy(resp->discover.backend_type, AVP_BACKEND_TYPE, sizeof(resp->discover.backend_type) - 1);
    strncpy(resp->discover.manufacturer, AVP_MANUFACTURER, sizeof(resp->discover.manufacturer) - 1);
    strncpy(resp->discover.model, AVP_MODEL, sizeof(resp->discover.model) - 1);

    /* TODO: Get actual serial from TROPIC01 */
    strncpy(resp->discover.serial, "NC00000001", sizeof(resp->discover.serial) - 1);

    resp->discover.supports_hw_sign = true;
    resp->discover.supports_hw_attest = true;
    resp->discover.max_secrets = AVP_MAX_SECRETS;
    resp->discover.max_secret_size = 256;

    return AVP_OK;
}

avp_ret_t avp_op_authenticate(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp)
{
    /* Check PIN lockout */
    if (ctx->session.pin_attempts >= AVP_MAX_PIN_ATTEMPTS) {
        resp->ok = false;
        resp->error_code = AVP_ERR_PIN_LOCKED;
        return AVP_ERR_PIN_LOCKED;
    }

    /* Validate PIN */
    /* TODO: Verify PIN with TROPIC01 */
    if (strlen(cmd->pin) < 4) {
        ctx->session.pin_attempts++;
        resp->ok = false;
        resp->error_code = AVP_ERR_PIN_INVALID;
        return AVP_ERR_PIN_INVALID;
    }

    /* Reset PIN attempts on success */
    ctx->session.pin_attempts = 0;

    /* Create new session */
    ctx->session.active = true;
    generate_session_id(ctx, ctx->session.session_id);
    strncpy(ctx->session.workspace, cmd->workspace[0] ? cmd->workspace : "default",
            sizeof(ctx->session.workspace) - 1);
    ctx->session.created_at = ctx->get_time();
    ctx->session.ttl = cmd->ttl > 0 ? cmd->ttl : AVP_DEFAULT_TTL;

    /* Build response */
    resp->ok = true;
    strncpy(resp->auth.session_id, ctx->session.session_id, sizeof(resp->auth.session_id) - 1);
    resp->auth.expires_in = ctx->session.ttl;
    strncpy(resp->auth.workspace, ctx->session.workspace, sizeof(resp->auth.workspace) - 1);

    return AVP_OK;
}

avp_ret_t avp_op_store(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp)
{
    /* Validate session */
    if (!avp_session_valid(ctx)) {
        resp->ok = false;
        resp->error_code = AVP_ERR_NOT_AUTHENTICATED;
        return AVP_ERR_NOT_AUTHENTICATED;
    }

    /* Check if secret already exists */
    int idx = find_secret_by_name(ctx, cmd->name);
    if (idx < 0) {
        /* Find free slot */
        idx = find_free_slot(ctx);
        if (idx < 0) {
            resp->ok = false;
            resp->error_code = AVP_ERR_CAPACITY;
            return AVP_ERR_CAPACITY;
        }

        /* Initialize new slot */
        strncpy(ctx->secrets[idx].name, cmd->name, AVP_MAX_NAME_LEN - 1);
        ctx->secrets[idx].slot_index = SLOT_SECRETS_START + idx;
        ctx->secrets[idx].created_at = ctx->get_time();
        ctx->secrets[idx].in_use = true;
        ctx->secret_count++;
    }

    /* Update timestamp */
    ctx->secrets[idx].updated_at = ctx->get_time();

    /* TODO: Store value in TROPIC01 slot */
    /* lt_r_mem_data_write(ctx->tropic_handle, ctx->secrets[idx].slot_index,
     *                     (uint8_t *)cmd->value, strlen(cmd->value)); */

    resp->ok = true;
    return AVP_OK;
}

avp_ret_t avp_op_retrieve(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp)
{
    /* Validate session */
    if (!avp_session_valid(ctx)) {
        resp->ok = false;
        resp->error_code = AVP_ERR_NOT_AUTHENTICATED;
        return AVP_ERR_NOT_AUTHENTICATED;
    }

    /* Find secret */
    int idx = find_secret_by_name(ctx, cmd->name);
    if (idx < 0) {
        resp->ok = false;
        resp->error_code = AVP_ERR_SECRET_NOT_FOUND;
        return AVP_ERR_SECRET_NOT_FOUND;
    }

    /* TODO: Read value from TROPIC01 slot */
    /* lt_r_mem_data_read(ctx->tropic_handle, ctx->secrets[idx].slot_index,
     *                    (uint8_t *)resp->retrieve.value, &len); */

    /* Placeholder - in real implementation, read from TROPIC01 */
    strncpy(resp->retrieve.value, "[stored_value]", sizeof(resp->retrieve.value) - 1);

    resp->ok = true;
    return AVP_OK;
}

avp_ret_t avp_op_delete(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp)
{
    /* Validate session */
    if (!avp_session_valid(ctx)) {
        resp->ok = false;
        resp->error_code = AVP_ERR_NOT_AUTHENTICATED;
        return AVP_ERR_NOT_AUTHENTICATED;
    }

    /* Find secret */
    int idx = find_secret_by_name(ctx, cmd->name);
    if (idx < 0) {
        resp->ok = false;
        resp->error_code = AVP_ERR_SECRET_NOT_FOUND;
        return AVP_ERR_SECRET_NOT_FOUND;
    }

    /* TODO: Erase TROPIC01 slot */
    /* lt_r_mem_data_erase(ctx->tropic_handle, ctx->secrets[idx].slot_index); */

    /* Clear metadata */
    memset(&ctx->secrets[idx], 0, sizeof(avp_secret_meta_t));
    ctx->secret_count--;

    resp->ok = true;
    return AVP_OK;
}

avp_ret_t avp_op_list(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp)
{
    (void)cmd;

    /* Validate session */
    if (!avp_session_valid(ctx)) {
        resp->ok = false;
        resp->error_code = AVP_ERR_NOT_AUTHENTICATED;
        return AVP_ERR_NOT_AUTHENTICATED;
    }

    /* Build list of secret names */
    resp->list.count = 0;
    for (int i = 0; i < AVP_MAX_SECRETS && resp->list.count < AVP_MAX_SECRETS; i++) {
        if (ctx->secrets[i].in_use) {
            strncpy(resp->list.names[resp->list.count], ctx->secrets[i].name,
                    AVP_MAX_NAME_LEN - 1);
            resp->list.count++;
        }
    }

    resp->ok = true;
    return AVP_OK;
}

avp_ret_t avp_op_rotate(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp)
{
    /* Rotate is essentially a store with the same name */
    return avp_op_store(ctx, cmd, resp);
}

avp_ret_t avp_op_hw_challenge(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp)
{
    (void)cmd;

    /* TODO: Perform actual attestation with TROPIC01 */
    /* lt_get_info_req(ctx->tropic_handle, &info); */
    /* lt_mcounter_get(ctx->tropic_handle, &counter); */

    resp->ok = true;
    resp->hw_challenge.verified = true;
    strncpy(resp->hw_challenge.model, "TROPIC01", sizeof(resp->hw_challenge.model) - 1);

    /* TODO: Get actual serial from TROPIC01 */
    strncpy(resp->hw_challenge.serial, "NC00000001", sizeof(resp->hw_challenge.serial) - 1);

    return AVP_OK;
}

avp_ret_t avp_op_hw_sign(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp)
{
    /* Validate session */
    if (!avp_session_valid(ctx)) {
        resp->ok = false;
        resp->error_code = AVP_ERR_NOT_AUTHENTICATED;
        return AVP_ERR_NOT_AUTHENTICATED;
    }

    /* TODO: Sign with TROPIC01 */
    /* lt_ecc_ecdsa_sign(ctx->tropic_handle, key_slot, cmd->data, cmd->data_len,
     *                   signature, &sig_len); */

    /* Placeholder signature */
    uint8_t fake_sig[64];
    ctx->random_bytes(fake_sig, sizeof(fake_sig));
    hex_encode(fake_sig, sizeof(fake_sig), resp->hw_sign.signature);

    resp->ok = true;
    return AVP_OK;
}

avp_ret_t avp_op_hw_attest(avp_ctx_t *ctx, const avp_cmd_t *cmd, avp_resp_t *resp)
{
    (void)cmd;

    /* Validate session */
    if (!avp_session_valid(ctx)) {
        resp->ok = false;
        resp->error_code = AVP_ERR_NOT_AUTHENTICATED;
        return AVP_ERR_NOT_AUTHENTICATED;
    }

    /* TODO: Generate attestation with TROPIC01 */

    resp->ok = true;
    strncpy(resp->hw_attest.attestation, "{\"model\":\"TROPIC01\",\"firmware\":\"1.0.0\"}",
            sizeof(resp->hw_attest.attestation) - 1);

    return AVP_OK;
}

/*============================================================================
 * Main API
 *============================================================================*/

avp_ret_t avp_init(avp_ctx_t *ctx, void *tropic,
                   uint32_t (*get_time)(void),
                   void (*rng)(uint8_t *, size_t))
{
    if (!ctx || !get_time || !rng) {
        return AVP_ERR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->tropic_handle = tropic;
    ctx->get_time = get_time;
    ctx->random_bytes = rng;

    return AVP_OK;
}

bool avp_session_valid(avp_ctx_t *ctx)
{
    if (!ctx->session.active) {
        return false;
    }

    uint32_t now = ctx->get_time();
    uint32_t expires_at = ctx->session.created_at + ctx->session.ttl;

    if (now >= expires_at) {
        ctx->session.active = false;
        return false;
    }

    return true;
}

void avp_session_invalidate(avp_ctx_t *ctx)
{
    ctx->session.active = false;
    memset(ctx->session.session_id, 0, sizeof(ctx->session.session_id));
}

avp_ret_t avp_process(avp_ctx_t *ctx, const char *json_in,
                      char *json_out, size_t out_len)
{
    avp_cmd_t cmd;
    avp_resp_t resp;
    avp_ret_t ret;

    memset(&resp, 0, sizeof(resp));

    /* Parse input JSON */
    ret = avp_parse_cmd(json_in, &cmd);
    if (ret != AVP_OK) {
        resp.ok = false;
        resp.error_code = ret;
        goto format_response;
    }

    /* Execute operation */
    switch (cmd.op) {
        case AVP_OP_DISCOVER:
            ret = avp_op_discover(ctx, &resp);
            break;
        case AVP_OP_AUTHENTICATE:
            ret = avp_op_authenticate(ctx, &cmd, &resp);
            break;
        case AVP_OP_STORE:
            ret = avp_op_store(ctx, &cmd, &resp);
            break;
        case AVP_OP_RETRIEVE:
            ret = avp_op_retrieve(ctx, &cmd, &resp);
            break;
        case AVP_OP_DELETE:
            ret = avp_op_delete(ctx, &cmd, &resp);
            break;
        case AVP_OP_LIST:
            ret = avp_op_list(ctx, &cmd, &resp);
            break;
        case AVP_OP_ROTATE:
            ret = avp_op_rotate(ctx, &cmd, &resp);
            break;
        case AVP_OP_HW_CHALLENGE:
            ret = avp_op_hw_challenge(ctx, &cmd, &resp);
            break;
        case AVP_OP_HW_SIGN:
            ret = avp_op_hw_sign(ctx, &cmd, &resp);
            break;
        case AVP_OP_HW_ATTEST:
            ret = avp_op_hw_attest(ctx, &cmd, &resp);
            break;
        default:
            resp.ok = false;
            resp.error_code = AVP_ERR_INVALID_OP;
            ret = AVP_ERR_INVALID_OP;
            break;
    }

format_response:
    /* Format output JSON */
    return avp_format_resp(&resp, json_out, out_len);
}
