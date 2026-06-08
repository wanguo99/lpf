/************************************************************************
 * BMC通信服务对外API
 *
 * 功能：
 * - 与带BMC的载荷通信（IPMI/Redfish协议）
 * - 支持多通道（网络/串口）自动切换
 * - 电源控制、状态查询、传感器读取
 * - 故障检测和自动恢复
 ************************************************************************/

#ifndef PDL_BMC_H
#define PDL_BMC_H


/*===========================================================================
 * BMC 配置类型
 *===========================================================================*/

/**
 * @brief BMC通信通道类型
 */
typedef enum
{
	PDL_BMC_CHANNEL_NETWORK = 0x00,  /* 网络通道（IPMI over LAN） */
	PDL_BMC_CHANNEL_SERIAL  = 0x01   /* 串口通道（IPMI over Serial） */
} pdl_bmc_channel_t;

/**
 * @brief BMC协议类型
 */
typedef enum
{
	PDL_BMC_PROTOCOL_IPMI = 0x00,    /* IPMI协议 */
	PDL_BMC_PROTOCOL_REDFISH = 0x01  /* Redfish协议 */
} pdl_bmc_protocol_t;

/**
 * @brief BMC通道配置（union形式）
 *
 * 使用union表示互斥的通道类型配置
 */
typedef union
{
	/* 网络通道配置 */
	struct {
		const char *ip_addr;      /* IP地址 */
		uint16_t port;            /* 端口（默认623） */
		const char *username;     /* 用户名 */
		const char *password;     /* 密码 */
		uint32_t timeout_ms;      /* 超时时间 */
	} network;

	/* 串口通道配置 */
	struct {
		const char *device;       /* 串口设备（传递给 HAL） */
		uint32_t baudrate;        /* 波特率（传递给 HAL） */
		uint8_t data_bits;        /* 数据位（传递给 HAL，默认8） */
		uint8_t stop_bits;        /* 停止位（传递给 HAL，默认1） */
		uint8_t parity;           /* 校验位（传递给 HAL，默认NONE） */
		uint32_t timeout_ms;      /* 超时时间 */
	} serial;
} pdl_bmc_channel_config_t;

/**
 * @brief BMC配置
 *
 * 设计说明：
 * - 使用双union模式支持主备通道配置
 * - primary_channel指定主通道类型
 * - backup_channel指定备用通道类型
 * - auto_switch启用时，主通道故障自动切换到备用通道
 * - 每个通道配置使用union，节省内存的同时保持灵活性
 */
typedef struct
{
	/* 主通道配置 */
	pdl_bmc_channel_t primary_channel;       /* 主通道类型 */
	pdl_bmc_channel_config_t primary_config; /* 主通道配置（union） */

	/* 备用通道配置（用于auto_switch） */
	pdl_bmc_channel_t backup_channel;        /* 备用通道类型 */
	pdl_bmc_channel_config_t backup_config;  /* 备用通道配置（union） */

	/* 服务配置 */
	bool auto_switch;                        /* 自动切换通道 */
	uint32_t retry_count;                    /* 重试次数 */
	uint32_t health_check_interval;          /* 健康检查间隔(ms) */
} pdl_bmc_config_t;

/*
 * BMC服务句柄
 */
typedef void* pdl_bmc_handle_t;

/*
 * 电源状态
 */
typedef enum
{
	PDL_BMC_POWER_OFF = 0x00,
	PDL_BMC_POWER_ON  = 0x01,
	PDL_BMC_POWER_UNKNOWN = 0x02
} pdl_bmc_power_state_t;

/*
 * BMC状态
 */
typedef struct
{
	pdl_bmc_power_state_t power_state;
	bool bmc_ready;
	uint32_t uptime_sec;
	float cpu_temp;
	float inlet_temp;
	uint64_t timestamp_us;  /* 数据采集时间戳（微秒） */
} pdl_bmc_status_t;

/*
 * 传感器类型
 */
