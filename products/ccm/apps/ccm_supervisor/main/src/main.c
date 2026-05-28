#include "ccm_supervisor.h"

int main(int argc, char *argv[])
{
    int32_t ret;

    (void)argc;
    (void)argv;

    LOG_INFO("MAIN", "PMC Supervisor进程启动");

    /* 初始化 */
    ret = PMC_Supervisor_Init();
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("MAIN", "初始化失败: %d", ret);
        return 1;
    }

    /* 运行主循环 */
    ret = PMC_Supervisor_Run();

    /* 清理 */
    PMC_Supervisor_Cleanup();

    LOG_INFO("MAIN", "PMC Supervisor进程退出: %d", ret);
    return (ret == OSAL_SUCCESS) ? 0 : 1;
}
