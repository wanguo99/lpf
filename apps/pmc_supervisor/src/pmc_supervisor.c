#include "pmc_supervisor.h"
#include "libpmc_ipc.h"
#include <signal.h>
#include <sys/wait.h>

/* 全局变量 */
static pmc_process_heartbeat_t *g_heartbeat = NULL;
static volatile bool g_running = true;

/* 进程信息 */
typedef struct {
    const char *name;
    const char *path;
    pmc_process_id_t id;
    pid_t pid;
    uint32 restart_count;
} process_info_t;

static process_info_t g_processes[] = {
    {"pmc_comm",      "./pmc_comm",      PMC_PROCESS_COMM,      0, 0},
    {"pmc_collector", "./pmc_collector", PMC_PROCESS_COLLECTOR, 0, 0},
    {"pmc_health",    "./pmc_health",    PMC_PROCESS_HEALTH,    0, 0},
    {"pmc_logger",    "./pmc_logger",    PMC_PROCESS_LOGGER,    0, 0},
};

#define PROCESS_COUNT (sizeof(g_processes) / sizeof(g_processes[0]))

/* 信号处理 */
static void signal_handler(int sig)
{
    if (sig == SIGTERM || sig == SIGINT) {
        g_running = false;
        LOG_INFO("SUPERVISOR", "收到退出信号");
    } else if (sig == SIGCHLD) {
        /* 子进程退出 */
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            LOG_WARN("SUPERVISOR", "子进程退出: pid=%d, status=%d", pid, status);
        }
    }
}

/* 启动进程 */
static int32 start_process(process_info_t *proc)
{
    pid_t pid = fork();

    if (pid < 0) {
        LOG_ERROR("SUPERVISOR", "fork失败: %s", proc->name);
        return OS_ERROR;
    } else if (pid == 0) {
        /* 子进程 */
        execl(proc->path, proc->name, NULL);
        LOG_ERROR("SUPERVISOR", "execl失败: %s", proc->name);
        exit(1);
    } else {
        /* 父进程 */
        proc->pid = pid;
        LOG_INFO("SUPERVISOR", "启动进程: %s (pid=%d)", proc->name, pid);
        return OS_SUCCESS;
    }
}

/* 停止进程 */
static int32 stop_process(process_info_t *proc)
{
    if (proc->pid <= 0) {
        return OS_SUCCESS;
    }

    LOG_INFO("SUPERVISOR", "停止进程: %s (pid=%d)", proc->name, proc->pid);

    /* 发送SIGTERM */
    kill(proc->pid, SIGTERM);

    /* 等待进程退出（最多5秒） */
    for (int32 i = 0; i < 50; i++) {
        int status;
        pid_t ret = waitpid(proc->pid, &status, WNOHANG);
        if (ret == proc->pid) {
            LOG_INFO("SUPERVISOR", "进程已退出: %s", proc->name);
            proc->pid = 0;
            return OS_SUCCESS;
        }
        OSAL_TaskDelay(100);
    }

    /* 强制杀死 */
    LOG_WARN("SUPERVISOR", "强制杀死进程: %s", proc->name);
    kill(proc->pid, SIGKILL);
    waitpid(proc->pid, NULL, 0);
    proc->pid = 0;

    return OS_SUCCESS;
}

/* 重启进程 */
static int32 restart_process(process_info_t *proc)
{
    LOG_WARN("SUPERVISOR", "重启进程: %s (重启次数=%u)", proc->name, proc->restart_count);

    /* 停止进程 */
    stop_process(proc);

    /* 等待1秒 */
    OSAL_TaskDelay(1000);

    /* 启动进程 */
    int32 ret = start_process(proc);
    if (ret == OS_SUCCESS) {
        proc->restart_count++;
    }

    return ret;
}

/* 检查进程心跳 */
static void check_process_heartbeat(void)
{
    for (uint32 i = 0; i < PROCESS_COUNT; i++) {
        process_info_t *proc = &g_processes[i];

        if (proc->pid <= 0) {
            continue;
        }

        /* 检查心跳 */
        bool alive;
        int32 ret = PMC_Heartbeat_Check(g_heartbeat, proc->id, 2000, &alive);
        if (ret == OS_SUCCESS && !alive) {
            LOG_ERROR("SUPERVISOR", "进程心跳超时: %s", proc->name);
            restart_process(proc);
        }
    }
}

/* 初始化 */
int32 PMC_Supervisor_Init(void)
{
    int32 ret;

    LOG_INFO("SUPERVISOR", "Supervisor进程初始化...");

    /* 注册信号处理 */
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGCHLD, signal_handler);

    /* 初始化心跳 */
    ret = PMC_Heartbeat_Init(&g_heartbeat);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("SUPERVISOR", "初始化心跳失败: %d", ret);
        return ret;
    }

    LOG_INFO("SUPERVISOR", "Supervisor进程初始化完成");
    return OS_SUCCESS;
}

/* 主循环 */
int32 PMC_Supervisor_Run(void)
{
    LOG_INFO("SUPERVISOR", "Supervisor进程开始运行");

    /* 启动所有子进程 */
    for (uint32 i = 0; i < PROCESS_COUNT; i++) {
        start_process(&g_processes[i]);
        OSAL_TaskDelay(500);  /* 间隔500ms启动 */
    }

    /* 监控循环 */
    while (g_running) {
        /* 更新自己的心跳 */
        PMC_Heartbeat_Update(g_heartbeat, PMC_PROCESS_SUPERVISOR);

        /* 检查子进程心跳 */
        check_process_heartbeat();

        /* 休眠500ms */
        OSAL_TaskDelay(500);
    }

    /* 停止所有子进程 */
    LOG_INFO("SUPERVISOR", "停止所有子进程...");
    for (uint32 i = 0; i < PROCESS_COUNT; i++) {
        stop_process(&g_processes[i]);
    }

    LOG_INFO("SUPERVISOR", "Supervisor进程退出");
    return OS_SUCCESS;
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
