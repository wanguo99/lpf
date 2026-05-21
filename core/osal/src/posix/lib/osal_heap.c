/************************************************************************
 * OSAL POSIX实现 - 内存管理
 ************************************************************************/

#include "osal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* 内存块头部，用于追踪分配大小 */
typedef struct {
    uint32_t size;
    uint32_t magic;
} mem_block_header_t;

#define MEM_BLOCK_MAGIC 0xDEADBEEF

typedef struct {
    uint32_t threshold_percent;
    uint32_t peak_usage;
    uint32_t current_usage;
    bool alert_triggered;
    pthread_mutex_t lock;
} heap_monitor_t;

static heap_monitor_t g_heap_monitor = {
    .threshold_percent = OSAL_HEAP_THRESHOLD_DEFAULT,
    .peak_usage = 0,
    .current_usage = 0,
    .alert_triggered = false,
};

void OS_HeapInit(void)
{
    pthread_mutex_init(&g_heap_monitor.lock, NULL);
}

static uint32_t read_memory_from_proc(const char *field)
{
    FILE *fp;
    char line[OSAL_HEAP_LINE_BUFFER_SIZE];
    uint32_t value;

    fp = fopen("/proc/self/status", "r");
    if (NULL == fp)
        return 0;

    value = 0;
    while (NULL != fgets(line, sizeof(line), fp)) {
        if (0 == strncmp(line, field, strlen(field))) {
            sscanf(line, "%*s %u", &value);
            break;
        }
    }
    fclose(fp);
    return value * OSAL_BYTES_PER_KB;
}

int32_t OSAL_HeapGetInfo(uint32_t *free_bytes, uint32_t *total_bytes)
{
    uint32_t vm_rss;
    uint32_t vm_peak;

    if (NULL == free_bytes || NULL == total_bytes)
        return OSAL_ERR_INVALID_POINTER;

    vm_rss = read_memory_from_proc("VmRSS:");
    vm_peak = read_memory_from_proc("VmPeak:");

    pthread_mutex_lock(&g_heap_monitor.lock);

    /* 如果无法从/proc读取（如macOS），使用内部统计 */
    if (vm_rss == 0 && vm_peak == 0) {
        vm_rss = g_heap_monitor.current_usage;
        vm_peak = g_heap_monitor.peak_usage;
    } else {
        g_heap_monitor.current_usage = vm_rss;
        if (vm_rss > g_heap_monitor.peak_usage)
            g_heap_monitor.peak_usage = vm_rss;
    }

    pthread_mutex_unlock(&g_heap_monitor.lock);

    *free_bytes = (vm_peak > vm_rss) ? (vm_peak - vm_rss) : 0;
    *total_bytes = vm_peak;

    return OSAL_SUCCESS;
}

int32_t OSAL_HeapSetThreshold(uint32_t percent)
{
    if (percent > OSAL_HEAP_PERCENT_MAX)
        return OSAL_ERR_INVALID_SIZE;

    pthread_mutex_lock(&g_heap_monitor.lock);
    g_heap_monitor.threshold_percent = percent;
    pthread_mutex_unlock(&g_heap_monitor.lock);

    return OSAL_SUCCESS;
}

int32_t OSAL_HeapCheckThreshold(bool *exceeded)
{
    uint32_t free_bytes, total_bytes;
    uint32_t usage_percent;

    if (NULL == exceeded)
        return OSAL_ERR_INVALID_POINTER;

    OSAL_HeapGetInfo(&free_bytes, &total_bytes);

    if (0 == total_bytes) {
        *exceeded = false;
        return OSAL_SUCCESS;
    }

    usage_percent = ((total_bytes - free_bytes) * OSAL_HEAP_PERCENT_MULTIPLIER) / total_bytes;

    pthread_mutex_lock(&g_heap_monitor.lock);
    *exceeded = (usage_percent >= g_heap_monitor.threshold_percent);
    if (*exceeded && !g_heap_monitor.alert_triggered) {
        g_heap_monitor.alert_triggered = true;
        OSAL_Printf("[HEAP] Memory threshold exceeded: %u%% (threshold: %u%%)\n",
                  usage_percent, g_heap_monitor.threshold_percent);
    } else if (!*exceeded) {
        g_heap_monitor.alert_triggered = false;
    }
    pthread_mutex_unlock(&g_heap_monitor.lock);

    return OSAL_SUCCESS;
}

int32_t OSAL_HeapGetStats(uint32_t *current, uint32_t *peak)
{
    if (NULL == current || NULL == peak)
        return OSAL_ERR_INVALID_POINTER;

    pthread_mutex_lock(&g_heap_monitor.lock);
    *current = g_heap_monitor.current_usage;
    *peak = g_heap_monitor.peak_usage;
    pthread_mutex_unlock(&g_heap_monitor.lock);

    return OSAL_SUCCESS;
}

void *OSAL_Malloc(uint32_t size)
{
    size_t total_size;
    void *raw_ptr;
    union {
        void *raw;
        mem_block_header_t *header;
        uint8_t *bytes;
    } ptr_union;

    /* 检查是否会导致整数溢出（uint32_t 最大值是 4GB） */
    if (size > UINT32_MAX - sizeof(mem_block_header_t)) {
        LOG_ERROR("OSAL_Heap", "Allocation size too large: %u", size);
        return NULL;
    }

    /* 分配额外空间存储块头 */
    total_size = (size_t)size + sizeof(mem_block_header_t);
    raw_ptr = malloc(total_size);

    if (NULL == raw_ptr) {
        return NULL;
    }

    /* 使用联合体避免强制转换 */
    ptr_union.raw = raw_ptr;

    /* 填充块头信息 */
    ptr_union.header->size = size;
    ptr_union.header->magic = MEM_BLOCK_MAGIC;

    /* 更新统计信息 */
    pthread_mutex_lock(&g_heap_monitor.lock);
    g_heap_monitor.current_usage += size;
    if (g_heap_monitor.current_usage > g_heap_monitor.peak_usage) {
        g_heap_monitor.peak_usage = g_heap_monitor.current_usage;
    }
    pthread_mutex_unlock(&g_heap_monitor.lock);

    /* 返回用户数据区指针（跳过块头） */
    return ptr_union.header + 1;
}

void OSAL_Free(void *ptr)
{
    union {
        void *user_ptr;
        mem_block_header_t *header;
        uint8_t *bytes;
    } ptr_union;
    mem_block_header_t *header;

    if (NULL == ptr) {
        return;
    }

    /* 使用联合体避免强制转换 */
    ptr_union.user_ptr = ptr;
    /* 获取块头指针 */
    header = ptr_union.header - 1;

    /* 验证魔数，检测内存损坏 */
    if (MEM_BLOCK_MAGIC != header->magic) {
        OSAL_Printf("[HEAP] Memory corruption detected at %p (invalid magic: 0x%X)\n",
                    ptr, header->magic);
        return;
    }

    /* 更新统计信息 */
    pthread_mutex_lock(&g_heap_monitor.lock);
    if (header->size > g_heap_monitor.current_usage) {
        /* 统计下溢：记录详细错误信息 */
        OSAL_Printf("[HEAP] ERROR: Heap usage underflow - freeing %u bytes but current usage is %u\n",
                    header->size, g_heap_monitor.current_usage);
        g_heap_monitor.current_usage = 0;
    } else {
        g_heap_monitor.current_usage -= header->size;
    }
    pthread_mutex_unlock(&g_heap_monitor.lock);

    /* 清除魔数，防止double free */
    header->magic = 0;

    /* 释放内存（包括块头） */
    free(header);
}
