/************************************************************************
 * HAL层 - GPIO硬件抽象接口 (Linux实现)
 *
 * 使用Linux sysfs GPIO接口实现
 ************************************************************************/

#include "osal.h"
#include "hal.h"
#include "hal_gpio_internal.h"

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
static osal_mutex_t gpio_isr_mutex = OSAL_MUTEX_INITIALIZER;
static osal_mutex_t gpio_init_mutex = OSAL_MUTEX_INITIALIZER;
static bool gpio_module_initialized = false;
static osal_flock_t *g_gpio_flock = NULL;

/*===========================================================================
 * Private static helper functions
 *===========================================================================*/

/**
 * @brief 写入字符串到文件
 */
static int32_t _gpio_write_file(const char *path, const char *value)
{
	int32_t fd;
	uint32_t len;
	int32_t written;

	fd = osal_open(path, OSAL_O_WRONLY, 0);
	if (fd < 0) {
		int32_t err = osal_get_errno();
		return err;
	}

	len = (uint32_t)osal_strlen(value);
	written = osal_write(fd, value, len);
	osal_close(fd);

	if (written != (int32_t)len) {
		int32_t err = osal_get_errno();
		return err;
	}

	return OSAL_SUCCESS;
}

/**
 * @brief 从文件读取字符串
 */
static int32_t _gpio_read_file(const char *path, char *buffer, size_t size)
{
	int32_t fd;
	int32_t len;

	fd = osal_open(path, OSAL_O_RDONLY, 0);
	if (fd < 0) {
		int32_t err = osal_get_errno();
		return err;
	}

	len = osal_read(fd, buffer, (uint32_t)(size - 1));
	osal_close(fd);

	if (len < 0) {
		int32_t err = osal_get_errno();
		return err;
	}

	buffer[len] = '\0';
	return OSAL_SUCCESS;
}

/**
 * @brief 导出GPIO引脚
 */
static int32_t _gpio_export(uint32_t gpio_num)
{
	char gpio_str[16];
	char path[256];

	/* 检查GPIO是否已导出 */
	osal_snprintf(path, OSAL_sizeof(path), "/sys/class/gpio/gpio%u", gpio_num);
	if (osal_access(path, OSAL_F_OK) == 0) {
		return OSAL_SUCCESS; /* 已导出 */
	}

	/* 导出GPIO */
	osal_snprintf(gpio_str, OSAL_sizeof(gpio_str), "%u", gpio_num);
	return _gpio_write_file("/sys/class/gpio/export", gpio_str);
}

/**
 * @brief 取消导出GPIO引脚
 */
static int32_t _gpio_unexport(uint32_t gpio_num)
{
	char gpio_str[16];
	osal_snprintf(gpio_str, OSAL_sizeof(gpio_str), "%u", gpio_num);
	return _gpio_write_file("/sys/class/gpio/unexport", gpio_str);
}

static void _gpio_reset_isr_context(uint32_t gpio_num)
{
	osal_memset(&gpio_isr_table[gpio_num], 0, OSAL_sizeof(gpio_isr_context_t));
	gpio_isr_table[gpio_num].value_fd = -1;
}

static bool _gpio_direction_is_valid(hal_gpio_direction_t direction)
{
	return direction == HAL_GPIO_DIR_INPUT || direction == HAL_GPIO_DIR_OUTPUT;
}

static bool _gpio_level_is_valid(hal_gpio_level_t level)
{
	return level == HAL_GPIO_LEVEL_LOW || level == HAL_GPIO_LEVEL_HIGH;
}

static const char *_gpio_edge_to_string(hal_gpio_edge_t edge)
{
	switch (edge) {
	case HAL_GPIO_EDGE_NONE:
		return "none";
	case HAL_GPIO_EDGE_RISING:
		return "rising";
	case HAL_GPIO_EDGE_FALLING:
		return "falling";
	case HAL_GPIO_EDGE_BOTH:
		return "both";
	default:
		return NULL;
	}
}

/**
 * @brief GPIO中断监听线程
 */
