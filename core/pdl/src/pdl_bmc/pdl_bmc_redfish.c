/************************************************************************
 * Redfish协议实现
 *
 * 职责：
 * - 实现Redfish RESTful API（HTTP/HTTPS客户端）
 * - JSON数据解析和构造
 * - 实现标准Redfish资源访问（Systems, Chassis, Sensors）
 * - 认证管理（Basic Auth / Session Token）
 ************************************************************************/

#include "pdl_bmc_internal.h"
#include "osal/osal.h"

/*
 * Redfish协议上下文
 */
typedef struct
{
    void *transport_handle;
    int32_t (*send_recv)(void*, const uint8_t*, uint32_t, uint8_t*, uint32_t, uint32_t*);
    redfish_auth_t auth;
    char base_url[128];
    osal_mutex_t *mutex;
} bmc_redfish_context_t;

/*
 * Redfish标准URI路径
 */
#define REDFISH_URI_ROOT            "/redfish/v1"
#define REDFISH_URI_SYSTEMS         "/redfish/v1/Systems"
#define REDFISH_URI_CHASSIS         "/redfish/v1/Chassis"
#define REDFISH_URI_SENSORS         "/redfish/v1/Chassis/1/Sensors"
#define REDFISH_URI_SESSION_SERVICE "/redfish/v1/SessionService/Sessions"

/**
 * @brief 简单的JSON值提取（查找键值对）
 */
static int32_t json_get_string(const char *json, const char *key, char *value, uint32_t value_size)
{
    char search_key[128];
    const char *key_pos;
    const char *colon;
    const char *value_start;
    const char *value_end;
    uint32_t len;

    if (NULL == json || NULL == key || NULL == value)
    {
        return OSAL_ERR_GENERIC;
    }

    OSAL_Snprintf(search_key, sizeof(search_key), "\"%s\"", key);

    key_pos = OSAL_Strstr(json, search_key);
    if (NULL == key_pos)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 查找冒号 */
    colon = key_pos;
    while (*colon != '\0' && *colon != ':')
    {
        colon++;
    }
    if (*colon != ':')
    {
        return OSAL_ERR_GENERIC;
    }

    value_start = colon + 1;
    while (*value_start == ' ' || *value_start == '\t' || *value_start == '\n')
    {
        value_start++;
    }

    if (*value_start == '"')
    {
        value_start++;
        /* 查找结束引号 */
        value_end = value_start;
        while (*value_end != '\0' && *value_end != '"')
        {
            value_end++;
        }
        if (*value_end != '"')
        {
            return OSAL_ERR_GENERIC;
        }

        len = value_end - value_start;
        if (len >= value_size)
        {
            len = value_size - 1;
        }

        OSAL_Memcpy(value, value_start, len);
        value[len] = '\0';
        return OSAL_SUCCESS;
    }

    return OSAL_ERR_GENERIC;
}

/**
 * @brief 构造HTTP请求
 */
static int32_t build_http_request(bmc_redfish_context_t *ctx,
                                  redfish_method_t method,
                                  const char *uri,
                                  const char *json_body,
                                  char *request,
                                  uint32_t request_size,
                                  uint32_t *actual_size)
{
    const char *method_str;
    uint32_t body_len;
    int32_t len;
    char auth_str[256];

    if (NULL == ctx || NULL == uri || NULL == request || NULL == actual_size)
    {
        return OSAL_ERR_GENERIC;
    }

    method_str = "GET";
    switch (method)
    {
        case REDFISH_METHOD_GET:    method_str = "GET"; break;
        case REDFISH_METHOD_POST:   method_str = "POST"; break;
        case REDFISH_METHOD_PATCH:  method_str = "PATCH"; break;
        case REDFISH_METHOD_DELETE: method_str = "DELETE"; break;
        default: return OSAL_ERR_GENERIC;
    }

    body_len = (NULL != json_body) ? OSAL_Strlen(json_body) : 0;

    len = OSAL_Snprintf(request, request_size,
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Accept: application/json\r\n",
        method_str, uri, ctx->base_url);

    if (ctx->auth.use_session && ctx->auth.session_token[0] != '\0')
    {
        len += OSAL_Snprintf(request + len, request_size - len,
            "X-Auth-Token: %s\r\n", ctx->auth.session_token);
    }
    else if (ctx->auth.username != NULL && ctx->auth.password != NULL)
    {
        OSAL_Snprintf(auth_str, sizeof(auth_str), "%s:%s", ctx->auth.username, ctx->auth.password);
        len += OSAL_Snprintf(request + len, request_size - len,
            "Authorization: Basic %s\r\n", auth_str);
    }

    if (body_len > 0)
    {
        len += OSAL_Snprintf(request + len, request_size - len,
            "Content-Length: %u\r\n\r\n%s", body_len, json_body);
    }
    else
    {
        len += OSAL_Snprintf(request + len, request_size - len, "\r\n");
    }

    *actual_size = len;
    return OSAL_SUCCESS;
}

