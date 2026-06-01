/************************************************************************
 * OSAL - 终端I/O控制封装（termios）
 *
 * 功能：
 * - 封装POSIX termios函数（tcgetattr/tcsetattr/tcflush等）
 * - 提供跨平台的串口配置接口
 * - 1:1映射系统调用，不引入业务逻辑
 *
 * 设计原则：
 * - 使用固定大小类型，避免平台相关类型
 * - 返回值与系统调用保持一致
 * - 便于RTOS移植
 ************************************************************************/

#ifndef OSAL_TERMIOS_H
#define OSAL_TERMIOS_H

#include "osal_platform.h"
#include "osal_types.h"

/*===========================================================================
 * termios数据结构（不透明类型）
 *===========================================================================*/

/**
 * @brief 终端属性结构（不透明类型）
 *
 * 注意：此结构体大小必须与系统的struct termios一致
 * Linux: 通常为60字节（x86_64）
 */
typedef struct {
    uint8_t opaque[128];  /* 不透明数据，足够容纳struct termios */
} osal_termios_t;

/*===========================================================================
 * tcsetattr的optional_actions参数
 *===========================================================================*/

#define OSAL_TCSANOW    0  /* 立即生效 */
#define OSAL_TCSADRAIN  1  /* 等待输出完成后生效 */
#define OSAL_TCSAFLUSH  2  /* 等待输出完成并丢弃未读输入后生效 */

/*===========================================================================
 * tcflush的queue_selector参数
 *===========================================================================*/

#define OSAL_TCIFLUSH   0  /* 刷新输入队列 */
#define OSAL_TCOFLUSH   1  /* 刷新输出队列 */
#define OSAL_TCIOFLUSH  2  /* 刷新输入和输出队列 */

/*===========================================================================
 * 波特率常量
 *===========================================================================*/

#define OSAL_B0         0x00000000  /* 挂断 */
#define OSAL_B50        0x00000001
#define OSAL_B75        0x00000002
#define OSAL_B110       0x00000003
#define OSAL_B134       0x00000004
#define OSAL_B150       0x00000005
#define OSAL_B200       0x00000006
#define OSAL_B300       0x00000007
#define OSAL_B600       0x00000008
#define OSAL_B1200      0x00000009
#define OSAL_B1800      0x0000000A
#define OSAL_B2400      0x0000000B
#define OSAL_B4800      0x0000000C
#define OSAL_B9600      0x0000000D
#define OSAL_B19200     0x0000000E
#define OSAL_B38400     0x0000000F
#define OSAL_B57600     0x00001001
#define OSAL_B115200    0x00001002
#define OSAL_B230400    0x00001003
#define OSAL_B460800    0x00001004
#define OSAL_B500000    0x00001005
#define OSAL_B576000    0x00001006
#define OSAL_B921600    0x00001007
#define OSAL_B1000000   0x00001008
#define OSAL_B1152000   0x00001009
#define OSAL_B1500000   0x0000100A
#define OSAL_B2000000   0x0000100B
#define OSAL_B2500000   0x0000100C
#define OSAL_B3000000   0x0000100D
#define OSAL_B3500000   0x0000100E
#define OSAL_B4000000   0x0000100F

/*===========================================================================
 * 终端I/O控制操作
 *===========================================================================*/

/**
 * @brief 获取终端属性
 * @param fd 文件描述符
 * @param termios_p 终端属性结构指针
 * @return 0成功，-1失败
 */
int32_t OSAL_tcgetattr(int32_t fd, osal_termios_t *termios_p);

/**
 * @brief 设置终端属性
 * @param fd 文件描述符
 * @param optional_actions 何时生效（OSAL_TCSANOW/TCSADRAIN/TCSAFLUSH）
 * @param termios_p 终端属性结构指针
 * @return 0成功，-1失败
 */
int32_t OSAL_tcsetattr(int32_t fd, int32_t optional_actions, const osal_termios_t *termios_p);

/**
 * @brief 刷新终端I/O缓冲区
 * @param fd 文件描述符
 * @param queue_selector 刷新哪个队列（OSAL_TCIFLUSH/TCOFLUSH/TCIOFLUSH）
 * @return 0成功，-1失败
 */
int32_t OSAL_tcflush(int32_t fd, int32_t queue_selector);

/**
 * @brief 设置终端输入波特率
 * @param termios_p 终端属性结构指针
 * @param speed 波特率（OSAL_B9600/B115200等）
 * @return 0成功，-1失败
 */
int32_t OSAL_cfsetispeed(osal_termios_t *termios_p, uint32_t speed);

/**
 * @brief 设置终端输出波特率
 * @param termios_p 终端属性结构指针
 * @param speed 波特率（OSAL_B9600/B115200等）
 * @return 0成功，-1失败
 */
int32_t OSAL_cfsetospeed(osal_termios_t *termios_p, uint32_t speed);

#endif /* OSAL_TERMIOS_H */