static void *_gpio_isr_thread(void *arg)
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
	osal_lseek(ctx->value_fd, 0, OSAL_SEEK_SET);
	bytes_read =
		osal_read(ctx->value_fd, value_str, OSAL_sizeof(value_str) - 1);
	(void)bytes_read; /* Suppress unused result warning */

	while (ctx->running) {
		ret = osal_poll(&pfd, 1, 1000); /* 1秒超时 */

		if (ret > 0 && (pfd.revents & OSAL_POLLPRI)) {
			if (!ctx->enabled) {
				/* 清除中断但不调用回调 */
				osal_lseek(ctx->value_fd, 0, OSAL_SEEK_SET);
				bytes_read = osal_read(ctx->value_fd, value_str,
									   OSAL_sizeof(value_str) - 1);
				(void)bytes_read; /* Suppress unused result warning */
				continue;
			}

			/* 读取当前电平 */
			osal_lseek(ctx->value_fd, 0, OSAL_SEEK_SET);
			len =
				osal_read(ctx->value_fd, value_str, OSAL_sizeof(value_str) - 1);
			if (len > 0) {
				value_str[len] = '\0';
				level = (value_str[0] == '1') ? HAL_GPIO_LEVEL_HIGH :
												HAL_GPIO_LEVEL_LOW;

				/* 调用回调函数 */
				if (ctx->callback) {
					ctx->callback(ctx->gpio_num, level, ctx->user_data);
				}
			}
		}
	}

	return NULL;
}

/**
 * @brief GPIO模块初始化
 */
static int32_t _gpio_module_init(void)
{
	int32_t ret;
	uint32_t i;

	osal_mutex_lock(&gpio_init_mutex);

	if (gpio_module_initialized) {
		osal_mutex_unlock(&gpio_init_mutex);
		return OSAL_SUCCESS; /* 已初始化 */
	}

	/* 初始化GPIO中断上下文表 - 修复未初始化问题 */
	for (i = 0; i < MAX_GPIO_PINS; i++) {
		_gpio_reset_isr_context(i);
	}

	/* 创建文件锁 */
	ret = osal_flock_create(HAL_GPIO_LOCK_PATH, &g_gpio_flock);
	if (ret != OSAL_SUCCESS) {
		osal_mutex_unlock(&gpio_init_mutex);
		return ret;
	}

	gpio_module_initialized = true;
	osal_mutex_unlock(&gpio_init_mutex);
	return OSAL_SUCCESS;
}

static int32_t _gpio_lock(void)
{
	int32_t ret;

	ret = _gpio_module_init();
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	ret = osal_flock_lock(g_gpio_flock, OSAL_FLOCK_EXCLUSIVE);
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	ret = osal_mutex_lock(&gpio_isr_mutex);
	if (ret != OSAL_SUCCESS) {
		osal_flock_unlock(g_gpio_flock);
		return ret;
	}

	return OSAL_SUCCESS;
}

static void _gpio_unlock(void)
{
	osal_mutex_unlock(&gpio_isr_mutex);
	osal_flock_unlock(g_gpio_flock);
}

static void _gpio_stop_interrupt_unlocked(uint32_t gpio_num)
{
	osal_thread_t thread;
	bool running;

	running = gpio_isr_table[gpio_num].running;
	thread = gpio_isr_table[gpio_num].thread;

	if (running) {
		gpio_isr_table[gpio_num].running = false;
		osal_thread_join(thread, NULL);
	}

	if (gpio_isr_table[gpio_num].value_fd >= 0) {
		osal_close(gpio_isr_table[gpio_num].value_fd);
	}

	_gpio_reset_isr_context(gpio_num);
}

static int32_t _gpio_set_direction_unlocked(uint32_t gpio_num,
											hal_gpio_direction_t direction)
{
	char path[256];
	const char *dir_str;

	dir_str = (direction == HAL_GPIO_DIR_INPUT) ? "in" : "out";
	osal_snprintf(path, OSAL_sizeof(path), "/sys/class/gpio/gpio%u/direction",
				  gpio_num);

	return _gpio_write_file(path, dir_str);
}

