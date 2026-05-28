/************************************************************************
 * HAL层 - GPIO硬件抽象接口 (Linux实现)
 *
 * 使用Linux sysfs GPIO接口实现
 ************************************************************************/

#include "hal/hal_gpio.h"
#include "osal/osal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <errno.h>

/*===========================================================================
 * GPIO中断监听线程管理
 *===========================================================================*/

#define MAX_GPIO_PINS 256

typedef struct {
    uint32_t gpio_num;
    int value_fd;
    hal_gpio_isr_callback_t callback;
    void *user_data;
    pthread_t thread;
    bool enabled;
    bool running;
} gpio_isr_context_t;

static gpio_isr_context_t gpio_isr_table[MAX_GPIO_PINS];
static pthread_mutex_t gpio_isr_mutex = PTHREAD_MUTEX_INITIALIZER;

/*===========================================================================
 * 内部辅助函数
 *===========================================================================*/

/**
 * @brief 写入字符串到文件
 */
static int32_t gpio_write_file(const char *path, const char *value)
{
    int fd;
    ssize_t len;
    ssize_t written;

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        if (errno == EACCES) return OSAL_ERR_PERMISSION;
        if (errno == ENOENT) return OSAL_ENOENT;
        return OSAL_EIO;
    }

    len = strlen(value);
    written = write(fd, value, len);
    close(fd);

    if (written != len) {
        return OSAL_EIO;
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 从文件读取字符串
 */
static int32_t gpio_read_file(const char *path, char *buffer, size_t size)
{
    int fd;
    ssize_t len;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        if (errno == EACCES) return OSAL_ERR_PERMISSION;
        if (errno == ENOENT) return OSAL_ENOENT;
        return OSAL_EIO;
    }

    len = read(fd, buffer, size - 1);
    close(fd);

    if (len < 0) {
        return OSAL_EIO;
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
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u", gpio_num);
    if (access(path, F_OK) == 0) {
        return OSAL_SUCCESS;  /* 已导出 */
    }

    /* 导出GPIO */
    snprintf(gpio_str, sizeof(gpio_str), "%u", gpio_num);
    return gpio_write_file("/sys/class/gpio/export", gpio_str);
}

/**
 * @brief 取消导出GPIO引脚
 */
static int32_t gpio_unexport(uint32_t gpio_num)
{
    char gpio_str[16];
    snprintf(gpio_str, sizeof(gpio_str), "%u", gpio_num);
    return gpio_write_file("/sys/class/gpio/unexport", gpio_str);
}

/**
 * @brief GPIO中断监听线程
 */
static void* gpio_isr_thread(void *arg)
{
    gpio_isr_context_t *ctx = (gpio_isr_context_t *)arg;
    struct pollfd pfd;
    char value_str[4];
    hal_gpio_level_t level;
    ssize_t bytes_read;
    int ret;
    ssize_t len;

    pfd.fd = ctx->value_fd;
    pfd.events = POLLPRI | POLLERR;

    /* 清除初始中断 */
    lseek(ctx->value_fd, 0, SEEK_SET);
    bytes_read = read(ctx->value_fd, value_str, sizeof(value_str));
    (void)bytes_read;  /* Suppress unused result warning */

    while (ctx->running) {
        ret = poll(&pfd, 1, 1000);  /* 1秒超时 */

        if (ret > 0 && (pfd.revents & POLLPRI)) {
            if (!ctx->enabled) {
                /* 清除中断但不调用回调 */
                lseek(ctx->value_fd, 0, SEEK_SET);
                bytes_read = read(ctx->value_fd, value_str, sizeof(value_str));
                (void)bytes_read;  /* Suppress unused result warning */
                continue;
            }

            /* 读取当前电平 */
            lseek(ctx->value_fd, 0, SEEK_SET);
            len = read(ctx->value_fd, value_str, sizeof(value_str));
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
        return OSAL_EINVAL;
    }

    /* 导出GPIO */
    ret = gpio_export(gpio_num);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 等待sysfs文件创建 */
    OSAL_msleep(100);  /* 100ms */

    /* 设置方向 */
    ret = HAL_GPIO_SetDirection(gpio_num, config->direction);
    if (ret != OSAL_SUCCESS) {
        gpio_unexport(gpio_num);
        return ret;
    }

    /* 如果是输出模式，设置初始电平 */
    if (config->direction == HAL_GPIO_DIR_OUTPUT) {
        ret = HAL_GPIO_SetLevel(gpio_num, config->initial_level);
        if (ret != OSAL_SUCCESS) {
            gpio_unexport(gpio_num);
            return ret;
        }
    }

    /* 配置中断 */
    if (config->edge != HAL_GPIO_EDGE_NONE && config->callback) {
        ret = HAL_GPIO_SetInterrupt(gpio_num, config->edge, config->callback, config->user_data);
        if (ret != OSAL_SUCCESS) {
            gpio_unexport(gpio_num);
            return ret;
        }
    }

    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_Deinit(uint32_t gpio_num)
{
    if (gpio_num >= MAX_GPIO_PINS) {
        return OSAL_EINVAL;
    }

    /* 停止中断监听线程 */
    pthread_mutex_lock(&gpio_isr_mutex);
    if (gpio_isr_table[gpio_num].running) {
        gpio_isr_table[gpio_num].running = false;
        pthread_mutex_unlock(&gpio_isr_mutex);

        pthread_join(gpio_isr_table[gpio_num].thread, NULL);

        pthread_mutex_lock(&gpio_isr_mutex);
        if (gpio_isr_table[gpio_num].value_fd >= 0) {
            close(gpio_isr_table[gpio_num].value_fd);
            gpio_isr_table[gpio_num].value_fd = -1;
        }
        memset(&gpio_isr_table[gpio_num], 0, sizeof(gpio_isr_context_t));
    }
    pthread_mutex_unlock(&gpio_isr_mutex);

    /* 取消导出GPIO */
    return gpio_unexport(gpio_num);
}

int32_t HAL_GPIO_SetDirection(uint32_t gpio_num, hal_gpio_direction_t direction)
{
    char path[256];
    const char *dir_str;

    if (gpio_num >= MAX_GPIO_PINS) {
        return OSAL_EINVAL;
    }

    dir_str = (direction == HAL_GPIO_DIR_INPUT) ? "in" : "out";
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/direction", gpio_num);

    return gpio_write_file(path, dir_str);
}

int32_t HAL_GPIO_GetDirection(uint32_t gpio_num, hal_gpio_direction_t *direction)
{
    char path[256];
    char buffer[16];
    int32_t ret;

    if (gpio_num >= MAX_GPIO_PINS || !direction) {
        return OSAL_EINVAL;
    }

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/direction", gpio_num);
    ret = gpio_read_file(path, buffer, sizeof(buffer));
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    *direction = (strncmp(buffer, "in", 2) == 0) ? HAL_GPIO_DIR_INPUT : HAL_GPIO_DIR_OUTPUT;
    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_SetLevel(uint32_t gpio_num, hal_gpio_level_t level)
{
    char path[256];
    const char *level_str;

    if (gpio_num >= MAX_GPIO_PINS) {
        return OSAL_EINVAL;
    }

    level_str = (level == HAL_GPIO_LEVEL_HIGH) ? "1" : "0";
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/value", gpio_num);

    return gpio_write_file(path, level_str);
}

int32_t HAL_GPIO_GetLevel(uint32_t gpio_num, hal_gpio_level_t *level)
{
    char path[256];
    char buffer[4];
    int32_t ret;

    if (gpio_num >= MAX_GPIO_PINS || !level) {
        return OSAL_EINVAL;
    }

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/value", gpio_num);
    ret = gpio_read_file(path, buffer, sizeof(buffer));
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    *level = (buffer[0] == '1') ? HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW;
    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_SetInterrupt(uint32_t gpio_num, hal_gpio_edge_t edge,
                               hal_gpio_isr_callback_t callback, void *user_data)
{
    char path[256];
    const char *edge_str;
    int32_t ret;
    int fd;

    if (gpio_num >= MAX_GPIO_PINS || !callback) {
        return OSAL_EINVAL;
    }

    /* 设置触发模式 */
    switch (edge) {
        case HAL_GPIO_EDGE_NONE:    edge_str = "none"; break;
        case HAL_GPIO_EDGE_RISING:  edge_str = "rising"; break;
        case HAL_GPIO_EDGE_FALLING: edge_str = "falling"; break;
        case HAL_GPIO_EDGE_BOTH:    edge_str = "both"; break;
        default: return OSAL_EINVAL;
    }

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/edge", gpio_num);
    ret = gpio_write_file(path, edge_str);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    if (edge == HAL_GPIO_EDGE_NONE) {
        return OSAL_SUCCESS;
    }

    /* 打开value文件用于poll */
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/value", gpio_num);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return OSAL_EIO;
    }

    /* 初始化中断上下文 */
    pthread_mutex_lock(&gpio_isr_mutex);

    /* 如果已有线程在运行，先停止 */
    if (gpio_isr_table[gpio_num].running) {
        gpio_isr_table[gpio_num].running = false;
        pthread_mutex_unlock(&gpio_isr_mutex);
        pthread_join(gpio_isr_table[gpio_num].thread, NULL);
        pthread_mutex_lock(&gpio_isr_mutex);
        if (gpio_isr_table[gpio_num].value_fd >= 0) {
            close(gpio_isr_table[gpio_num].value_fd);
        }
    }

    gpio_isr_table[gpio_num].gpio_num = gpio_num;
    gpio_isr_table[gpio_num].value_fd = fd;
    gpio_isr_table[gpio_num].callback = callback;
    gpio_isr_table[gpio_num].user_data = user_data;
    gpio_isr_table[gpio_num].enabled = true;
    gpio_isr_table[gpio_num].running = true;

    /* 创建监听线程 */
    ret = pthread_create(&gpio_isr_table[gpio_num].thread, NULL,
                         gpio_isr_thread, &gpio_isr_table[gpio_num]);

    pthread_mutex_unlock(&gpio_isr_mutex);

    if (ret != 0) {
        close(fd);
        memset(&gpio_isr_table[gpio_num], 0, sizeof(gpio_isr_context_t));
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_EnableInterrupt(uint32_t gpio_num)
{
    if (gpio_num >= MAX_GPIO_PINS) {
        return OSAL_EINVAL;
    }

    pthread_mutex_lock(&gpio_isr_mutex);
    if (!gpio_isr_table[gpio_num].running) {
        pthread_mutex_unlock(&gpio_isr_mutex);
        return OSAL_EINVAL;
    }
    gpio_isr_table[gpio_num].enabled = true;
    pthread_mutex_unlock(&gpio_isr_mutex);

    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_DisableInterrupt(uint32_t gpio_num)
{
    if (gpio_num >= MAX_GPIO_PINS) {
        return OSAL_EINVAL;
    }

    pthread_mutex_lock(&gpio_isr_mutex);
    if (!gpio_isr_table[gpio_num].running) {
        pthread_mutex_unlock(&gpio_isr_mutex);
        return OSAL_EINVAL;
    }
    gpio_isr_table[gpio_num].enabled = false;
    pthread_mutex_unlock(&gpio_isr_mutex);

    return OSAL_SUCCESS;
}
