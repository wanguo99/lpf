// SPDX-License-Identifier: GPL-2.0

#include <linux/i2c.h>
#include <linux/module.h>

#include "lpf/compat/lpf_compat_errno.h"
#include "lpf/compat/lpf_compat_i2c.h"

static bool lpf_compat_i2c_parse_adapter_id(const char *device,
					    int *adapter_id)
{
	const char *name;

	if (!device || !adapter_id)
		return false;

	name = strrchr(device, '/');
	name = name ? name + 1 : device;

	if (sscanf(name, "i2c-%d", adapter_id) == 1)
		return true;

	if (sscanf(name, "%d", adapter_id) == 1)
		return true;

	return false;
}

int32_t lpf_compat_i2c_open(const char *device, lpf_i2c_handle_t *handle)
{
	struct i2c_adapter *adapter;
	int adapter_id;

	if (!device || !handle)
		return OSAL_ERR_INVALID_PARAM;

	*handle = NULL;
	if (!lpf_compat_i2c_parse_adapter_id(device, &adapter_id))
		return OSAL_ERR_INVALID_PARAM;

	adapter = i2c_get_adapter(adapter_id);
	if (!adapter)
		return OSAL_ENOENT;

	*handle = adapter;
	return OSAL_SUCCESS;
}

void lpf_compat_i2c_close(lpf_i2c_handle_t handle)
{
	if (handle)
		i2c_put_adapter((struct i2c_adapter *)handle);
}

int32_t lpf_compat_i2c_transfer(lpf_i2c_handle_t handle,
				lpf_i2c_msg_t *msgs, uint32_t num)
{
	struct i2c_adapter *adapter = handle;
	struct i2c_msg *kernel_msgs;
	uint32_t i;
	int ret;

	if (!adapter || !msgs || num == 0 || num > 64)
		return OSAL_ERR_INVALID_PARAM;

	kernel_msgs = osal_zalloc(sizeof(*kernel_msgs) * num);
	if (!kernel_msgs)
		return OSAL_ERR_NO_MEMORY;

	for (i = 0; i < num; i++) {
		if (!msgs[i].buf || msgs[i].len == 0) {
			osal_free(kernel_msgs);
			return OSAL_ERR_INVALID_PARAM;
		}

		kernel_msgs[i].addr = msgs[i].addr;
		kernel_msgs[i].flags = msgs[i].flags;
		kernel_msgs[i].len = msgs[i].len;
		kernel_msgs[i].buf = msgs[i].buf;
	}

	ret = i2c_transfer(adapter, kernel_msgs, (int)num);
	osal_free(kernel_msgs);

	if (ret == (int)num)
		return OSAL_SUCCESS;
	if (ret >= 0)
		return OSAL_ERR_GENERIC;

	return lpf_compat_errno_to_status(ret);
}
