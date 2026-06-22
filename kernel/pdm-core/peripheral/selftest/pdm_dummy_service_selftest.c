// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "lpf/core/lpf_core.h"
#include "lpf/lpf_errno.h"

#define LPF_DUMMY_SELFTEST_INDEX 0U
#define LPF_DUMMY_SELFTEST_NAME "dummy-selftest0"
#define LPF_DUMMY_SELFTEST_DRIVER "dummy-selftest"
#define LPF_DUMMY_SELFTEST_CAP (1ULL << 63)

typedef struct {
	uint32_t init_count;
	uint32_t exit_count;
	uint32_t probe_count;
	uint32_t remove_count;
	uint32_t events[LPF_DEVICE_EVENT_REMOVED + 1U];
} lpf_dummy_selftest_state_t;

static lpf_dummy_selftest_state_t g_lpf_dummy_selftest_state;

static int32_t lpf_dummy_selftest_fail(const char *message)
{
	pr_err("LPF:DUMMY_SELFTEST: %s\n", message);
	return OSAL_ERR_GENERIC;
}

#define LPF_DUMMY_SELFTEST_EXPECT(condition, message) \
	do { \
		if (!(condition)) \
			return lpf_dummy_selftest_fail(message); \
	} while (0)

static bool lpf_dummy_selftest_is_target(const lpf_device_info_t *info)
{
	return info && info->type == LPF_DEVICE_TYPE_DUMMY &&
	       info->index == LPF_DUMMY_SELFTEST_INDEX;
}

static void lpf_dummy_selftest_event(const lpf_device_event_t *event,
				     void *user_data)
{
	lpf_dummy_selftest_state_t *state = user_data;

	if (!event || !state || !lpf_dummy_selftest_is_target(&event->device))
		return;

	if (event->type <= LPF_DEVICE_EVENT_REMOVED)
		state->events[event->type]++;
}

static int lpf_dummy_selftest_driver_init(void)
{
	g_lpf_dummy_selftest_state.init_count++;
	return 0;
}

static void lpf_dummy_selftest_driver_exit(void)
{
	g_lpf_dummy_selftest_state.exit_count++;
}

static int32_t lpf_dummy_selftest_probe(const lpf_device_t *device)
{
	LPF_DUMMY_SELFTEST_EXPECT(device != NULL, "probe device is NULL");
	LPF_DUMMY_SELFTEST_EXPECT(device->config.type == LPF_DEVICE_TYPE_DUMMY,
				  "probe type mismatch");
	LPF_DUMMY_SELFTEST_EXPECT(device->config.index ==
					  LPF_DUMMY_SELFTEST_INDEX,
				  "probe index mismatch");

	g_lpf_dummy_selftest_state.probe_count++;
	return OSAL_SUCCESS;
}

static void lpf_dummy_selftest_remove(const lpf_device_t *device)
{
	if (!device || device->config.type != LPF_DEVICE_TYPE_DUMMY ||
	    device->config.index != LPF_DUMMY_SELFTEST_INDEX)
		return;

	g_lpf_dummy_selftest_state.remove_count++;
}

static const lpf_driver_t g_lpf_dummy_selftest_driver = {
	.name = LPF_DUMMY_SELFTEST_DRIVER,
	.type = LPF_DEVICE_TYPE_DUMMY,
	.capabilities = LPF_DUMMY_SELFTEST_CAP,
	.init = lpf_dummy_selftest_driver_init,
	.exit = lpf_dummy_selftest_driver_exit,
	.probe = lpf_dummy_selftest_probe,
	.remove = lpf_dummy_selftest_remove,
};

static const lpf_device_config_t g_lpf_dummy_selftest_device = {
	.type = LPF_DEVICE_TYPE_DUMMY,
	.index = LPF_DUMMY_SELFTEST_INDEX,
	.name = LPF_DUMMY_SELFTEST_NAME,
	.capabilities = LPF_DEVICE_CAP_DEBUGFS,
};

