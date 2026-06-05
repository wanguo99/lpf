/************************************************************************
 * BMC服务内部接口
 *
 * 仅供pdl_bmc模块内部使用，不对外暴露
 ************************************************************************/

#ifndef PDL_BMC_INTERNAL_H
#define PDL_BMC_INTERNAL_H

#include "osal.h"
#include "pdl.h"

/************************************************************************
 * 传输层接口（pdl_bmc_transport.c实现）
 * 职责：提供底层网络和串口通信能力
 ************************************************************************/

/*
 * 网络传输接口
 */
int32_t bmc_transport_net_init(const char *ip_addr, uint16_t port, uint32_t timeout_ms, void **handle);
int32_t bmc_transport_net_deinit(void *handle);
int32_t bmc_transport_net_send_recv(void *handle,
                                    const uint8_t *request,
                                    uint32_t req_size,
                                    uint8_t *response,
                                    uint32_t resp_size,
                                    uint32_t *actual_size);

/*
 * 串口传输接口
 */
int32_t bmc_transport_serial_init(const char *device, uint32_t baudrate, uint32_t timeout_ms, void **handle);
int32_t bmc_transport_serial_deinit(void *handle);
int32_t bmc_transport_serial_send_recv(void *handle,
                                       const uint8_t *request,
                                       uint32_t req_size,
                                       uint8_t *response,
                                       uint32_t resp_size,
                                       uint32_t *actual_size);

/************************************************************************
 * IPMI协议层接口（pdl_bmc_ipmi.c实现）
 * 职责：实现IPMI协议栈（RMCP/RMCP+, NetFn/Cmd, SDR解析等）
 ************************************************************************/

/*
 * IPMI NetFn定义
 */
#define IPMI_NETFN_CHASSIS_REQ      0x00
#define IPMI_NETFN_CHASSIS_RESP     0x01
#define IPMI_NETFN_SENSOR_REQ       0x04
#define IPMI_NETFN_SENSOR_RESP      0x05
#define IPMI_NETFN_APP_REQ          0x06
#define IPMI_NETFN_APP_RESP         0x07

/*
 * IPMI命令码定义
 */
#define IPMI_CMD_CHASSIS_CONTROL    0x02
#define IPMI_CMD_GET_CHASSIS_STATUS 0x01
#define IPMI_CMD_GET_SENSOR_READING 0x2D
#define IPMI_CMD_GET_DEVICE_SDR     0x21

/*
 * IPMI Chassis Control子命令
 */
#define IPMI_CHASSIS_POWER_DOWN     0x00
#define IPMI_CHASSIS_POWER_UP       0x01
#define IPMI_CHASSIS_POWER_CYCLE    0x02
#define IPMI_CHASSIS_HARD_RESET     0x03

/*
 * IPMI完成码
 */
#define IPMI_CC_OK                  0x00
#define IPMI_CC_NODE_BUSY           0xC0
#define IPMI_CC_INVALID_CMD         0xC1
#define IPMI_CC_TIMEOUT             0xC3

/*
 * IPMI协议初始化/反初始化
 */
int32_t bmc_ipmi_init(void *transport_handle,
                      int32_t (*send_recv)(void*, const uint8_t*, uint32_t, uint8_t*, uint32_t, uint32_t*),
                      void **protocol_handle);
int32_t bmc_ipmi_deinit(void *protocol_handle);

/*
 * IPMI帧封装/解析
 */
int32_t bmc_ipmi_pack_request(uint8_t netfn,
                              uint8_t cmd,
                              const uint8_t *data,
                              uint32_t data_len,
                              uint8_t *frame,
                              uint32_t frame_size,
                              uint32_t *actual_size);

int32_t bmc_ipmi_unpack_response(const uint8_t *frame,
                                 uint32_t frame_len,
                                 uint8_t *netfn,
                                 uint8_t *cmd,
                                 uint8_t *completion_code,
                                 uint8_t *data,
                                 uint32_t data_size,
                                 uint32_t *actual_size);

/*
 * IPMI业务接口
 */
int32_t bmc_ipmi_power_on(void *protocol_handle);
int32_t bmc_ipmi_power_off(void *protocol_handle);
int32_t bmc_ipmi_power_reset(void *protocol_handle);
int32_t bmc_ipmi_get_power_state(void *protocol_handle, pdl_bmc_power_state_t *state);
int32_t bmc_ipmi_read_sensors(void *protocol_handle,
                              pdl_bmc_sensor_type_t type,
                              pdl_bmc_sensor_reading_t *readings,
                              uint32_t max_count,
                              uint32_t *actual_count);

/************************************************************************
 * Redfish协议层接口（pdl_bmc_redfish.c实现）
 * 职责：实现Redfish RESTful API（HTTP/HTTPS, JSON, 标准资源访问）
 ************************************************************************/

/*
 * Redfish HTTP方法
 */
typedef enum
{
    REDFISH_METHOD_GET = 0x00,
    REDFISH_METHOD_POST,
    REDFISH_METHOD_PATCH,
    REDFISH_METHOD_DELETE
} redfish_method_t;

/*
 * Redfish认证信息
 */
typedef struct
{
    const char *username;
    const char *password;
    char session_token[0x80];
    bool use_session;
} redfish_auth_t;

/*
 * Redfish协议初始化/反初始化
 */
int32_t bmc_redfish_init(void *transport_handle,
                         int32_t (*send_recv)(void*, const uint8_t*, uint32_t, uint8_t*, uint32_t, uint32_t*),
                         const char *username,
                         const char *password,
                         void **protocol_handle);
int32_t bmc_redfish_deinit(void *protocol_handle);

/*
 * Redfish HTTP请求/响应
 */
int32_t bmc_redfish_request(void *protocol_handle,
                            redfish_method_t method,
                            const char *uri,
                            const char *json_body,
                            char *response,
                            uint32_t resp_size,
                            uint32_t *actual_size);

/*
 * Redfish业务接口
 */
int32_t bmc_redfish_power_on(void *protocol_handle);
int32_t bmc_redfish_power_off(void *protocol_handle);
int32_t bmc_redfish_power_reset(void *protocol_handle);
int32_t bmc_redfish_get_power_state(void *protocol_handle, pdl_bmc_power_state_t *state);
int32_t bmc_redfish_read_sensors(void *protocol_handle,
                                 pdl_bmc_sensor_type_t type,
                                 pdl_bmc_sensor_reading_t *readings,
                                 uint32_t max_count,
                                 uint32_t *actual_count);

#endif /* PDL_BMC_INTERNAL_H */
