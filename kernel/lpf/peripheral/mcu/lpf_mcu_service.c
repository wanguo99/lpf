/************************************************************************
 * MCU外设驱动实现
 *
 * 职责：
 * - 提供对外API（初始化、命令收发）
 * - 直接使用 PDM protocol 层进行协议编解码
 * - 选择通信层（CAN/串口）
 ************************************************************************/

#include "osal.h"
#include "hal.h"
#include "pconfig.h"
#include "pdm_protocol.h"
#include "lpf_mcu_internal.h"

/*===========================================================================
 * MCU上下文
 *===========================================================================*/

typedef struct {
	const pconfig_mcu_config_t *config;
	const lpf_mcu_ops_t *ops;
	void *transport_handle;
	osal_mutex_t mutex;
} lpf_mcu_context_t;

static lpf_mcu_context_t *g_mcu_contexts[LPF_MCU_MAX_DEVICES] = { NULL };
static osal_mutex_t g_registry_mutex;
static bool g_registry_initialized = false;

static const lpf_driver_t g_lpf_mcu_driver;

static void lpf_mcu_remove_index(uint32_t index);

/*===========================================================================
 * 内部辅助函数
 *===========================================================================*/

/**
 * @brief 发送 PDM protocol 报文
 */
static int32_t _mcu_send_packet(lpf_mcu_context_t *ctx, uint8_t msg_type,
								const void *payload, uint16_t payload_len,
								uint8_t *response, uint32_t resp_size,
								uint32_t *actual_size)
{
	uint8_t *tx_buffer;
	uint8_t *rx_buffer;
	int32_t tx_len;
	int32_t ret;
	uint32_t rx_len;

	tx_buffer = osal_malloc(PDM_PROTOCOL_MAX_PACKET_SIZE);
	rx_buffer = osal_malloc(PDM_PROTOCOL_MAX_PACKET_SIZE);
	if (!tx_buffer || !rx_buffer) {
		ret = OSAL_ERR_NO_MEMORY;
		goto out_free;
	}

	/* 编码 PDM protocol 报文 */
	pdm_protocol_encode_ctx_t encode_ctx = { .dev_type = PDM_PROTOCOL_DEV_TYPE_MCU,
									.msg_type = msg_type,
									.flags = 0,
									.payload = payload,
									.payload_len = payload_len,
									.buffer = tx_buffer,
									.buffer_size = PDM_PROTOCOL_MAX_PACKET_SIZE };
	tx_len = pdm_protocol_encode(&encode_ctx);
	if (tx_len < 0) {
		ret = tx_len;
		goto out_free;
	}

	/* 发送并接收响应 */
	ret = ctx->ops->send_packet(ctx->transport_handle, tx_buffer, tx_len,
								rx_buffer, PDM_PROTOCOL_MAX_PACKET_SIZE, &rx_len,
								ctx->config->cmd_timeout_ms);
	if (ret != OSAL_SUCCESS) {
		goto out_free;
	}

	/* 解码响应报文 */
	pdm_protocol_decode_ctx_t decode_ctx = { .buffer = rx_buffer,
									.buffer_len = rx_len,
									.payload = response,
									.payload_size = resp_size };
	ret = pdm_protocol_decode(&decode_ctx);
	if (ret == OSAL_SUCCESS && actual_size) {
		*actual_size = decode_ctx.payload_len;
	}

out_free:
	osal_free(rx_buffer);
	osal_free(tx_buffer);
	return ret;
}

static int32_t lpf_mcu_registry_init(void)
{
	if (g_registry_initialized)
		return OSAL_SUCCESS;

	if (OSAL_SUCCESS != osal_mutex_init(&g_registry_mutex, NULL))
		return OSAL_ERR_GENERIC;

	g_registry_initialized = true;
	return OSAL_SUCCESS;
}

/*===========================================================================
 * 基础 API 实现
 *===========================================================================*/

/**
 * @brief 发送简单命令到MCU（无发送数据）
 */
