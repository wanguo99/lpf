/************************************************************************
 * OSAL - PTY系统调用封装
 *
 * 功能：
 * - 封装POSIX openpty API
 * - 提供伪终端对创建接口
 * - 仅暴露必要的文件描述符和名称参数
 ************************************************************************/

#ifndef OSAL_PTY_H
#define OSAL_PTY_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建伪终端对
 * @param[out] master_fd 主端文件描述符
 * @param[out] slave_fd 从端文件描述符
 * @param[out] name 从端名称缓冲区，可为NULL
 * @param[in] termios_p 终端属性，可为NULL
 * @param[in] winsize_p 窗口大小，可为NULL
 * @return 0成功，-1失败
 */
int32_t osal_openpty(int32_t *master_fd, int32_t *slave_fd, char *name,
					 const void *termios_p, const void *winsize_p);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_PTY_H */