static int32_t lpf_dummy_selftest_expect_info(const lpf_device_info_t *info,
					      lpf_device_state_t state,
					      int32_t last_error,
					      uint32_t error_count)
{
	LPF_DUMMY_SELFTEST_EXPECT(lpf_dummy_selftest_is_target(info),
				  "device info target mismatch");
	LPF_DUMMY_SELFTEST_EXPECT(info->state == state,
				  "device state mismatch");
	LPF_DUMMY_SELFTEST_EXPECT(info->last_error == last_error,
				  "device last_error mismatch");
	LPF_DUMMY_SELFTEST_EXPECT(info->error_count == error_count,
				  "device error_count mismatch");
	LPF_DUMMY_SELFTEST_EXPECT(
		(info->capabilities & LPF_DUMMY_SELFTEST_CAP) != 0,
		"driver capability missing");
	LPF_DUMMY_SELFTEST_EXPECT(
		(info->capabilities & LPF_DEVICE_CAP_DEBUGFS) != 0,
		"device capability missing");
	LPF_DUMMY_SELFTEST_EXPECT(osal_strcmp(info->name,
					      LPF_DUMMY_SELFTEST_NAME) == 0,
				  "device name mismatch");
	LPF_DUMMY_SELFTEST_EXPECT(osal_strcmp(info->driver_name,
					      LPF_DUMMY_SELFTEST_DRIVER) == 0,
				  "driver name mismatch");
	return OSAL_SUCCESS;
}