int32_t lpf_mcu_send_cmd(lpf_mcu_handle_t handle, lpf_mcu_cmd_t *cmd)
{
	lpf_mcu_context_t *ctx = (lpf_mcu_context_t *)handle;
	int32_t ret;

	if (!ctx || !cmd) {
		return OSAL_ERR_INVALID_PARAM;
	}

	osal_mutex_lock(&ctx->mutex);

	ret = _mcu_send_packet(ctx, cmd->cmd, NULL, 0, cmd->response,
						   cmd->response_max, &cmd->response_len);

	osal_mutex_unlock(&ctx->mutex);

	return ret;
}

/**
 * @brief 发送数据命令到MCU（带发送数据）
 */
int32_t lpf_mcu_send_data(lpf_mcu_handle_t handle, lpf_mcu_data_t *data)
{
	lpf_mcu_context_t *ctx = (lpf_mcu_context_t *)handle;
	int32_t ret;

	if (!ctx || !data) {
		return OSAL_ERR_INVALID_PARAM;
	}

	osal_mutex_lock(&ctx->mutex);

	ret = _mcu_send_packet(ctx, data->cmd, data->data, data->data_len,
						   data->response, data->response_max,
						   &data->response_len);

	osal_mutex_unlock(&ctx->mutex);

	return ret;
}

/*===========================================================================
 * 对外API实现
 *===========================================================================*/

/**
 * @brief 初始化 MCU 驱动
 */
static int32_t lpf_mcu_init_from_entry(uint32_t mcu_index,
				       const pconfig_mcu_entry_t *entry,
				       lpf_mcu_handle_t *handle)
{
	lpf_mcu_context_t *ctx;
	const lpf_mcu_ops_t *ops;
	int32_t ret;

	if (!handle) {
		return OSAL_ERR_INVALID_PARAM;
	}

	if (!entry || !entry->enabled) {
		return OSAL_ERR_GENERIC;
	}

	if (mcu_index >= OSAL_ARRAY_SIZE(g_mcu_contexts))
		return OSAL_ERR_INVALID_PARAM;

	/* 初始化全局注册表互斥锁 */
	if (lpf_mcu_registry_init() != OSAL_SUCCESS)
		return OSAL_ERR_GENERIC;

	/* 选择通信层 */
	switch (entry->config.interface) {
	case PCONFIG_MCU_INTERFACE_CAN:
		ops = &lpf_mcu_can_ops;
		break;
	case PCONFIG_MCU_INTERFACE_SERIAL:
		ops = &lpf_mcu_serial_ops;
		break;
	default:
		return OSAL_ERR_GENERIC;
	}

	osal_mutex_lock(&g_registry_mutex);
	if (g_mcu_contexts[mcu_index]) {
		*handle = g_mcu_contexts[mcu_index];
		osal_mutex_unlock(&g_registry_mutex);
		return OSAL_SUCCESS;
	}
	osal_mutex_unlock(&g_registry_mutex);

	/* 分配上下文 */
	ctx = (lpf_mcu_context_t *)osal_malloc(sizeof(lpf_mcu_context_t));
	if (!ctx) {
		return OSAL_ERR_NO_MEMORY;
	}

	osal_memset(ctx, 0, sizeof(lpf_mcu_context_t));
	ctx->config = &entry->config;
	ctx->ops = ops;

	/* 初始化通信层 */
	ret = ops->init(&entry->config, &ctx->transport_handle);
	if (ret != OSAL_SUCCESS) {
		osal_free(ctx);
		return ret;
	}

	/* 创建互斥锁 */
	if (OSAL_SUCCESS != osal_mutex_init(&ctx->mutex, NULL)) {
		ops->deinit(ctx->transport_handle);
		osal_free(ctx);
		return OSAL_ERR_GENERIC;
	}

	/* 注册上下文 */
	osal_mutex_lock(&g_registry_mutex);
	if (g_mcu_contexts[mcu_index]) {
		*handle = g_mcu_contexts[mcu_index];
		osal_mutex_unlock(&g_registry_mutex);
		osal_mutex_destroy(&ctx->mutex);
		ops->deinit(ctx->transport_handle);
		osal_free(ctx);
		return OSAL_SUCCESS;
	}
	g_mcu_contexts[mcu_index] = ctx;
	osal_mutex_unlock(&g_registry_mutex);

	*handle = ctx;
	return OSAL_SUCCESS;
}

