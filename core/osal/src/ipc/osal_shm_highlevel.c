/************************************************************************
 * OSAL 高级共享内存 API 实现
 *
 * 基于 POSIX shm_open/mmap 的高级封装
 ************************************************************************/

#include "osal.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* 内部数据结构：保存共享内存的大小信息 */
typedef struct {
	int32_t fd;
	osal_size_t size;
} shm_context_t;

/* 简单的句柄表（最多支持 32 个并发共享内存） */
#define MAX_SHM_HANDLES 32
static shm_context_t shm_table[MAX_SHM_HANDLES];
static int shm_table_initialized = 0;

/* 初始化句柄表 */
static void _init_shm_table(void)
{
	if (!shm_table_initialized) {
		memset(shm_table, 0, sizeof(shm_table));
		for (int i = 0; i < MAX_SHM_HANDLES; i++) {
			shm_table[i].fd = -1;
		}
		shm_table_initialized = 1;
	}
}

/* 分配句柄 */
static int32_t _alloc_shm_handle(int32_t fd, osal_size_t size)
{
	_init_shm_table();

	for (int32_t i = 0; i < MAX_SHM_HANDLES; i++) {
		if (shm_table[i].fd == -1) {
			shm_table[i].fd = fd;
			shm_table[i].size = size;
			return i;
		}
	}
	return -1;
}

/* 释放句柄 */
static void _free_shm_handle(int32_t handle)
{
	if (handle >= 0 && handle < MAX_SHM_HANDLES) {
		shm_table[handle].fd = -1;
		shm_table[handle].size = 0;
	}
}

/* 获取句柄信息 */
static shm_context_t *_get_shm_context(int32_t handle)
{
	if (handle >= 0 && handle < MAX_SHM_HANDLES && shm_table[handle].fd != -1) {
		return &shm_table[handle];
	}
	return NULL;
}

/**
 * @brief 创建或打开共享内存
 */
int32_t osal_shm_create(const char *name, osal_size_t size, int32_t flags,
						osal_shm_t *shm)
{
	int32_t fd;
	int32_t oflag = 0;
	mode_t mode = 0666;

	if (!name || !shm) {
		return OSAL_ERR_INVALID_PARAM;
	}

	/* 转换标志 */
	if (flags & OSAL_SHM_CREATE) {
		oflag |= O_CREAT;
	}

	if (flags & OSAL_SHM_RDWR) {
		oflag |= O_RDWR;
	} else if (flags & OSAL_SHM_RDONLY) {
		oflag |= O_RDONLY;
	} else {
		oflag |= O_RDWR; /* 默认读写 */
	}

	/* 打开或创建共享内存 */
	fd = shm_open(name, oflag, mode);
	if (fd < 0) {
		return OSAL_ERR_GENERIC;
	}

	/* 如果创建，设置大小 */
	if (flags & OSAL_SHM_CREATE) {
		if (ftruncate(fd, size) < 0) {
			close(fd);
			return OSAL_ERR_GENERIC;
		}
	}

	/* 分配句柄 */
	int32_t handle = _alloc_shm_handle(fd, size);
	if (handle < 0) {
		close(fd);
		return OSAL_ERR_NO_MEMORY;
	}

	*shm = handle;
	return OSAL_SUCCESS;
}

/**
 * @brief 映射共享内存到进程地址空间
 */
int32_t osal_shm_map(osal_shm_t shm, osal_off_t offset, osal_size_t length,
					 int32_t flags, void **addr)
{
	shm_context_t *ctx;
	int32_t prot = 0;
	void *mapped_addr;

	if (!addr) {
		return OSAL_ERR_INVALID_PARAM;
	}

	ctx = _get_shm_context(shm);
	if (!ctx) {
		return OSAL_ERR_INVALID_PARAM;
	}

	/* 如果 length 为 0，使用整个共享内存大小 */
	if (length == 0) {
		length = ctx->size;
	}

	/* 转换标志 */
	if (flags & OSAL_SHM_RDWR) {
		prot = PROT_READ | PROT_WRITE;
	} else if (flags & OSAL_SHM_RDONLY) {
		prot = PROT_READ;
	} else {
		prot = PROT_READ | PROT_WRITE; /* 默认读写 */
	}

	/* 映射 */
	mapped_addr = mmap(NULL, length, prot, MAP_SHARED, ctx->fd, offset);
	if (mapped_addr == MAP_FAILED) {
		return OSAL_ERR_GENERIC;
	}

	*addr = mapped_addr;
	return OSAL_SUCCESS;
}

/**
 * @brief 解除共享内存映射
 */
int32_t osal_shm_unmap(void *addr, osal_size_t length)
{
	if (!addr) {
		return OSAL_ERR_INVALID_PARAM;
	}

	if (munmap(addr, length) < 0) {
		return OSAL_ERR_GENERIC;
	}

	return OSAL_SUCCESS;
}

/**
 * @brief 关闭共享内存句柄
 */
int32_t osal_shm_close(osal_shm_t shm)
{
	shm_context_t *ctx;

	ctx = _get_shm_context(shm);
	if (!ctx) {
		return OSAL_ERR_INVALID_PARAM;
	}

	close(ctx->fd);
	_free_shm_handle(shm);

	return OSAL_SUCCESS;
}

/**
 * @brief 删除共享内存对象
 */
int32_t osal_shm_remove(const char *name)
{
	if (!name) {
		return OSAL_ERR_INVALID_PARAM;
	}

	if (shm_unlink(name) < 0) {
		return OSAL_ERR_GENERIC;
	}

	return OSAL_SUCCESS;
}
