#include "pmc_health.h"
#include "pmc_runtime.h"

int main(int argc, char *argv[])
{
    int32_t ret;

    (void)argc;
    (void)argv;

    LOG_INFO("MAIN", "PMC Health进程启动");

    ret = PMC_Runtime_Init();
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("MAIN", "产品运行时初始化失败: %d", ret);
        return 1;
    }

    ret = PMC_Health_Init();
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("MAIN", "初始化失败: %d", ret);
        PMC_Runtime_Cleanup();
        return 1;
    }

    ret = PMC_Health_Run();

    PMC_Health_Cleanup();
    PMC_Runtime_Cleanup();

    LOG_INFO("MAIN", "PMC Health进程退出: %d", ret);
    return (ret == OSAL_SUCCESS) ? 0 : 1;
}