int32_t lpf_mcu_init(uint32_t mcu_index, lpf_mcu_handle_t *handle)
{
	const pconfig_platform_config_t *platform;
	const pconfig_mcu_entry_t *entry;

	platform = pconfig_get_board();
	if (!platform)
		return OSAL_ERR_GENERIC;

	entry = pconfig_hw_get_mcu(platform, mcu_index);
	return lpf_mcu_init_from_entry(mcu_index, entry, handle);
}

int32_t lpf_mcu_probe(const lpf_device_t *device)
{
	const pconfig_mcu_entry_t *entry;
	lpf_mcu_handle_t handle = NULL;
	int32_t ret;

	if (!device || device->config.type != LPF_DEVICE_TYPE_MCU)
		return OSAL_ERR_INVALID_PARAM;

	entry = (const pconfig_mcu_entry_t *)device->config.entry;
	ret = lpf_mcu_init_from_entry(device->config.index, entry, &handle);
	if (ret != OSAL_SUCCESS)
		return ret;

	ret = lpf_mcu_chrdev_register_device(device);
	if (ret) {
		lpf_mcu_remove_index(device->config.index);
		return OSAL_ERR_GENERIC;
	}

	return OSAL_SUCCESS;
}

lpf_mcu_handle_t lpf_mcu_get(uint32_t index)
{
	lpf_mcu_handle_t handle = NULL;

	if (lpf_mcu_registry_init() != OSAL_SUCCESS)
		return NULL;

	if (index >= OSAL_ARRAY_SIZE(g_mcu_contexts))
		return NULL;

	osal_mutex_lock(&g_registry_mutex);
	handle = g_mcu_contexts[index];
	osal_mutex_unlock(&g_registry_mutex);

	return handle;
}

int32_t lpf_mcu_debug_get(uint32_t index, lpf_mcu_debug_info_t *info)
{
	lpf_mcu_context_t *ctx;

	if (!info || index >= OSAL_ARRAY_SIZE(g_mcu_contexts))
		return OSAL_ERR_INVALID_PARAM;

	osal_memset(info, 0, sizeof(*info));

	if (!g_registry_initialized)
		return OSAL_ERR_INVALID_STATE;

	osal_mutex_lock(&g_registry_mutex);
	ctx = g_mcu_contexts[index];
	if (ctx) {
		info->present = true;
		osal_strncpy(info->name, ctx->config->name,
			     sizeof(info->name));
		info->name[sizeof(info->name) - 1] = '\0';
		info->interface = ctx->config->interface;
		info->cmd_timeout_ms = ctx->config->cmd_timeout_ms;
	}
	osal_mutex_unlock(&g_registry_mutex);

	return OSAL_SUCCESS;
}

static void lpf_mcu_remove_index(uint32_t index)
{
	lpf_mcu_context_t *ctx;

	if (!g_registry_initialized ||
	    index >= OSAL_ARRAY_SIZE(g_mcu_contexts))
		return;

	osal_mutex_lock(&g_registry_mutex);
	ctx = g_mcu_contexts[index];
	g_mcu_contexts[index] = NULL;
	osal_mutex_unlock(&g_registry_mutex);

	if (!ctx)
		return;

	ctx->ops->deinit(ctx->transport_handle);
	osal_mutex_destroy(&ctx->mutex);
	osal_free(ctx);
}

void lpf_mcu_remove(const lpf_device_t *device)
{
	if (!device || device->config.type != LPF_DEVICE_TYPE_MCU)
		return;

	lpf_mcu_chrdev_unregister_device(device);
	lpf_mcu_remove_index(device->config.index);
}

static void lpf_mcu_registry_deinit(void)
{
	uint32_t i;

	if (!g_registry_initialized)
		return;

	for (i = 0; i < OSAL_ARRAY_SIZE(g_mcu_contexts); i++)
		lpf_mcu_remove_index(i);

	osal_mutex_destroy(&g_registry_mutex);
	g_registry_initialized = false;
}

/**
 * @brief 反初始化 MCU 驱动
 */
