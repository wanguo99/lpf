/************************************************************************
 * HAL层 - GPIO硬件抽象接口 (Linux实现)
 *
 * 使用Linux sysfs GPIO接口实现
 ************************************************************************/

#include "hal/hal_gpio_api.h"
#include "hal_gpio_internal.h"
#include "osal.h"
#include "osal_flock.h"
#include "sys/osal_poll_internal.h"
#include "sys/osal_file_internal.h"

/*===========================================================================
 * GPIO中断监听线程管理
 *===========================================================================*/

#define MAX_GPIO_PINS 256

typedef struct {
    uint32_t gpio_num;
    int value_fd;
    hal_gpio_isr_callback_t callback;
    void *user_data;
    osal_thread_t thread;
    bool enabled;
    bool running;
} gpio_isr_context_t;

static gpio_isr_context_t gpio_isr_table[MAX_GPIO_PINS];
static osal_mutex_t *gpio_isr_mutex = NULL;
static osal_flock_t *g_gpio_flock = NULL;

/*===========================================================================
 * 模块初始化和清理
 *===========================================================================*/

/**
 * @brief GPIO模块初始化
 */
static int32_t gpio_module_init(void)
{
    int32_t ret;

    if (gpio_isr_mutex != NULL) {
        return OSAL_SUCCESS;  /* 已初始化 */
    }

    /* 创建文件锁 */
    ret = OSAL_FlockCreate(HAL_GPIO_LOCK_PATH, &g_gpio_flock);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    ret = OSAL_MutexCreate(&gpio_isr_mutex);
    if (ret != OSAL_SUCCESS) {
        OSAL_FlockDestroy(g_gpio_flock);
        g_gpio_flock = NULL;
        return ret;
    }

    return OSAL_SUCCESS;
}

/**
 * @brief GPIO模块清理
 */
static void gpio_module_cleanup(void)
{
    if (gpio_isr_mutex != NULL) {
        OSAL_MutexDelete(gpio_isr_mutex);
        gpio_isr_mutex = NULL;
    }

    if (g_gpio_flock != NULL) {
        OSAL_FlockDestroy(g_gpio_flock);
        g_gpio_flock = NULL;
    }
}

/*===========================================================================
 * 内部辅助函数
 *===========================================================================*/

/**
 * @brief 写入字符串到文件
 */