/**
 * @brief 解析HTTP响应
 */
static int32_t parse_http_response(const char *response,
                                   uint32_t response_len,
                                   int32_t *status_code,
                                   char *body,
                                   uint32_t body_size,
                                   uint32_t *body_len)
{
    const char *body_start;
    uint32_t len;

    if (NULL == response || response_len < 12)
    {
        return OSAL_ERR_GENERIC;
    }

    if (OSAL_Strncmp(response, "HTTP/1.1 ", 9) != 0)
    {
        return OSAL_ERR_GENERIC;
    }

    if (NULL != status_code)
    {
        *status_code = OSAL_Atoi(response + 9);
    }

    body_start = OSAL_Strstr(response, "\r\n\r\n");
    if (NULL != body_start)
    {
        body_start += 4;
        len = response_len - (body_start - response);

        if (NULL != body && body_size > 0)
        {
            if (len >= body_size)
            {
                len = body_size - 1;
            }
            OSAL_Memcpy(body, body_start, len);
            body[len] = '\0';
        }

        if (NULL != body_len)
        {
            *body_len = len;
        }
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 初始化Redfish协议
 */
int32_t bmc_redfish_init(void *transport_handle,
                         int32_t (*send_recv)(void*, const uint8_t*, uint32_t, uint8_t*, uint32_t, uint32_t*),
                         const char *username,
                         const char *password,
                         void **protocol_handle)
{
    bmc_redfish_context_t *ctx;

    if (NULL == transport_handle || NULL == send_recv || NULL == protocol_handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_redfish_context_t *)OSAL_Malloc(sizeof(bmc_redfish_context_t));
    if (NULL == ctx)
    {
        return OSAL_ERR_GENERIC;
    }

    OSAL_Memset(ctx, 0, sizeof(bmc_redfish_context_t));
    ctx->transport_handle = transport_handle;
    ctx->send_recv = send_recv;
    ctx->auth.username = username;
    ctx->auth.password = password;
    ctx->auth.use_session = false;
    OSAL_Strcpy(ctx->base_url, "bmc");

    if (OSAL_SUCCESS != OSAL_MutexCreate(&ctx->mutex))
    {
        OSAL_Free(ctx);
        return OSAL_ERR_GENERIC;
    }

    *protocol_handle = ctx;
    return OSAL_SUCCESS;
}

/**
 * @brief 反初始化Redfish协议
 */
int32_t bmc_redfish_deinit(void *protocol_handle)
{
    bmc_redfish_context_t *ctx;

    if (NULL == protocol_handle)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_redfish_context_t *)protocol_handle;

    OSAL_MutexDelete(ctx->mutex);
    OSAL_Free(ctx);

    return OSAL_SUCCESS;
}

/**
 * @brief 发送Redfish HTTP请求
 */
int32_t bmc_redfish_request(void *protocol_handle,
                            redfish_method_t method,
                            const char *uri,
                            const char *json_body,
                            char *response,
                            uint32_t resp_size,
                            uint32_t *actual_size)
{
    bmc_redfish_context_t *ctx;
    char request[2048];
    uint32_t request_len;
    int32_t ret;
    uint8_t http_response[4096];
    uint32_t http_response_len;
    int32_t status_code;

    if (NULL == protocol_handle || NULL == uri)
    {
        return OSAL_ERR_GENERIC;
    }

    ctx = (bmc_redfish_context_t *)protocol_handle;

    OSAL_MutexLock(ctx->mutex);

    ret = build_http_request(ctx, method, uri, json_body,
                                     request, sizeof(request), &request_len);
    if (OSAL_SUCCESS != ret)
    {
        OSAL_MutexUnlock(ctx->mutex);
        return ret;
    }

    ret = ctx->send_recv(ctx->transport_handle,
                        (const uint8_t *)request, request_len,
                        http_response, sizeof(http_response), &http_response_len);
    if (OSAL_SUCCESS != ret)
    {
        OSAL_MutexUnlock(ctx->mutex);
        return ret;
    }

    ret = parse_http_response((const char *)http_response, http_response_len,
                             &status_code, response, resp_size, actual_size);

    if (OSAL_SUCCESS == ret && (status_code < 200 || status_code >= 300))
    {
        LOG_ERROR("Redfish", "HTTP request failed with status: %d", status_code);
        ret = OSAL_ERR_GENERIC;
    }

    OSAL_MutexUnlock(ctx->mutex);

    return ret;
}

/**
 * @brief Redfish电源开机
 */
int32_t bmc_redfish_power_on(void *protocol_handle)
{
    const char *json_body;
    char response[1024];
    uint32_t response_len;

    if (NULL == protocol_handle)
    {
        return OSAL_ERR_GENERIC;
    }

    json_body = "{\"ResetType\":\"On\"}";

    return bmc_redfish_request(protocol_handle, REDFISH_METHOD_POST,
                              "/redfish/v1/Systems/1/Actions/ComputerSystem.Reset",
                              json_body, response, sizeof(response), &response_len);
}

/**
 * @brief Redfish电源关机
 */
int32_t bmc_redfish_power_off(void *protocol_handle)
{
    const char *json_body;
    char response[1024];
    uint32_t response_len;

    if (NULL == protocol_handle)
    {
        return OSAL_ERR_GENERIC;
    }

    json_body = "{\"ResetType\":\"ForceOff\"}";

    return bmc_redfish_request(protocol_handle, REDFISH_METHOD_POST,
                              "/redfish/v1/Systems/1/Actions/ComputerSystem.Reset",
                              json_body, response, sizeof(response), &response_len);
}

/**
 * @brief Redfish电源复位
 */
int32_t bmc_redfish_power_reset(void *protocol_handle)
{
    const char *json_body;
    char response[1024];
    uint32_t response_len;

    if (NULL == protocol_handle)
    {
        return OSAL_ERR_GENERIC;
    }

    json_body = "{\"ResetType\":\"ForceRestart\"}";

    return bmc_redfish_request(protocol_handle, REDFISH_METHOD_POST,
                              "/redfish/v1/Systems/1/Actions/ComputerSystem.Reset",
                              json_body, response, sizeof(response), &response_len);
}

/**
 * @brief 获取电源状态
 */
int32_t bmc_redfish_get_power_state(void *protocol_handle, bmc_power_state_t *state)
{
    char response[2048];
    uint32_t response_len;
    int32_t ret;
    char power_state[32];

    if (NULL == protocol_handle || NULL == state)
    {
        return OSAL_ERR_GENERIC;
    }

    ret = bmc_redfish_request(protocol_handle, REDFISH_METHOD_GET,
                                     "/redfish/v1/Systems/1",
                                     NULL, response, sizeof(response), &response_len);
    if (OSAL_SUCCESS != ret)
    {
        *state = BMC_POWER_UNKNOWN;
        return ret;
    }

    if (OSAL_SUCCESS == json_get_string(response, "PowerState", power_state, sizeof(power_state)))
    {
        if (OSAL_Strcmp(power_state, "On") == 0)
        {
            *state = BMC_POWER_ON;
        }
        else if (OSAL_Strcmp(power_state, "Off") == 0)
        {
            *state = BMC_POWER_OFF;
        }
        else
        {
            *state = BMC_POWER_UNKNOWN;
        }
    }
    else
    {
        *state = BMC_POWER_UNKNOWN;
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 读取传感器
 */
int32_t bmc_redfish_read_sensors(void *protocol_handle,
                                 bmc_sensor_type_t type,
                                 bmc_sensor_reading_t *readings,
                                 uint32_t max_count,
                                 uint32_t *actual_count)
{
    if (NULL == protocol_handle)
    {
        return OSAL_ERR_GENERIC;
    }

    (void)type;
    (void)readings;
    (void)max_count;

    if (NULL != actual_count)
    {
        *actual_count = 0;
    }

    return OSAL_ERR_GENERIC;
}
