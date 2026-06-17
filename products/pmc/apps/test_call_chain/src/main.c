/**
 * @file main.c
 * @brief 测试应用主入口
 */

#include "osal.h"
#include "pconfig.h"
#include "aconfig.h"
#include "test_call_chain.h"

/* 外部平台配置 */
extern const pconfig_platform_config_t pconfig_h200_100p_am625;

int main(int argc, char *argv[])
{
	int32_t ret;

	(void)argc;
	(void)argv;

	OSAL_printf("=== Test Call Chain Application ===\n");

	/* 初始化 PCONFIG */
	ret = PCONFIG_init();
	if (OSAL_SUCCESS != ret) {
		OSAL_printf("Failed to initialize PCONFIG: %d\n", ret);
		return -1;
	}

	/* 注册平台配置 */
	ret = PCONFIG_register(&pconfig_h200_100p_am625);
	if (OSAL_SUCCESS != ret) {
		OSAL_printf("Failed to register platform config: %d\n", ret);
		return -1;
	}

	OSAL_printf("Platform config registered: %s\n", pconfig_h200_100p_am625.platform_name);

	/* 初始化 ACONFIG */
	ret = ACONFIG_init();
	if (OSAL_SUCCESS != ret) {
		OSAL_printf("Failed to initialize ACONFIG: %d\n", ret);
		return -1;
	}

	/* 运行测试 */
	OSAL_printf("\nStarting call chain tests...\n\n");

	ret = TestCallChain_RunAll();

	/* 清理 */
	ACONFIG_cleanup();
	PCONFIG_cleanup();

	if (OSAL_SUCCESS == ret) {
		OSAL_printf("\nAll tests completed successfully!\n");
		return 0;
	} else {
		OSAL_printf("\nSome tests failed!\n");
		return -1;
	}
}
