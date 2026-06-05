/************************************************************************
 * OSAL File API
 *
 * File operations (open, read, write, ioctl)
 ************************************************************************/

#ifndef OSAL_FILE_H
#define OSAL_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 文件操作标志（对应fcntl.h中的O_*标志）
 *===========================================================================*/

/* 访问模式 */
#define OSAL_O_RDONLY    0x0000  /* 只读 */
#define OSAL_O_WRONLY    0x0001  /* 只写 */
#define OSAL_O_RDWR      0x0002  /* 读写 */

/* 文件创建标志 */
#define OSAL_O_CREAT     0x0040  /* 文件不存在则创建 */
#define OSAL_O_EXCL      0x0080  /* 与O_CREAT一起使用，文件存在则失败 */
#define OSAL_O_TRUNC     0x0200  /* 截断文件 */
#define OSAL_O_APPEND    0x0400  /* 追加模式 */

/* 文件状态标志 */
#define OSAL_O_NONBLOCK  0x0800  /* 非阻塞模式 */
#define OSAL_O_NOCTTY    0x8000  /* 不作为控制终端 */

/* 文件权限（对应mode_t） */
#define OSAL_S_IRUSR     0x0100  /* 用户读权限 */
#define OSAL_S_IWUSR     0x0080  /* 用户写权限 */
#define OSAL_S_IXUSR     0x0040  /* 用户执行权限 */
#define OSAL_S_IRGRP     0x0020  /* 组读权限 */
#define OSAL_S_IWGRP     0x0010  /* 组写权限 */
#define OSAL_S_IXGRP     0x0008  /* 组执行权限 */
#define OSAL_S_IROTH     0x0004  /* 其他读权限 */
#define OSAL_S_IWOTH     0x0002  /* 其他写权限 */
#define OSAL_S_IXOTH     0x0001  /* 其他执行权限 */

/* lseek的whence参数 */
#define OSAL_SEEK_SET    0x00  /* 从文件开头 */
#define OSAL_SEEK_CUR    0x01  /* 从当前位置 */
#define OSAL_SEEK_END    0x02  /* 从文件末尾 */

/* fcntl命令 */
#define OSAL_F_GETFL     0x03  /* 获取文件状态标志 */
#define OSAL_F_SETFL     0x04  /* 设置文件状态标志 */
#define OSAL_F_GETFD     0x01  /* 获取文件描述符标志 */
#define OSAL_F_SETFD     0x02  /* 设置文件描述符标志 */

/* access()的mode参数 */
#define OSAL_F_OK    0x00  /* 文件存在 */
#define OSAL_R_OK    0x04  /* 可读 */
#define OSAL_W_OK    0x02  /* 可写 */
#define OSAL_X_OK    0x01  /* 可执行 */

/*===========================================================================
 * 文件I/O操作 API
 *===========================================================================*/

/**
 * @brief 打开文件
 * @param pathname 文件路径
 * @param flags 打开标志（OSAL_O_*）
 * @param mode 文件权限（OSAL_S_*），仅在创建文件时使用
 * @return 文件描述符(>=0)，失败返回-1
 */
int32_t OSAL_open(const char *pathname, int32_t flags, uint32_t mode);

/**
 * @brief 关闭文件描述符
 * @param fd 文件描述符
 * @return 0成功，-1失败
 */
int32_t OSAL_close(int32_t fd);

/**
 * @brief 读取数据
 * @param fd 文件描述符
 * @param buf 缓冲区
 * @param count 字节数（最大4GB）
 * @return 实际读取字节数(>=0)，0表示EOF，-1失败
 */
int32_t OSAL_read(int32_t fd, void *buf, uint32_t count);

/**
 * @brief 写入数据
 * @param fd 文件描述符
 * @param buf 缓冲区
 * @param count 字节数（最大4GB）
 * @return 实际写入字节数(>=0)，-1失败
 */
int32_t OSAL_write(int32_t fd, const void *buf, uint32_t count);

/**
 * @brief 移动文件读写位置
 * @param fd 文件描述符
 * @param offset 偏移量（支持大文件）
 * @param whence 起始位置（OSAL_SEEK_SET/CUR/END）
 * @return 新的文件位置(>=0)，-1失败
 */
int64_t OSAL_lseek(int32_t fd, int64_t offset, int32_t whence);

/**
 * @brief 文件控制操作
 * @param fd 文件描述符
 * @param cmd 命令（OSAL_F_*）
 * @param arg 参数（依赖于cmd）
 * @return 依赖于cmd，失败返回-1
 */
int32_t OSAL_fcntl(int32_t fd, int32_t cmd, int32_t arg);

/**
 * @brief 设备I/O控制
 * @param fd 文件描述符
 * @param request 请求码
 * @param argp 参数指针
 * @return 0成功，-1失败
 */
int32_t OSAL_ioctl(int32_t fd, uint32_t request, void *argp);

/**
 * @brief 删除文件
 * @param pathname 文件路径
 * @return 0成功，-1失败
 */
int32_t OSAL_unlink(const char *pathname);

/**
 * @brief 检查文件访问权限
 * @param pathname 文件路径
 * @param mode 检查模式（OSAL_F_OK/R_OK/W_OK/X_OK）
 * @return 0成功（有权限），-1失败（无权限或文件不存在）
 */
int32_t OSAL_access(const char *pathname, int32_t mode);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_FILE_H */