static int32_t _gpio_set_level_unlocked(uint32_t gpio_num,
										hal_gpio_level_t level)
{
	char path[256];
	const char *level_str;

	level_str = (level == HAL_GPIO_LEVEL_HIGH) ? "1" : "0";
	osal_snprintf(path, OSAL_sizeof(path), "/sys/class/gpio/gpio%u/value",
				  gpio_num);

	return _gpio_write_file(path, level_str);
}

static int32_t _gpio_set_interrupt_unlocked(uint32_t gpio_num,
											hal_gpio_edge_t edge,
											hal_gpio_isr_callback_t callback,
											void *user_data)
{
	char path[256];
	const char *edge_str;
	int32_t ret;
	int32_t fd;

	edge_str = _gpio_edge_to_string(edge);
	if (!edge_str || (edge != HAL_GPIO_EDGE_NONE && !callback)) {
		return OSAL_ERR_INVALID_PARAM;
	}

	osal_snprintf(path, OSAL_sizeof(path), "/sys/class/gpio/gpio%u/edge",
				  gpio_num);
	ret = _gpio_write_file(path, edge_str);
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	_gpio_stop_interrupt_unlocked(gpio_num);

	if (edge == HAL_GPIO_EDGE_NONE) {
		return OSAL_SUCCESS;
	}

	osal_snprintf(path, OSAL_sizeof(path), "/sys/class/gpio/gpio%u/value",
				  gpio_num);
	fd = osal_open(path, OSAL_O_RDONLY, 0);
	if (fd < 0) {
		return osal_get_errno();
	}

	gpio_isr_table[gpio_num].gpio_num = gpio_num;
	gpio_isr_table[gpio_num].value_fd = fd;
	gpio_isr_table[gpio_num].callback = callback;
	gpio_isr_table[gpio_num].user_data = user_data;
	gpio_isr_table[gpio_num].enabled = true;
	gpio_isr_table[gpio_num].running = true;

	ret = osal_thread_create(&gpio_isr_table[gpio_num].thread, NULL,
							 _gpio_isr_thread, &gpio_isr_table[gpio_num]);
	if (ret != OSAL_SUCCESS) {
		osal_close(fd);
		_gpio_reset_isr_context(gpio_num);
		return ret;
	}

	return OSAL_SUCCESS;
}

/*===========================================================================
 * Primary API functions
 *===========================================================================*/

int32_t hal_gpio_set_direction(uint32_t gpio_num,
							   hal_gpio_direction_t direction)
{
	int32_t ret;

	if (gpio_num >= MAX_GPIO_PINS || !_gpio_direction_is_valid(direction)) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ret = _gpio_lock();
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	ret = _gpio_set_direction_unlocked(gpio_num, direction);
	_gpio_unlock();

	return ret;
}

int32_t hal_gpio_get_direction(uint32_t gpio_num,
							   hal_gpio_direction_t *direction)
{
	char path[256];
	char buffer[16];
	int32_t ret;

	if (gpio_num >= MAX_GPIO_PINS || !direction) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ret = _gpio_lock();
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	osal_snprintf(path, OSAL_sizeof(path), "/sys/class/gpio/gpio%u/direction",
				  gpio_num);
	ret = _gpio_read_file(path, buffer, OSAL_sizeof(buffer));
	if (ret != OSAL_SUCCESS) {
		_gpio_unlock();
		return ret;
	}

	*direction = (osal_strncmp(buffer, "in", 2) == 0) ? HAL_GPIO_DIR_INPUT :
														HAL_GPIO_DIR_OUTPUT;

	_gpio_unlock();

	return OSAL_SUCCESS;
}

int32_t hal_gpio_set_level(uint32_t gpio_num, hal_gpio_level_t level)
{
	int32_t ret;

	if (gpio_num >= MAX_GPIO_PINS || !_gpio_level_is_valid(level)) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ret = _gpio_lock();
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	ret = _gpio_set_level_unlocked(gpio_num, level);
	_gpio_unlock();

	return ret;
}