int32_t lpf_mcu_deinit(lpf_mcu_handle_t handle)
{
	lpf_mcu_context_t *ctx = (lpf_mcu_context_t *)handle;
	uint32_t i;

	if (!ctx) {
		return OSAL_ERR_INVALID_PARAM;
	}

	/* 从注册表移除 */
	osal_mutex_lock(&g_registry_mutex);
	for (i = 0; i < sizeof(g_mcu_contexts) / sizeof(g_mcu_contexts[0]); i++) {
		if (g_mcu_contexts[i] == ctx) {
			g_mcu_contexts[i] = NULL;
			break;
		}
	}
	osal_mutex_unlock(&g_registry_mutex);

	/* 清理资源 */
	ctx->ops->deinit(ctx->transport_handle);
	osal_mutex_destroy(&ctx->mutex);
	osal_free(ctx);

	return OSAL_SUCCESS;
}

/**
 * @brief 获取 MCU 版本信息
 */
int32_t lpf_mcu_get_version(lpf_mcu_handle_t handle, lpf_mcu_version_t *version)
{
	uint8_t response[64];
	int32_t ret;

	if (!handle || !version) {
		return OSAL_ERR_INVALID_PARAM;
	}

	/* 发送 GET_VERSION 命令 */
	lpf_mcu_cmd_t cmd = { .cmd = PDM_PROTOCOL_MCU_MSG_GET_VERSION,
						  .response = response,
						  .response_max = sizeof(response) };

	ret = lpf_mcu_send_cmd(handle, &cmd);
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	/* 解析版本信息 */
	if (cmd.response_len >= 3) {
		version->major = response[0];
		version->minor = response[1];
		version->patch = response[2];
		version->build = (cmd.response_len >= 4) ? response[3] : 0;
		osal_snprintf(version->version_string,
			      sizeof(version->version_string), "%u.%u.%u.%u",
			      version->major, version->minor, version->patch,
			      version->build);
	} else {
		return OSAL_ERR_GENERIC;
	}

	return OSAL_SUCCESS;
}

/**
 * @brief 获取 MCU 状态
 */
int32_t lpf_mcu_get_status(lpf_mcu_handle_t handle, lpf_mcu_status_t *status)
{
	uint8_t response[64];
	int32_t ret;

	if (!handle || !status) {
		return OSAL_ERR_INVALID_PARAM;
	}

	/* 发送 GET_STATUS 命令 */
	lpf_mcu_cmd_t cmd = { .cmd = PDM_PROTOCOL_MCU_MSG_GET_STATUS,
						  .response = response,
						  .response_max = sizeof(response) };

	ret = lpf_mcu_send_cmd(handle, &cmd);
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	osal_memset(status, 0, sizeof(*status));

	/* 解析状态信息 */
	if (cmd.response_len >= 1) {
		status->state = response[0];
		status->online = (status->state != LPF_MCU_STATE_OFFLINE);
	} else {
		return OSAL_ERR_GENERIC;
	}

	if (cmd.response_len >= 2) {
		status->error_code = response[1];
	}

	if (cmd.response_len >= 4) {
		status->uptime_sec =
			((uint32_t)response[2] << 8) | (uint32_t)response[3];
	}

	status->timestamp_us = osal_get_monotonic_time();

	return OSAL_SUCCESS;
}

/**
 * @brief MCU 复位
 */
int32_t lpf_mcu_reset(lpf_mcu_handle_t handle)
{
	uint8_t response[64];

	if (!handle) {
		return OSAL_ERR_INVALID_PARAM;
	}

	/* 发送 RESET 命令 */
	lpf_mcu_cmd_t cmd = { .cmd = PDM_PROTOCOL_MCU_MSG_RESET,
						  .response = response,
						  .response_max = sizeof(response) };

	return lpf_mcu_send_cmd(handle, &cmd);
}

/**
 * @brief 读取 MCU 数据
 */
int32_t lpf_mcu_read_data(lpf_mcu_handle_t handle, uint32_t addr, uint8_t *data,
						  uint32_t size)
{
	uint8_t request[8];
	int32_t ret;

	if (!handle || !data || size == 0) {
		return OSAL_ERR_INVALID_PARAM;
	}

	/* 构造请求 */
	request[0] = (addr >> 24) & 0xFF;
	request[1] = (addr >> 16) & 0xFF;
	request[2] = (addr >> 8) & 0xFF;
	request[3] = addr & 0xFF;
	request[4] = (size >> 24) & 0xFF;
	request[5] = (size >> 16) & 0xFF;
	request[6] = (size >> 8) & 0xFF;
	request[7] = size & 0xFF;

	/* 发送 READ_DATA 命令 */
	lpf_mcu_data_t send_data = { .cmd = PDM_PROTOCOL_MCU_MSG_READ_DATA,
								 .data = request,
								 .data_len = 8,
								 .response = data,
								 .response_max = size };

	ret = lpf_mcu_send_data(handle, &send_data);
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	return OSAL_SUCCESS;
}

