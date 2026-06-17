/**
 * @file main.c
 * @brief 测试应用主入口
 */

#include "osal.h"
#include "pmc_runtime.h"
#include "test_call_chain.h"

int main(int argc, char *argv[])
{
	int32_t ret;

	(void)argc;
	(void)argv;

	OSAL_printf("=== Test Call Chain Application ===\n");

	ret = PMC_Runtime_Init();
	if (OSAL_SUCCESS != ret) {
		OSAL_printf("Failed to initialize PMC runtime: %d\n", ret);
		return -1;
	}

	OSAL_printf("\nStarting call chain tests...\n\n");

	ret = TestCallChain_RunAll();

	PMC_Runtime_Cleanup();

	if (OSAL_SUCCESS == ret) {
		OSAL_printf("\nAll tests completed successfully!\n");
		return 0;
	}

	OSAL_printf("\nSome tests failed!\n");
	return -1;
}