typedef enum
{
	PDL_BMC_SENSOR_TEMP = 0x00,      /* 温度 */
	PDL_BMC_SENSOR_VOLTAGE = 0x01,   /* 电压 */
	PDL_BMC_SENSOR_CURRENT = 0x02,   /* 电流 */
	PDL_BMC_SENSOR_FAN = 0x03        /* 风扇转速 */
} pdl_bmc_sensor_type_t;

/*
 * 传感器读数
 */
typedef struct
{
	pdl_bmc_sensor_type_t type;
	char name[64];
	float value;
	char unit[16];
	bool valid;
	uint64_t timestamp_us;  /* 数据采集时间戳（微秒） */
} pdl_bmc_sensor_reading_t;

/**
 * @brief 初始化BMC服务
 *
 * @param[in] config 配置参数
 * @param[out] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_BMC_Init(const pdl_bmc_config_t *config,
                   pdl_bmc_handle_t *handle);

/**
 * @brief 反初始化BMC服务
 *
 * @param[in] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_Deinit(pdl_bmc_handle_t handle);

/**
 * @brief 电源开机
 *
 * @param[in] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_TIMEOUT 超时
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_BMC_PowerOn(pdl_bmc_handle_t handle);

/**
 * @brief 电源关机
 *
 * @param[in] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_PowerOff(pdl_bmc_handle_t handle);

/**
 * @brief 电源复位
 *
 * @param[in] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_PowerReset(pdl_bmc_handle_t handle);

/**
 * @brief 查询电源状态
 *
 * @param[in] handle 服务句柄
 * @param[out] state 电源状态
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_GetPowerState(pdl_bmc_handle_t handle,
                            pdl_bmc_power_state_t *state);

/**
 * @brief 读取传感器
 *
 * @param[in] handle 服务句柄
 * @param[in] type 传感器类型
 * @param[out] readings 读数数组
 * @param[in] max_count 数组大小
 * @param[out] actual_count 实际读取数量
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_ReadSensors(pdl_bmc_handle_t handle,
                          pdl_bmc_sensor_type_t type,
                          pdl_bmc_sensor_reading_t *readings,
                          uint32_t max_count,
                          uint32_t *actual_count);

/**
 * @brief 执行原始IPMI命令
 *
 * @param[in] handle 服务句柄
 * @param[in] cmd 命令字符串
 * @param[out] response 响应缓冲区
 * @param[in] resp_size 缓冲区大小
 *
 * @return 实际接收字节数
 * @return <0 错误码
 */
int32_t PDL_BMC_ExecuteCommand(pdl_bmc_handle_t handle,
                             const char *cmd,
                             char *response,
                             uint32_t resp_size);

/**
 * @brief 切换通信通道
 *
 * @param[in] handle 服务句柄
 * @param[in] channel 目标通道
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_SwitchChannel(pdl_bmc_handle_t handle,
                            pdl_bmc_channel_t channel);

/**
 * @brief 获取当前通道
 *
 * @param[in] handle 服务句柄
 *
 * @return pdl_bmc_channel_t 当前通道
 */
pdl_bmc_channel_t PDL_BMC_GetChannel(pdl_bmc_handle_t handle);

/**
 * @brief 检查连接状态
 *
 * @param[in] handle 服务句柄
 *
 * @return true 已连接
 * @return false 未连接
 */
bool PDL_BMC_IsConnected(pdl_bmc_handle_t handle);

/**
 * @brief 获取服务统计信息
 *
 * @param[in] handle 服务句柄
 * @param[out] cmd_count 命令总数
 * @param[out] success_count 成功数
 * @param[out] fail_count 失败数
 * @param[out] switch_count 通道切换次数
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_BMC_GetStats(pdl_bmc_handle_t handle,
                       uint32_t *cmd_count,
                       uint32_t *success_count,
                       uint32_t *fail_count,
                       uint32_t *switch_count);

#endif /* PDL_BMC_H */