/**
 * @brief 写入 MCU 数据
 */
int32_t lpf_mcu_write_data(lpf_mcu_handle_t handle, uint32_t addr,
						   const uint8_t *data, uint32_t size)
{
	uint8_t request[256];
	uint8_t response[64];

	if (!handle || !data || size == 0 || size > 248) {
		return OSAL_ERR_INVALID_PARAM;
	}

	/* 构造请求 */
	request[0] = (addr >> 24) & 0xFF;
	request[1] = (addr >> 16) & 0xFF;
	request[2] = (addr >> 8) & 0xFF;
	request[3] = addr & 0xFF;
	request[4] = (size >> 24) & 0xFF;
	request[5] = (size >> 16) & 0xFF;
	request[6] = (size >> 8) & 0xFF;
	request[7] = size & 0xFF;
	osal_memcpy(&request[8], data, size);

	/* 发送 WRITE_DATA 命令 */
	lpf_mcu_data_t send_data = { .cmd = PDM_PROTOCOL_MCU_MSG_WRITE_DATA,
								 .data = request,
								 .data_len = 8 + size,
								 .response = response,
								 .response_max = sizeof(response) };

	return lpf_mcu_send_data(handle, &send_data);
}

/**
 * @brief 执行 MCU 命令
 */
int32_t lpf_mcu_send_command(lpf_mcu_handle_t handle, uint8_t cmd_id,
							 const uint8_t *params, uint32_t param_len,
							 uint8_t *result, uint32_t result_size,
							 uint32_t *actual_len)
{
	uint8_t request[256];
	int32_t ret;

	if (!handle || param_len > 255) {
		return OSAL_ERR_INVALID_PARAM;
	}

	/* 构造请求 */
	request[0] = cmd_id;
	if (params && param_len > 0) {
		osal_memcpy(&request[1], params, param_len);
	}

	/* 发送 EXECUTE_CMD 命令 */
	lpf_mcu_data_t send_data = { .cmd = PDM_PROTOCOL_MCU_MSG_EXECUTE_CMD,
								 .data = request,
								 .data_len = 1 + param_len,
								 .response = result,
								 .response_max = result_size };

	ret = lpf_mcu_send_data(handle, &send_data);
	if (ret == OSAL_SUCCESS && actual_len) {
		*actual_len = send_data.response_len;
	}

	return ret;
}

static int lpf_mcu_driver_init(void)
{
	int ret;

	ret = lpf_mcu_chrdev_register();
	if (ret)
		return ret;

	ret = lpf_mcu_proc_register();
	if (ret) {
		lpf_mcu_chrdev_unregister();
		return ret;
	}

	ret = lpf_mcu_debugfs_register();
	if (ret) {
		lpf_mcu_proc_unregister();
		lpf_mcu_chrdev_unregister();
		return ret;
	}

	LOG_INFO("LPF_MCU", "registered");
	return 0;
}

static void lpf_mcu_driver_exit(void)
{
	lpf_mcu_registry_deinit();
	lpf_mcu_debugfs_unregister();
	lpf_mcu_proc_unregister();
	lpf_mcu_chrdev_unregister();
	LOG_INFO("LPF_MCU", "unregistered");
}

static const lpf_driver_t g_lpf_mcu_driver = {
	.name = "mcu",
	.type = LPF_DEVICE_TYPE_MCU,
	.capabilities = LPF_DEVICE_CAP_USER_IOCTL | LPF_DEVICE_CAP_DEBUGFS,
	.init = lpf_mcu_driver_init,
	.exit = lpf_mcu_driver_exit,
	.probe = lpf_mcu_probe,
	.remove = lpf_mcu_remove,
};

int32_t lpf_mcu_service_register(void)
{
	return lpf_driver_register(&g_lpf_mcu_driver);
}

void lpf_mcu_service_unregister(void)
{
	lpf_driver_unregister(&g_lpf_mcu_driver);
}