static int32_t lpf_dummy_selftest_run(void)
{
	lpf_device_handle_t *handle = NULL;
	lpf_device_info_t info;
	uint32_t count = 0;
	int32_t ret;

	osal_memset(&g_lpf_dummy_selftest_state, 0,
		    sizeof(g_lpf_dummy_selftest_state));

	ret = lpf_device_event_subscribe(lpf_dummy_selftest_event,
					 &g_lpf_dummy_selftest_state);
	if (ret != OSAL_SUCCESS)
		return ret;

	ret = lpf_driver_register(&g_lpf_dummy_selftest_driver);
	if (ret != OSAL_SUCCESS)
		goto out_unsubscribe;

	ret = lpf_driver_register(&g_lpf_dummy_selftest_driver);
	if (ret != OSAL_ERR_ALREADY_EXISTS) {
		ret = lpf_dummy_selftest_fail("duplicate driver not rejected");
		goto out_unregister;
	}

	ret = lpf_device_register(&g_lpf_dummy_selftest_device);
	if (ret != OSAL_SUCCESS)
		goto out_unregister;

	ret = lpf_device_register(&g_lpf_dummy_selftest_device);
	if (ret != OSAL_ERR_ALREADY_EXISTS) {
		ret = lpf_dummy_selftest_fail("duplicate device not rejected");
		goto out_unregister;
	}

	LPF_DUMMY_SELFTEST_EXPECT(g_lpf_dummy_selftest_state.init_count == 1U,
				  "driver init count mismatch");
	LPF_DUMMY_SELFTEST_EXPECT(g_lpf_dummy_selftest_state.probe_count == 1U,
				  "driver probe count mismatch");

	ret = lpf_device_get_info(LPF_DEVICE_TYPE_DUMMY,
				  LPF_DUMMY_SELFTEST_INDEX, &info);
	if (ret != OSAL_SUCCESS)
		goto out_unregister;
	ret = lpf_dummy_selftest_expect_info(&info, LPF_DEVICE_STATE_BOUND,
					     OSAL_SUCCESS, 0U);
	if (ret != OSAL_SUCCESS)
		goto out_unregister;

	ret = lpf_device_get_info_by_name(LPF_DUMMY_SELFTEST_NAME, &info);
	if (ret != OSAL_SUCCESS)
		goto out_unregister;
	ret = lpf_dummy_selftest_expect_info(&info, LPF_DEVICE_STATE_BOUND,
					     OSAL_SUCCESS, 0U);
	if (ret != OSAL_SUCCESS)
		goto out_unregister;

	handle = lpf_device_get_by_capability(LPF_DUMMY_SELFTEST_CAP, 0U);
	if (!handle) {
		ret = lpf_dummy_selftest_fail("capability handle lookup failed");
		goto out_unregister;
	}
	ret = lpf_device_handle_get_info(handle, &info);
	lpf_device_put(handle);
	handle = NULL;
	if (ret != OSAL_SUCCESS)
		goto out_unregister;
	ret = lpf_dummy_selftest_expect_info(&info, LPF_DEVICE_STATE_BOUND,
					     OSAL_SUCCESS, 0U);
	if (ret != OSAL_SUCCESS)
		goto out_unregister;

	ret = lpf_device_list(NULL, &count);
	if (ret != OSAL_SUCCESS)
		goto out_unregister;
	LPF_DUMMY_SELFTEST_EXPECT(count > 0U, "device list count is zero");

	lpf_device_record_error(LPF_DEVICE_TYPE_DUMMY,
				LPF_DUMMY_SELFTEST_INDEX, OSAL_ERR_BUSY);
	ret = lpf_device_get_info(LPF_DEVICE_TYPE_DUMMY,
				  LPF_DUMMY_SELFTEST_INDEX, &info);
	if (ret != OSAL_SUCCESS)
		goto out_unregister;
	ret = lpf_dummy_selftest_expect_info(&info, LPF_DEVICE_STATE_ERROR,
					     OSAL_ERR_BUSY, 1U);
	if (ret != OSAL_SUCCESS)
		goto out_unregister;

	ret = lpf_device_record_recovery(LPF_DEVICE_TYPE_DUMMY,
					 LPF_DUMMY_SELFTEST_INDEX);
	if (ret != OSAL_SUCCESS)
		goto out_unregister;
	ret = lpf_device_get_info(LPF_DEVICE_TYPE_DUMMY,
				  LPF_DUMMY_SELFTEST_INDEX, &info);
	if (ret != OSAL_SUCCESS)
		goto out_unregister;
	ret = lpf_dummy_selftest_expect_info(&info, LPF_DEVICE_STATE_BOUND,
					     OSAL_ERR_BUSY, 1U);
	if (ret != OSAL_SUCCESS)
		goto out_unregister;

	lpf_driver_unregister(&g_lpf_dummy_selftest_driver);

	LPF_DUMMY_SELFTEST_EXPECT(g_lpf_dummy_selftest_state.remove_count == 1U,
				  "driver remove count mismatch");
	LPF_DUMMY_SELFTEST_EXPECT(g_lpf_dummy_selftest_state.exit_count == 1U,
				  "driver exit count mismatch");
	LPF_DUMMY_SELFTEST_EXPECT(
		g_lpf_dummy_selftest_state.events[LPF_DEVICE_EVENT_REGISTERED] ==
			1U,
		"registered event count mismatch");
	LPF_DUMMY_SELFTEST_EXPECT(
		g_lpf_dummy_selftest_state.events[LPF_DEVICE_EVENT_BOUND] == 1U,
		"bound event count mismatch");
	LPF_DUMMY_SELFTEST_EXPECT(
		g_lpf_dummy_selftest_state
			.events[LPF_DEVICE_EVENT_STATE_CHANGED] == 2U,
		"state event count mismatch");
	LPF_DUMMY_SELFTEST_EXPECT(
		g_lpf_dummy_selftest_state.events[LPF_DEVICE_EVENT_ERROR] == 1U,
		"error event count mismatch");
	LPF_DUMMY_SELFTEST_EXPECT(
		g_lpf_dummy_selftest_state.events[LPF_DEVICE_EVENT_REMOVING] ==
			1U,
		"removing event count mismatch");
	LPF_DUMMY_SELFTEST_EXPECT(
		g_lpf_dummy_selftest_state.events[LPF_DEVICE_EVENT_REMOVED] ==
			1U,
		"removed event count mismatch");

	ret = lpf_device_get_info_by_name(LPF_DUMMY_SELFTEST_NAME, &info);
	LPF_DUMMY_SELFTEST_EXPECT(ret == OSAL_ERR_NAME_NOT_FOUND,
				  "dummy device still discoverable");

	lpf_device_event_unsubscribe(lpf_dummy_selftest_event,
				     &g_lpf_dummy_selftest_state);
	return OSAL_SUCCESS;

out_unregister:
	lpf_driver_unregister(&g_lpf_dummy_selftest_driver);
out_unsubscribe:
	lpf_device_event_unsubscribe(lpf_dummy_selftest_event,
				     &g_lpf_dummy_selftest_state);
	return ret;
}

static int __init lpf_dummy_selftest_init(void)
{
	int32_t ret;

	ret = lpf_dummy_selftest_run();
	if (ret != OSAL_SUCCESS)
		return (int)lpf_status_to_errno(ret);

	pr_info("LPF:DUMMY_SELFTEST: dummy service lifecycle checks passed\n");
	return 0;
}

static void __exit lpf_dummy_selftest_exit(void)
{
	pr_info("LPF:DUMMY_SELFTEST: unloaded\n");
}

module_init(lpf_dummy_selftest_init);
module_exit(lpf_dummy_selftest_exit);

MODULE_AUTHOR("LPF");
MODULE_DESCRIPTION("LPF dummy peripheral service self-test");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: lpf_core");
