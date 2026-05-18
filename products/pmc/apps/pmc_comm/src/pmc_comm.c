#include "pmc_comm.h"
#include "libpmc_ipc.h"
#include "libpmc_protocol.h"
#include <signal.h>

/* 全局变量 */
static pmc_tm_cache_t *g_tm_cache = NULL;
static pmc_process_heartbeat_t *g_heartbeat = NULL;
static volatile bool g_running = true;

/* 信号处理 */
static void signal_handler(int sig)
{
    if (sig == SIGTERM || sig == SIGINT) {
        g_running = false;
        LOG_INFO("COMM", "收到退出信号");
    }
}

/* 初始化 */
int32 PMC_Comm_Init(void)
{
    int32 ret;

    LOG_INFO("COMM", "Communication进程初始化...");

    /* 注册信号处理 */
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    /* 初始化遥测缓存 */
    ret = PMC_TM_Cache_Init(&g_tm_cache);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("COMM", "初始化遥测缓存失败: %d", ret);
        return ret;
    }

    /* 初始化心跳 */
    ret = PMC_Heartbeat_Init(&g_heartbeat);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("COMM", "初始化心跳失败: %d", ret);
        PMC_TM_Cache_Cleanup(g_tm_cache);
        return ret;
    }

    /* 设置实时调度策略 */
    ret = OSAL_ThreadSetScheduler(OSAL_ThreadGetId(), OSAL_SCHED_FIFO, 99);
    if (ret != OS_SUCCESS) {
        LOG_WARN("COMM", "设置实时调度失败: %d (需要root权限)", ret);
    }

    /* 绑定到CPU0 */
    ret = OSAL_ThreadSetAffinity(OSAL_ThreadGetId(), 0);
    if (ret != OS_SUCCESS) {
        LOG_WARN("COMM", "绑定CPU0失败: %d", ret);
    }

    /* 锁定内存 */
    ret = OSAL_MemLockAll();
    if (ret != OS_SUCCESS) {
        LOG_WARN("COMM", "锁定内存失败: %d (需要root权限)", ret);
    }

    LOG_INFO("COMM", "Communication进程初始化完成");
    return OS_SUCCESS;
}

/* 处理遥测请求 */
static int32 handle_telemetry_request(const pmc_tm_request_t *req, pmc_tm_response_t *resp)
{
    uint8 data[PMC_TM_MAX_DATA_SIZE];
    uint32 size;
    pmc_tm_freshness_t freshness;

    /* 从缓存读取遥测数据 */
    int32 ret = PMC_TM_Cache_Read(g_tm_cache, req->tm_type, data, &size, &freshness);
    if (ret != OS_SUCCESS) {
        LOG_WARN("COMM", "读取遥测失败: tm_type=%d", req->tm_type);
        return ret;
    }

    /* 构造应答 */
    resp->tm_type = req->tm_type;
    OSAL_Memcpy(resp->data, data, size);
    resp->data_size = size;
    resp->freshness = (uint8)freshness;

    return OS_SUCCESS;
}

/* 处理遥控命令 */
static int32 handle_telecommand(const pmc_tc_frame_t *tc)
{
    LOG_INFO("COMM", "处理遥控命令: cmd_type=%d, param=%u", tc->cmd_type, tc->param);

    /* TODO: 实现遥控命令处理
     * 1. 根据命令类型调用PDL接口
     * 2. 更新系统状态
     * 3. 失效相关遥测
     */

    return OS_SUCCESS;
}

/* 主循环 */
int32 PMC_Comm_Run(void)
{
    LOG_INFO("COMM", "Communication进程开始运行");

    while (g_running) {
        /* 更新心跳 */
        PMC_Heartbeat_Update(g_heartbeat, PMC_PROCESS_COMM);

        /* TODO: 实现CAN接收和处理
         * 1. 从CAN总线接收帧
         * 2. 解析帧类型（遥控/遥测请求）
         * 3. 处理并应答
         */

        /* 模拟处理 */
        #if 0
        pmc_can_frame_t frame;
        /* 假设从CAN接收到帧 */

        if (frame.can_id == PMC_CAN_ID_TM_REQUEST) {
            /* 遥测请求 */
            pmc_tm_request_t req;
            pmc_tm_response_t resp;

            if (PMC_Protocol_ParseTM_Request(&frame, &req) == OS_SUCCESS) {
                if (handle_telemetry_request(&req, &resp) == OS_SUCCESS) {
                    pmc_can_frame_t resp_frame;
                    PMC_Protocol_BuildTM_Response(&resp, &resp_frame);
                    /* 发送应答到CAN */
                }
            }
        } else if (frame.can_id == PMC_CAN_ID_TC_CMD) {
            /* 遥控命令 */
            pmc_tc_frame_t tc;

            if (PMC_Protocol_ParseTC(&frame, &tc) == OS_SUCCESS) {
                handle_telecommand(&tc);
            }
        }
        #endif

        /* 暂时休眠，避免空转 */
        OSAL_TaskDelay(100);
    }

    LOG_INFO("COMM", "Communication进程退出");
    return OS_SUCCESS;
}

/* 清理 */
void PMC_Comm_Cleanup(void)
{
    LOG_INFO("COMM", "Communication进程清理...");

    if (g_tm_cache) {
        PMC_TM_Cache_Cleanup(g_tm_cache);
        g_tm_cache = NULL;
    }

    if (g_heartbeat) {
        PMC_Heartbeat_Cleanup(g_heartbeat);
        g_heartbeat = NULL;
    }

    LOG_INFO("COMM", "Communication进程清理完成");
}