static int32_t gpio_write_file(const char *path, const char *value)
{
    int32_t fd;
    uint32_t len;
    int32_t written;

    fd = OSAL_open(path, OSAL_O_WRONLY, 0);
    if (fd < 0) {
        int32_t err = OSAL_GetErrno();
        return err;
    }

    len = (uint32_t)OSAL_Strlen(value);
    written = OSAL_write(fd, value, len);
    OSAL_close(fd);

    if (written != (int32_t)len) {
        int32_t err = OSAL_GetErrno();
        return err;
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 从文件读取字符串
 */
static int32_t gpio_read_file(const char *path, char *buffer, size_t size)
{
    int32_t fd;
    int32_t len;

    fd = OSAL_open(path, OSAL_O_RDONLY, 0);
    if (fd < 0) {
        int32_t err = OSAL_GetErrno();
        return err;
    }

    len = OSAL_read(fd, buffer, (uint32_t)(size - 1));
    OSAL_close(fd);

    if (len < 0) {
        int32_t err = OSAL_GetErrno();
        return err;
    }

    buffer[len] = '\0';
    return OSAL_SUCCESS;
}

/**
 * @brief 导出GPIO引脚
 */
static int32_t gpio_export(uint32_t gpio_num)
{
    char gpio_str[16];
    char path[256];

    /* 检查GPIO是否已导出 */
    OSAL_Snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u", gpio_num);
    if (OSAL_access(path, OSAL_F_OK) == 0) {
        return OSAL_SUCCESS;  /* 已导出 */
    }

    /* 导出GPIO */
    OSAL_Snprintf(gpio_str, sizeof(gpio_str), "%u", gpio_num);
    return gpio_write_file("/sys/class/gpio/export", gpio_str);
}

/**
 * @brief 取消导出GPIO引脚
 */
static int32_t gpio_unexport(uint32_t gpio_num)
{
    char gpio_str[16];
    OSAL_Snprintf(gpio_str, sizeof(gpio_str), "%u", gpio_num);
    return gpio_write_file("/sys/class/gpio/unexport", gpio_str);
}

/**
 * @brief GPIO中断监听线程
 */
static void* gpio_isr_thread(void *arg)
{
    gpio_isr_context_t *ctx = (gpio_isr_context_t *)arg;
    osal_pollfd_t pfd;
    char value_str[4];
    hal_gpio_level_t level;
    int32_t bytes_read;
    int ret;
    int32_t len;

    pfd.fd = ctx->value_fd;
    pfd.events = OSAL_POLLPRI | OSAL_POLLERR;

    /* 清除初始中断 */
    OSAL_lseek(ctx->value_fd, 0, OSAL_SEEK_SET);
    bytes_read = OSAL_read(ctx->value_fd, value_str, sizeof(value_str));
    (void)bytes_read;  /* Suppress unused result warning */

    while (ctx->running) {
        ret = OSAL_poll(&pfd, 1, 1000);  /* 1秒超时 */

        if (ret > 0 && (pfd.revents & OSAL_POLLPRI)) {
            if (!ctx->enabled) {
                /* 清除中断但不调用回调 */
                OSAL_lseek(ctx->value_fd, 0, OSAL_SEEK_SET);
                bytes_read = OSAL_read(ctx->value_fd, value_str, sizeof(value_str));
                (void)bytes_read;  /* Suppress unused result warning */
                continue;
            }

            /* 读取当前电平 */
            OSAL_lseek(ctx->value_fd, 0, OSAL_SEEK_SET);
            len = OSAL_read(ctx->value_fd, value_str, sizeof(value_str));
            if (len > 0) {
                value_str[len] = '\0';
                level = (value_str[0] == '1') ? HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW;

                /* 调用回调函数 */
                if (ctx->callback) {
                    ctx->callback(ctx->gpio_num, level, ctx->user_data);
                }
            }
        }
    }

    return NULL;
}

/*===========================================================================
 * GPIO API实现
 *===========================================================================*/

int32_t HAL_GPIO_Init(uint32_t gpio_num, const hal_gpio_config_t *config)
{
    int32_t ret;

    if (!config || gpio_num >= MAX_GPIO_PINS) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 初始化GPIO模块 */
    ret = gpio_module_init();
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 获取文件锁 */
    ret = OSAL_FlockLock(g_gpio_flock, OSAL_FLOCK_EXCLUSIVE);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 获取互斥锁 */
    OSAL_MutexLock(gpio_isr_mutex);

    /* 导出GPIO */
    ret = gpio_export(gpio_num);
    if (ret != OSAL_SUCCESS) {
        OSAL_MutexUnlock(gpio_isr_mutex);
        OSAL_FlockUnlock(g_gpio_flock);
        return ret;
    }

    /* 等待sysfs文件创建 */
    OSAL_msleep(100);  /* 100ms */

    /* 设置方向 */
    ret = HAL_GPIO_SetDirection(gpio_num, config->direction);
    if (ret != OSAL_SUCCESS) {
        gpio_unexport(gpio_num);
        OSAL_MutexUnlock(gpio_isr_mutex);
        OSAL_FlockUnlock(g_gpio_flock);
        return ret;
    }

    /* 如果是输出模式，设置初始电平 */
    if (config->direction == HAL_GPIO_DIR_OUTPUT) {
        ret = HAL_GPIO_SetLevel(gpio_num, config->initial_level);
        if (ret != OSAL_SUCCESS) {
            gpio_unexport(gpio_num);
            OSAL_MutexUnlock(gpio_isr_mutex);
            OSAL_FlockUnlock(g_gpio_flock);
            return ret;
        }
    }

    /* 配置中断 */
    if (config->edge != HAL_GPIO_EDGE_NONE && config->callback) {
        ret = HAL_GPIO_SetInterrupt(gpio_num, config->edge, config->callback, config->user_data);
        if (ret != OSAL_SUCCESS) {
            gpio_unexport(gpio_num);
            OSAL_MutexUnlock(gpio_isr_mutex);
            OSAL_FlockUnlock(g_gpio_flock);
            return ret;
        }
    }

    OSAL_MutexUnlock(gpio_isr_mutex);
    OSAL_FlockUnlock(g_gpio_flock);

    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_Deinit(uint32_t gpio_num)
{
    int32_t ret;

    if (gpio_num >= MAX_GPIO_PINS) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 获取文件锁 */
    ret = OSAL_FlockLock(g_gpio_flock, OSAL_FLOCK_EXCLUSIVE);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 停止中断监听线程 */
    OSAL_MutexLock(gpio_isr_mutex);
    if (gpio_isr_table[gpio_num].running) {
        gpio_isr_table[gpio_num].running = false;
        OSAL_MutexUnlock(gpio_isr_mutex);

        OSAL_ThreadJoin(gpio_isr_table[gpio_num].thread);

        OSAL_MutexLock(gpio_isr_mutex);
        if (gpio_isr_table[gpio_num].value_fd >= 0) {
            OSAL_close(gpio_isr_table[gpio_num].value_fd);
            gpio_isr_table[gpio_num].value_fd = -1;
        }
        OSAL_Memset(&gpio_isr_table[gpio_num], 0, sizeof(gpio_isr_context_t));
    }
    OSAL_MutexUnlock(gpio_isr_mutex);

    /* 清理GPIO模块 */
    gpio_module_cleanup();

    /* 取消导出GPIO */
    ret = gpio_unexport(gpio_num);

    OSAL_FlockUnlock(g_gpio_flock);

    return ret;
}

int32_t HAL_GPIO_SetDirection(uint32_t gpio_num, hal_gpio_direction_t direction)
{
    char path[256];
    const char *dir_str;
    int32_t ret;

    if (gpio_num >= MAX_GPIO_PINS) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 获取文件锁 */
    ret = OSAL_FlockLock(g_gpio_flock, OSAL_FLOCK_EXCLUSIVE);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 获取互斥锁 */
    OSAL_MutexLock(gpio_isr_mutex);

    dir_str = (direction == HAL_GPIO_DIR_INPUT) ? "in" : "out";
    OSAL_Snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/direction", gpio_num);

    ret = gpio_write_file(path, dir_str);

    OSAL_MutexUnlock(gpio_isr_mutex);
    OSAL_FlockUnlock(g_gpio_flock);

    return ret;
}

int32_t HAL_GPIO_GetDirection(uint32_t gpio_num, hal_gpio_direction_t *direction)
{
    char path[256];
    char buffer[16];
    int32_t ret;

    if (gpio_num >= MAX_GPIO_PINS || !direction) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 获取文件锁 */
    ret = OSAL_FlockLock(g_gpio_flock, OSAL_FLOCK_EXCLUSIVE);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 获取互斥锁 */
    OSAL_MutexLock(gpio_isr_mutex);

    OSAL_Snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/direction", gpio_num);
    ret = gpio_read_file(path, buffer, sizeof(buffer));
    if (ret != OSAL_SUCCESS) {
        OSAL_MutexUnlock(gpio_isr_mutex);
        OSAL_FlockUnlock(g_gpio_flock);
        return ret;
    }

    *direction = (OSAL_Strncmp(buffer, "in", 2) == 0) ? HAL_GPIO_DIR_INPUT : HAL_GPIO_DIR_OUTPUT;

    OSAL_MutexUnlock(gpio_isr_mutex);
    OSAL_FlockUnlock(g_gpio_flock);

    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_SetLevel(uint32_t gpio_num, hal_gpio_level_t level)
{
    char path[256];
    const char *level_str;
    int32_t ret;

    if (gpio_num >= MAX_GPIO_PINS) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 获取文件锁 */
    ret = OSAL_FlockLock(g_gpio_flock, OSAL_FLOCK_EXCLUSIVE);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 获取互斥锁 */
    OSAL_MutexLock(gpio_isr_mutex);

    level_str = (level == HAL_GPIO_LEVEL_HIGH) ? "1" : "0";
    OSAL_Snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/value", gpio_num);

    ret = gpio_write_file(path, level_str);

    OSAL_MutexUnlock(gpio_isr_mutex);
    OSAL_FlockUnlock(g_gpio_flock);

    return ret;
}

int32_t HAL_GPIO_GetLevel(uint32_t gpio_num, hal_gpio_level_t *level)
{
    char path[256];
    char buffer[4];
    int32_t ret;

    if (gpio_num >= MAX_GPIO_PINS || !level) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 获取文件锁 */
    ret = OSAL_FlockLock(g_gpio_flock, OSAL_FLOCK_EXCLUSIVE);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 获取互斥锁 */
    OSAL_MutexLock(gpio_isr_mutex);

    OSAL_Snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/value", gpio_num);
    ret = gpio_read_file(path, buffer, sizeof(buffer));
    if (ret != OSAL_SUCCESS) {
        OSAL_MutexUnlock(gpio_isr_mutex);
        OSAL_FlockUnlock(g_gpio_flock);
        return ret;
    }

    *level = (buffer[0] == '1') ? HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW;

    OSAL_MutexUnlock(gpio_isr_mutex);
    OSAL_FlockUnlock(g_gpio_flock);

    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_SetInterrupt(uint32_t gpio_num, hal_gpio_edge_t edge,
                               hal_gpio_isr_callback_t callback, void *user_data)
{
    char path[256];
    const char *edge_str;
    int32_t ret;
    int32_t fd;

    if (gpio_num >= MAX_GPIO_PINS || !callback) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 获取文件锁 */
    ret = OSAL_FlockLock(g_gpio_flock, OSAL_FLOCK_EXCLUSIVE);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 设置触发模式 */
    switch (edge) {
        case HAL_GPIO_EDGE_NONE:    edge_str = "none"; break;
        case HAL_GPIO_EDGE_RISING:  edge_str = "rising"; break;
        case HAL_GPIO_EDGE_FALLING: edge_str = "falling"; break;
        case HAL_GPIO_EDGE_BOTH:    edge_str = "both"; break;
        default:
            OSAL_FlockUnlock(g_gpio_flock);
            return OSAL_ERR_INVALID_PARAM;
    }

    OSAL_Snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/edge", gpio_num);
    ret = gpio_write_file(path, edge_str);
    if (ret != OSAL_SUCCESS) {
        OSAL_FlockUnlock(g_gpio_flock);
        return ret;
    }

    if (edge == HAL_GPIO_EDGE_NONE) {
        OSAL_FlockUnlock(g_gpio_flock);
        return OSAL_SUCCESS;
    }

    /* 打开value文件用于poll */
    OSAL_Snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/value", gpio_num);
    fd = OSAL_open(path, OSAL_O_RDONLY, 0);
    if (fd < 0) {
        int32_t err = OSAL_GetErrno();
        OSAL_FlockUnlock(g_gpio_flock);
        return err;
    }

    /* 初始化中断上下文 */
    OSAL_MutexLock(gpio_isr_mutex);

    /* 如果已有线程在运行，先停止 */
    if (gpio_isr_table[gpio_num].running) {
        gpio_isr_table[gpio_num].running = false;
        OSAL_MutexUnlock(gpio_isr_mutex);
        OSAL_ThreadJoin(gpio_isr_table[gpio_num].thread);
        OSAL_MutexLock(gpio_isr_mutex);
        if (gpio_isr_table[gpio_num].value_fd >= 0) {
            OSAL_close(gpio_isr_table[gpio_num].value_fd);
        }
    }

    gpio_isr_table[gpio_num].gpio_num = gpio_num;
    gpio_isr_table[gpio_num].value_fd = fd;
    gpio_isr_table[gpio_num].callback = callback;
    gpio_isr_table[gpio_num].user_data = user_data;
    gpio_isr_table[gpio_num].enabled = true;
    gpio_isr_table[gpio_num].running = true;

    /* 创建监听线程 */
    ret = OSAL_ThreadCreate(&gpio_isr_table[gpio_num].thread,
                            gpio_isr_thread, &gpio_isr_table[gpio_num]);

    OSAL_MutexUnlock(gpio_isr_mutex);
    OSAL_FlockUnlock(g_gpio_flock);

    if (ret != OSAL_SUCCESS) {
        OSAL_close(fd);
        OSAL_Memset(&gpio_isr_table[gpio_num], 0, sizeof(gpio_isr_context_t));
        return ret;
    }

    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_EnableInterrupt(uint32_t gpio_num)
{
    int32_t ret;

    if (gpio_num >= MAX_GPIO_PINS) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 获取文件锁 */
    ret = OSAL_FlockLock(g_gpio_flock, OSAL_FLOCK_EXCLUSIVE);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    OSAL_MutexLock(gpio_isr_mutex);
    if (!gpio_isr_table[gpio_num].running) {
        OSAL_MutexUnlock(gpio_isr_mutex);
        OSAL_FlockUnlock(g_gpio_flock);
        return OSAL_ERR_INVALID_PARAM;
    }
    gpio_isr_table[gpio_num].enabled = true;
    OSAL_MutexUnlock(gpio_isr_mutex);

    OSAL_FlockUnlock(g_gpio_flock);

    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_DisableInterrupt(uint32_t gpio_num)
{
    int32_t ret;

    if (gpio_num >= MAX_GPIO_PINS) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 获取文件锁 */
    ret = OSAL_FlockLock(g_gpio_flock, OSAL_FLOCK_EXCLUSIVE);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    OSAL_MutexLock(gpio_isr_mutex);
    if (!gpio_isr_table[gpio_num].running) {
        OSAL_MutexUnlock(gpio_isr_mutex);
        OSAL_FlockUnlock(g_gpio_flock);
        return OSAL_ERR_INVALID_PARAM;
    }
    gpio_isr_table[gpio_num].enabled = false;
    OSAL_MutexUnlock(gpio_isr_mutex);

    OSAL_FlockUnlock(g_gpio_flock);

    return OSAL_SUCCESS;
}