int32_t hal_gpio_get_level(uint32_t gpio_num, hal_gpio_level_t *level)
{
	char path[256];
	char buffer[4];
	int32_t ret;

	if (gpio_num >= MAX_GPIO_PINS || !level) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ret = _gpio_lock();
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	osal_snprintf(path, OSAL_sizeof(path), "/sys/class/gpio/gpio%u/value",
				  gpio_num);
	ret = _gpio_read_file(path, buffer, OSAL_sizeof(buffer));
	if (ret != OSAL_SUCCESS) {
		_gpio_unlock();
		return ret;
	}

	*level = (buffer[0] == '1') ? HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW;

	_gpio_unlock();

	return OSAL_SUCCESS;
}

int32_t hal_gpio_set_interrupt(uint32_t gpio_num, hal_gpio_edge_t edge,
							   hal_gpio_isr_callback_t callback,
							   void *user_data)
{
	int32_t ret;

	if (gpio_num >= MAX_GPIO_PINS) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ret = _gpio_lock();
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	ret = _gpio_set_interrupt_unlocked(gpio_num, edge, callback, user_data);
	_gpio_unlock();

	return ret;
}

int32_t hal_gpio_enable_interrupt(uint32_t gpio_num)
{
	int32_t ret;

	if (gpio_num >= MAX_GPIO_PINS) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ret = _gpio_lock();
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	if (!gpio_isr_table[gpio_num].running) {
		_gpio_unlock();
		return OSAL_ERR_INVALID_PARAM;
	}
	gpio_isr_table[gpio_num].enabled = true;
	_gpio_unlock();

	return OSAL_SUCCESS;
}

int32_t hal_gpio_disable_interrupt(uint32_t gpio_num)
{
	int32_t ret;

	if (gpio_num >= MAX_GPIO_PINS) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ret = _gpio_lock();
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	if (!gpio_isr_table[gpio_num].running) {
		_gpio_unlock();
		return OSAL_ERR_INVALID_PARAM;
	}
	gpio_isr_table[gpio_num].enabled = false;
	_gpio_unlock();

	return OSAL_SUCCESS;
}

/*===========================================================================
 * Initialization and cleanup functions
 *===========================================================================*/

int32_t hal_gpio_init(uint32_t gpio_num, const hal_gpio_config_t *config)
{
	int32_t ret;

	if (!config || gpio_num >= MAX_GPIO_PINS ||
		!_gpio_direction_is_valid(config->direction) ||
		!_gpio_level_is_valid(config->initial_level) ||
		!_gpio_edge_to_string(config->edge) ||
		(config->edge != HAL_GPIO_EDGE_NONE && !config->callback)) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ret = _gpio_lock();
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	/* 导出GPIO */
	ret = _gpio_export(gpio_num);
	if (ret != OSAL_SUCCESS) {
		_gpio_unlock();
		return ret;
	}

	/* 等待sysfs文件创建 */
	osal_msleep(100); /* 100ms */

	/* 设置方向 */
	ret = _gpio_set_direction_unlocked(gpio_num, config->direction);
	if (ret != OSAL_SUCCESS) {
		_gpio_unexport(gpio_num);
		_gpio_unlock();
		return ret;
	}

	/* 如果是输出模式，设置初始电平 */
	if (config->direction == HAL_GPIO_DIR_OUTPUT) {
		ret = _gpio_set_level_unlocked(gpio_num, config->initial_level);
		if (ret != OSAL_SUCCESS) {
			_gpio_unexport(gpio_num);
			_gpio_unlock();
			return ret;
		}
	}

	/* 配置中断 */
	if (config->edge != HAL_GPIO_EDGE_NONE && config->callback) {
		ret = _gpio_set_interrupt_unlocked(gpio_num, config->edge,
										   config->callback, config->user_data);
		if (ret != OSAL_SUCCESS) {
			_gpio_unexport(gpio_num);
			_gpio_unlock();
			return ret;
		}
	}

	_gpio_unlock();

	return OSAL_SUCCESS;
}

int32_t hal_gpio_deinit(uint32_t gpio_num)
{
	int32_t ret;

	if (gpio_num >= MAX_GPIO_PINS) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ret = _gpio_lock();
	if (ret != OSAL_SUCCESS) {
		return ret;
	}

	/* 停止中断监听线程 */
	_gpio_stop_interrupt_unlocked(gpio_num);

	/* 取消导出GPIO */
	ret = _gpio_unexport(gpio_num);

	_gpio_unlock();

	return ret;
}
