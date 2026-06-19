/**
 * @file pdm_protocol.c
 * @brief Protocol Layer Public API Implementation
 * @details PDM internal protocol API implementation
 */

#include "osal.h"

#include "pdm_protocol.h"

static bool g_protocol_initialized;

/* Secondary API functions */

bool pdm_protocol_is_device_type_valid(uint8_t dev_type)
{
	return pdm_protocol_device_type_valid(dev_type);
}

const char *pdm_protocol_get_device_type_name(uint8_t dev_type)
{
	return pdm_protocol_device_type_name(dev_type);
}

const char *pdm_protocol_get_error_string(int error_code)
{
	/* 现在使用 OSAL 错误码，直接返回 OSAL 错误描述 */
	return osal_get_status_name(error_code);
}

void pdm_protocol_get_version(uint8_t *major, uint8_t *minor)
{
	if (major) {
		*major = PDM_PROTOCOL_VERSION_MAJOR;
	}
	if (minor) {
		*minor = PDM_PROTOCOL_VERSION_MINOR;
	}
}

/* Initialization and cleanup functions */

int pdm_protocol_init(void)
{
	if (g_protocol_initialized) {
		return OSAL_SUCCESS;
	}

	/* 初始化序列号（可选） */
	/* 这里可以从持久化存储中恢复序列号 */

	g_protocol_initialized = true;
	return OSAL_SUCCESS;
}

int pdm_protocol_deinit(void)
{
	if (!g_protocol_initialized) {
		return OSAL_SUCCESS;
	}

	/* 清理资源（如果有） */

	g_protocol_initialized = false;
	return OSAL_SUCCESS;
}
