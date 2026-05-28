#include "ccm_supervisor.h"
#include "libccm/libccm_ipc.h"
#include "osal/sys/osal_signal.h"
#include "osal/sys/osal_process.h"

/* 全局变量 */
static pmc_process_heartbeat_t *g_heartbeat = NULL;
static volatile bool g_running = true;

/* 进程信息 */
typedef struct {
    const char *name;
    const char *path;
    pmc_process_id_t id;
    osal_id_t pid;
    uint32_t restart_count;
} process_info_t;

static process_info_t g_processes[] = {
    {"ccm_comm",      "./ccm_comm",      PMC_PROCESS_COMM,      0, 0},
    {"ccm_collector", "./ccm_collector", PMC_PROCESS_COLLECTOR, 0, 0},
    {"ccm_health",    "./ccm_health",    PMC_PROCESS_HEALTH,    0, 0},
    {"ccm_logger",    "./ccm_logger",    PMC_PROCESS_LOGGER,    0, 0},
};

#define PROCESS_COUNT (sizeof(g_processes) / sizeof(g_processes[0]))

/* 信号处理 */
static void signal_handler(int32_t sig)
{
    if (sig == SIGTERM || sig == SIGINT) {
        g_running = false;
        LOG_INFO("SUPERVISOR", "收到退出信号");
    }
    /* SIGCHLD处理移除，改用轮询检查子进程状态 */
}

/* 启动进程 */
static int32_t start_process(process_info_t *proc)
{
    char *argv[] = {(char *)proc->name, NULL};
    int32_t ret = OSAL_ProcessCreate(&proc->pid, proc->path, argv, NULL);

    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("SUPERVISOR", "启动进程失败: %s, ret=%d", proc->name, ret);
        return ret;
    }

    LOG_INFO("SUPERVISOR", "启动进程: %s (pid=%u)", proc->name, (uint32_t)proc->pid);
    return OSAL_SUCCESS;
}

/* 停止进程 */
static int32_t stop_process(process_info_t *proc)
{
    if (proc->pid == 0) {
        return OSAL_SUCCESS;
    }

    LOG_INFO("SUPERVISOR", "停止进程: %s (pid=%u)", proc->name, (uint32_t)proc->pid);

    /* 发送SIGTERM */
    OSAL_ProcessKill(proc->pid, SIGTERM);

    /* 等待进程退出（最多5秒） */
    {
        int32_t i;
        for (i = 0; i < 50; i++) {
            int32_t status;
            int32_t ret = OSAL_ProcessWait(proc->pid, &status, 0);
            if (ret == OSAL_SUCCESS) {
                LOG_INFO("SUPERVISOR", "进程已退出: %s", proc->name);
                proc->pid = 0;
                return OSAL_SUCCESS;
            }
            OSAL_msleep(100);
        }
    }

    /* 强制杀死 */
    LOG_WARN("SUPERVISOR", "强制杀死进程: %s", proc->name);
    OSAL_ProcessKill(proc->pid, SIGKILL);
    OSAL_ProcessWait(proc->pid, NULL, -1);  /* 阻塞等待 */
    proc->pid = 0;

    return OSAL_SUCCESS;
}

/* 重启进程 */
static int32_t restart_process(process_info_t *proc)
{
    int32_t ret;

    LOG_WARN("SUPERVISOR", "重启进程: %s (重启次数=%u)", proc->name, proc->restart_count);

    /* 停止进程 */
    stop_process(proc);

    /* 等待1秒 */
    OSAL_msleep(1000);

    /* 启动进程 */
    ret = start_process(proc);
    if (ret == OSAL_SUCCESS) {
        proc->restart_count++;
    }

    return ret;
}

/* 检查进程心跳 */
static void check_process_heartbeat(void)
{
    uint32_t i;
    for (i = 0; i < PROCESS_COUNT; i++) {
        process_info_t *proc = &g_processes[i];
        bool alive;
        int32_t ret;

        if (proc->pid <= 0) {
            continue;
        }

        /* 检查心跳 */
        ret = PMC_Heartbeat_Check(g_heartbeat, proc->id, 2000, &alive);
        if (ret == OSAL_SUCCESS && !alive) {
            LOG_ERROR("SUPERVISOR", "进程心跳超时: %s", proc->name);
            restart_process(proc);
        }
    }
}

/* 初始化 */
int32_t PMC_Supervisor_Init(void)
{
    int32_t ret;

    LOG_INFO("SUPERVISOR", "Supervisor进程初始化...");

    /* 注册信号处理 */
    OSAL_SignalRegister(SIGTERM, signal_handler);
    OSAL_SignalRegister(SIGINT, signal_handler);

    /* 初始化心跳 */
    ret = PMC_Heartbeat_Init(&g_heartbeat);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("SUPERVISOR", "初始化心跳失败: %d", ret);
        return ret;
    }

    LOG_INFO("SUPERVISOR", "Supervisor进程初始化完成");
    return OSAL_SUCCESS;
}

/* 主循环 */
int32_t PMC_Supervisor_Run(void)
{
    uint32_t i;

    LOG_INFO("SUPERVISOR", "Supervisor进程开始运行");

    /* 启动所有子进程 */
    for (i = 0; i < PROCESS_COUNT; i++) {
        start_process(&g_processes[i]);
        OSAL_msleep(500);  /* 间隔500ms启动 */
    }

    /* 监控循环 */
    while (g_running) {
        /* 更新自己的心跳 */
        PMC_Heartbeat_Update(g_heartbeat, PMC_PROCESS_SUPERVISOR);

        /* 检查子进程心跳 */
        check_process_heartbeat();

        /* 休眠500ms */
        OSAL_msleep(500);
    }

    /* 停止所有子进程 */
    LOG_INFO("SUPERVISOR", "停止所有子进程...");
    for (i = 0; i < PROCESS_COUNT; i++) {
        stop_process(&g_processes[i]);
    }

    LOG_INFO("SUPERVISOR", "Supervisor进程退出");
    return OSAL_SUCCESS;
}

/* 清理 */
void PMC_Supervisor_Cleanup(void)
{
    LOG_INFO("SUPERVISOR", "Supervisor进程清理...");

    if (g_heartbeat) {
        PMC_Heartbeat_Cleanup(g_heartbeat);
        g_heartbeat = NULL;
    }

    LOG_INFO("SUPERVISOR", "Supervisor进程清理完成");
}
