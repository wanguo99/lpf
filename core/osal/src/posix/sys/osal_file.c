/************************************************************************
 * OSAL - 文件I/O操作封装实现（POSIX）
 ************************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdint.h>
#include <errno.h>
#include "osal.h"

/*===========================================================================
 * 标志位映射（OSAL -> POSIX）
 *===========================================================================*/

static int32_t osal_flags_to_posix(int32_t osal_flags)
{
    int32_t posix_flags = 0;

    /* 访问模式 */
    if ((osal_flags & 0x0003) == OSAL_O_RDONLY) {
        posix_flags |= O_RDONLY;
    } else if ((osal_flags & 0x0003) == OSAL_O_WRONLY) {
        posix_flags |= O_WRONLY;
    } else if ((osal_flags & 0x0003) == OSAL_O_RDWR) {
        posix_flags |= O_RDWR;
    }

    /* 文件创建标志 */
    if (osal_flags & OSAL_O_CREAT)    posix_flags |= O_CREAT;
    if (osal_flags & OSAL_O_EXCL)     posix_flags |= O_EXCL;
    if (osal_flags & OSAL_O_TRUNC)    posix_flags |= O_TRUNC;
    if (osal_flags & OSAL_O_APPEND)   posix_flags |= O_APPEND;

    /* 文件状态标志 */
    if (osal_flags & OSAL_O_NONBLOCK) posix_flags |= O_NONBLOCK;
    if (osal_flags & OSAL_O_NOCTTY)   posix_flags |= O_NOCTTY;

    return posix_flags;
}

static int32_t posix_flags_to_osal(int32_t posix_flags)
{
    int32_t osal_flags = 0;

    /* 访问模式 */
    int32_t access_mode = posix_flags & O_ACCMODE;
    if (access_mode == O_RDONLY) {
        osal_flags |= OSAL_O_RDONLY;
    } else if (access_mode == O_WRONLY) {
        osal_flags |= OSAL_O_WRONLY;
    } else if (access_mode == O_RDWR) {
        osal_flags |= OSAL_O_RDWR;
    }

    /* 文件创建标志 */
    if (posix_flags & O_CREAT)    osal_flags |= OSAL_O_CREAT;
    if (posix_flags & O_EXCL)     osal_flags |= OSAL_O_EXCL;
    if (posix_flags & O_TRUNC)    osal_flags |= OSAL_O_TRUNC;
    if (posix_flags & O_APPEND)   osal_flags |= OSAL_O_APPEND;

    /* 文件状态标志 */
    if (posix_flags & O_NONBLOCK) osal_flags |= OSAL_O_NONBLOCK;
    if (posix_flags & O_NOCTTY)   osal_flags |= OSAL_O_NOCTTY;

    return osal_flags;
}

static uint32_t osal_mode_to_posix(uint32_t osal_mode)
{
    uint32_t posix_mode = 0;

    if (osal_mode & OSAL_S_IRUSR) posix_mode |= S_IRUSR;
    if (osal_mode & OSAL_S_IWUSR) posix_mode |= S_IWUSR;
    if (osal_mode & OSAL_S_IXUSR) posix_mode |= S_IXUSR;
    if (osal_mode & OSAL_S_IRGRP) posix_mode |= S_IRGRP;
    if (osal_mode & OSAL_S_IWGRP) posix_mode |= S_IWGRP;
    if (osal_mode & OSAL_S_IXGRP) posix_mode |= S_IXGRP;
    if (osal_mode & OSAL_S_IROTH) posix_mode |= S_IROTH;
    if (osal_mode & OSAL_S_IWOTH) posix_mode |= S_IWOTH;
    if (osal_mode & OSAL_S_IXOTH) posix_mode |= S_IXOTH;

    return posix_mode;
}

/*===========================================================================
 * 文件I/O操作
 *===========================================================================*/

int32_t OSAL_open(const char *pathname, int32_t flags, uint32_t mode)
{
    int32_t posix_flags = osal_flags_to_posix(flags);
    uint32_t posix_mode = osal_mode_to_posix(mode);
    int32_t result = open(pathname, posix_flags, posix_mode);
    return result;
}

int32_t OSAL_close(int32_t fd)
{
    int32_t result = close(fd);
    return result;
}

int32_t OSAL_read(int32_t fd, void *buf, uint32_t count)
{
    osal_ssize_t result;
    int32_t safe_result;

    result = read(fd, buf, (osal_size_t)count);

    /* 错误情况直接返回 */
    if (result < 0) {
        return -1;
    }

    /* 检查返回值是否超出 int32_t 范围 */
    if (result > INT32_MAX) {
        errno = EOVERFLOW;
        return -1;
    }

    /* 安全转换：已验证 result 在 [0, INT32_MAX] 范围内 */
    safe_result = (int32_t)result;
    return safe_result;
}

int32_t OSAL_write(int32_t fd, const void *buf, uint32_t count)
{
    osal_ssize_t result;
    int32_t safe_result;

    result = write(fd, buf, (osal_size_t)count);

    /* 错误情况直接返回 */
    if (result < 0) {
        return -1;
    }

    /* 检查返回值是否超出 int32_t 范围 */
    if (result > INT32_MAX) {
        errno = EOVERFLOW;
        return -1;
    }

    /* 安全转换：已验证 result 在 [0, INT32_MAX] 范围内 */
    safe_result = (int32_t)result;
    return safe_result;
}

int64_t OSAL_lseek(int32_t fd, int64_t offset, int32_t whence)
{
    osal_off_t result = lseek(fd, (osal_off_t)offset, whence);

    /* 错误情况直接返回 */
    if (result == (osal_off_t)-1) {
        return -1;
    }

    /* osal_off_t 在 64 位系统上是 int64_t，直接返回 */
    return (int64_t)result;
}

/*===========================================================================
 * 文件控制操作
 *===========================================================================*/

int32_t OSAL_fcntl(int32_t fd, int32_t cmd, int32_t arg)
{
    int32_t result;

    if (cmd == OSAL_F_GETFL) {
        /* 获取标志：需要转换POSIX -> OSAL */
        result = fcntl(fd, F_GETFL, 0);
        if (result >= 0) {
            result = posix_flags_to_osal(result);
        }
    } else if (cmd == OSAL_F_SETFL) {
        /* 设置标志：需要转换OSAL -> POSIX */
        int32_t posix_flags = osal_flags_to_posix(arg);
        result = fcntl(fd, F_SETFL, posix_flags);
    } else if (cmd == OSAL_F_GETFD) {
        result = fcntl(fd, F_GETFD, 0);
    } else if (cmd == OSAL_F_SETFD) {
        result = fcntl(fd, F_SETFD, arg);
    } else {
        /* 其他命令直接传递 */
        result = fcntl(fd, cmd, arg);
    }

    return result;
}

/*===========================================================================
 * 设备控制操作
 *===========================================================================*/

int32_t OSAL_ioctl(int32_t fd, uint32_t request, void *argp)
{
    /* uint32_t 转换为 unsigned long（ioctl 的标准参数类型） */
    int32_t result = ioctl(fd, (unsigned long)request, argp);
    return result;
}

/*===========================================================================
 * 文件系统操作
 *===========================================================================*/

int32_t OSAL_unlink(const char *pathname)
{
    int32_t result = unlink(pathname);
    return result;
}

/*===========================================================================
 * 文件访问权限检查
 *===========================================================================*/

int32_t OSAL_access(const char *pathname, int32_t mode)
{
    int32_t result = access(pathname, mode);
    return result;
}
