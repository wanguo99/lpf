#include "test_framework.h"
/**
 * @file test_pdl_bmc_protocol.c
 * @brief PDL BMC协议层单元测试（IPMI和Redfish）
 */

#include "osal.h"
#include "pdl.h"

/*===========================================================================
 * IPMI协议测试
 *===========================================================================*/

/* 测试用例: IPMI帧封装 - 基本命令 */
TEST_CASE(test_ipmi_pack_request_basic)
{
    uint8_t frame[256];
    uint32_t frame_len;

    int32_t ret = bmc_ipmi_pack_request(IPMI_NETFN_CHASSIS_REQ,
                                        IPMI_CMD_CHASSIS_CONTROL,
                                        NULL, 0,
                                        frame, sizeof(frame), &frame_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_TRUE(frame_len > 0);
}

/* 测试用例: IPMI帧封装 - 带数据 */
TEST_CASE(test_ipmi_pack_request_with_data)
{
    uint8_t frame[256];
    uint32_t frame_len;
    uint8_t data[] = {IPMI_CHASSIS_POWER_UP};

    int32_t ret = bmc_ipmi_pack_request(IPMI_NETFN_CHASSIS_REQ,
                                        IPMI_CMD_CHASSIS_CONTROL,
                                        data, sizeof(data),
                                        frame, sizeof(frame), &frame_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_TRUE(frame_len > sizeof(data));
}

/* 测试用例: IPMI帧封装 - 空缓冲区 */
TEST_CASE(test_ipmi_pack_request_null_buffer)
{
    uint32_t frame_len;

    int32_t ret = bmc_ipmi_pack_request(IPMI_NETFN_CHASSIS_REQ,
                                        IPMI_CMD_CHASSIS_CONTROL,
                                        NULL, 0,
                                        NULL, 256, &frame_len);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: IPMI帧封装 - 缓冲区太小 */
TEST_CASE(test_ipmi_pack_request_buffer_too_small)
{
    uint8_t frame[10];
    uint32_t frame_len;

    int32_t ret = bmc_ipmi_pack_request(IPMI_NETFN_CHASSIS_REQ,
                                        IPMI_CMD_CHASSIS_CONTROL,
                                        NULL, 0,
                                        frame, sizeof(frame), &frame_len);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: IPMI响应解析 - 成功响应 */
TEST_CASE(test_ipmi_unpack_response_success)
{
    /* 模拟IPMI响应帧 */
    uint8_t response[] = {
        0x06, 0x00, 0xFF, 0x07,  /* RMCP头 (4字节) */
        0x00, 0x00, 0x00, 0x00, 0x00,  /* 会话头: auth_type(1) + session_seq(4) */
        0x00, 0x00, 0x00, 0x00,  /* 会话头: session_id(4) */
        0x10,  /* 消息长度 */
        0x81, 0x1D, 0x63,  /* 响应头部分1 */
        0x20, 0x00, 0x01,  /* 响应头部分2 */
        0x00,  /* 完成码 */
        0x01,  /* 数据 */
        0xE2   /* 校验和 */
    };

    uint8_t netfn, cmd, cc;
    uint8_t data[16];
    uint32_t data_len;

    int32_t ret = bmc_ipmi_unpack_response(response, sizeof(response),
                                           &netfn, &cmd, &cc,
                                           data, sizeof(data), &data_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(0x00, cc);
}

/* 测试用例: IPMI响应解析 - 空帧 */
TEST_CASE(test_ipmi_unpack_response_null_frame)
{
    uint8_t netfn, cmd, cc;
    uint8_t data[16];
    uint32_t data_len;

    int32_t ret = bmc_ipmi_unpack_response(NULL, 0,
                                           &netfn, &cmd, &cc,
                                           data, sizeof(data), &data_len);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: IPMI响应解析 - 帧太短 */
TEST_CASE(test_ipmi_unpack_response_frame_too_short)
{
    uint8_t response[] = {0x06, 0x00, 0xFF};
    uint8_t netfn, cmd, cc;
    uint8_t data[16];
    uint32_t data_len;

    int32_t ret = bmc_ipmi_unpack_response(response, sizeof(response),
                                           &netfn, &cmd, &cc,
                                           data, sizeof(data), &data_len);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * Redfish协议测试（需要模拟传输层）
 *===========================================================================*/

/* 模拟传输层发送/接收函数 */
static int32_t mock_transport_send_recv(void *handle,
                                        const uint8_t *request,
                                        uint32_t req_size,
                                        uint8_t *response,
                                        uint32_t resp_size,
                                        uint32_t *actual_size)
{
    (void)handle;
    (void)request;
    (void)req_size;

    /* 模拟HTTP响应 */
    const char *mock_response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 25\r\n"
        "\r\n"
        "{\"PowerState\":\"On\"}";

    uint32_t len = OSAL_Strlen(mock_response);
    if (len > resp_size)
    {
        len = resp_size;
    }

    OSAL_Memcpy(response, mock_response, len);
    if (NULL != actual_size)
    {
        *actual_size = len;
    }

    return OSAL_SUCCESS;
}

/* 测试用例: Redfish初始化 - 成功 */
TEST_CASE(test_redfish_init_success)
{
    void *protocol_handle = NULL;
    void *mock_transport = (void *)0x12345678;

    int32_t ret = bmc_redfish_init(mock_transport,
                                   mock_transport_send_recv,
                                   "admin",
                                   "admin",
                                   &protocol_handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(protocol_handle);

    bmc_redfish_deinit(protocol_handle);
}

/* 测试用例: Redfish初始化 - 空传输句柄 */
TEST_CASE(test_redfish_init_null_transport)
{
    void *protocol_handle = NULL;

    int32_t ret = bmc_redfish_init(NULL,
                                   mock_transport_send_recv,
                                   "admin",
                                   "admin",
                                   &protocol_handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: Redfish HTTP请求 - GET方法 */
TEST_CASE(test_redfish_request_get)
{
    void *protocol_handle = NULL;
    void *mock_transport = (void *)0x12345678;

    int32_t ret = bmc_redfish_init(mock_transport,
                                   mock_transport_send_recv,
                                   "admin",
                                   "admin",
                                   &protocol_handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    char response[1024];
    uint32_t response_len;

    ret = bmc_redfish_request(protocol_handle,
                             REDFISH_METHOD_GET,
                             "/redfish/v1/Systems/1",
                             NULL,
                             response, sizeof(response), &response_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_TRUE(response_len > 0);

    bmc_redfish_deinit(protocol_handle);
}

/* 测试用例: Redfish HTTP请求 - POST方法 */
TEST_CASE(test_redfish_request_post)
{
    void *protocol_handle = NULL;
    void *mock_transport = (void *)0x12345678;

    int32_t ret = bmc_redfish_init(mock_transport,
                                   mock_transport_send_recv,
                                   "admin",
                                   "admin",
                                   &protocol_handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    char response[1024];
    uint32_t response_len;
    const char *json_body = "{\"ResetType\":\"On\"}";

    ret = bmc_redfish_request(protocol_handle,
                             REDFISH_METHOD_POST,
                             "/redfish/v1/Systems/1/Actions/ComputerSystem.Reset",
                             json_body,
                             response, sizeof(response), &response_len);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    bmc_redfish_deinit(protocol_handle);
}

/* 测试用例: Redfish获取电源状态 - 成功 */
TEST_CASE(test_redfish_get_power_state_success)
{
    void *protocol_handle = NULL;
    void *mock_transport = (void *)0x12345678;

    int32_t ret = bmc_redfish_init(mock_transport,
                                   mock_transport_send_recv,
                                   "admin",
                                   "admin",
                                   &protocol_handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    pdl_bmc_power_state_t state;
    ret = bmc_redfish_get_power_state(protocol_handle, &state);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(PDL_BMC_POWER_ON, state);

    bmc_redfish_deinit(protocol_handle);
}

/*===========================================================================
 * 传输层测试
 *===========================================================================*/

/* 测试用例: 网络传输初始化 - 无效IP */
TEST_CASE(test_transport_net_init_invalid_ip)
{
    void *handle = NULL;

    int32_t ret = bmc_transport_net_init("invalid_ip", 623, 5000, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 网络传输初始化 - 空句柄 */
TEST_CASE(test_transport_net_init_null_handle)
{
    int32_t ret = bmc_transport_net_init("192.168.1.100", 623, 5000, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 串口传输初始化 - 空设备 */
TEST_CASE(test_transport_serial_init_null_device)
{
    void *handle = NULL;

    int32_t ret = bmc_transport_serial_init(NULL, 115200, 5000, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

TEST_MODULE_BEGIN(test_pdl_bmc_protocol, "PDL")
    /* IPMI协议测试 */
    TEST_CASE_REF(test_ipmi_pack_request_basic)
    TEST_CASE_REF(test_ipmi_pack_request_with_data)
    TEST_CASE_REF(test_ipmi_pack_request_null_buffer)
    TEST_CASE_REF(test_ipmi_pack_request_buffer_too_small)
    TEST_CASE_REF(test_ipmi_unpack_response_success)
    TEST_CASE_REF(test_ipmi_unpack_response_null_frame)
    TEST_CASE_REF(test_ipmi_unpack_response_frame_too_short)

    /* Redfish协议测试 */
    TEST_CASE_REF(test_redfish_init_success)
    TEST_CASE_REF(test_redfish_init_null_transport)
    TEST_CASE_REF(test_redfish_request_get)
    TEST_CASE_REF(test_redfish_request_post)
    TEST_CASE_REF(test_redfish_get_power_state_success)

    /* 传输层测试 */
    TEST_CASE_REF(test_transport_net_init_invalid_ip)
    TEST_CASE_REF(test_transport_net_init_null_handle)
    TEST_CASE_REF(test_transport_serial_init_null_device)
TEST_MODULE_END(test_pdl_bmc_protocol, "PDL")
