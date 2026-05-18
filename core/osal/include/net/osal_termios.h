/************************************************************************
 * OSAL - termios系统调用封装
 *
 * 功能：
 * - 封装POSIX termios API（串口配置）
 * - 1:1映射系统调用，不引入业务逻辑
 * - 使用固定大小类型，避免平台相关类型
 *
 * 设计原则：
 * - 参数使用int32/uint32等固定大小类型
 * - 返回值与系统调用保持一致
 * - 便于RTOS移植
 ************************************************************************/

#ifndef OSAL_TERMIOS_H
#define OSAL_TERMIOS_H

#include "osal_types.h"

/*===========================================================================
 * termios结构体
 *===========================================================================*/

/* 控制字符数组大小 */
#define OSAL_NCCS 32

/**
 * @brief termios结构体
 */
typedef struct {
    uint32_t c_iflag;         /* 输入模式标志 */
    uint32_t c_oflag;         /* 输出模式标志 */
    uint32_t c_cflag;         /* 控制模式标志 */
    uint32_t c_lflag;         /* 本地模式标志 */
    uint8_t  c_line;          /* 线路规程 */
    uint8_t  c_cc[OSAL_NCCS]; /* 控制字符 */
    uint32_t c_ispeed;        /* 输入波特率 */
    uint32_t c_ospeed;        /* 输出波特率 */
} osal_termios_t;

/*===========================================================================
 * c_iflag - 输入模式标志
 *===========================================================================*/
#define OSAL_IGNBRK  0000001  /* 忽略BREAK */
#define OSAL_BRKINT  0000002  /* BREAK产生SIGINT */
#define OSAL_IGNPAR  0000004  /* 忽略奇偶校验错误 */
#define OSAL_PARMRK  0000010  /* 标记奇偶校验错误 */
#define OSAL_INPCK   0000020  /* 启用输入奇偶校验 */
#define OSAL_ISTRIP  0000040  /* 剥离第8位 */
#define OSAL_INLCR   0000100  /* 将NL映射为CR */
#define OSAL_IGNCR   0000200  /* 忽略CR */
#define OSAL_ICRNL   0000400  /* 将CR映射为NL */
#define OSAL_IXON    0002000  /* 启用输出软件流控 */
#define OSAL_IXANY   0004000  /* 任意字符重启输出 */
#define OSAL_IXOFF   0010000  /* 启用输入软件流控 */

/*===========================================================================
 * c_oflag - 输出模式标志
 *===========================================================================*/
#define OSAL_OPOST   0000001  /* 启用输出处理 */
#define OSAL_ONLCR   0000004  /* 将NL映射为CR-NL */
#define OSAL_OCRNL   0000010  /* 将CR映射为NL */

/*===========================================================================
 * c_cflag - 控制模式标志
 *===========================================================================*/
#define OSAL_CSIZE   0000060  /* 字符大小掩码 */
#define OSAL_CS5     0000000  /* 5位 */
#define OSAL_CS6     0000020  /* 6位 */
#define OSAL_CS7     0000040  /* 7位 */
#define OSAL_CS8     0000060  /* 8位 */
#define OSAL_CSTOPB  0000100  /* 2个停止位 */
#define OSAL_CREAD   0000200  /* 启用接收 */
#define OSAL_PARENB  0000400  /* 启用奇偶校验 */
#define OSAL_PARODD  0001000  /* 奇校验 */
#define OSAL_HUPCL   0002000  /* 关闭时挂断 */
#define OSAL_CLOCAL  0004000  /* 忽略调制解调器状态线 */
#define OSAL_CRTSCTS 020000000000 /* 硬件流控 */

/*===========================================================================
 * c_lflag - 本地模式标志
 *===========================================================================*/
#define OSAL_ISIG    0000001  /* 启用信号 */
#define OSAL_ICANON  0000002  /* 规范模式 */
#define OSAL_ECHO    0000010  /* 回显输入字符 */
#define OSAL_ECHOE   0000020  /* 回显ERASE */
#define OSAL_ECHOK   0000040  /* 回显KILL */
#define OSAL_ECHONL  0000100  /* 回显NL */
#define OSAL_NOFLSH  0000200  /* 禁止在信号后刷新 */
#define OSAL_TOSTOP  0000400  /* 后台输出发送SIGTTOU */
#define OSAL_IEXTEN  0100000  /* 启用扩展功能 */

/*===========================================================================
 * c_cc - 控制字符索引
 *===========================================================================*/
#define OSAL_VINTR    0   /* INTR字符 */
#define OSAL_VQUIT    1   /* QUIT字符 */
#define OSAL_VERASE   2   /* ERASE字符 */
#define OSAL_VKILL    3   /* KILL字符 */
#define OSAL_VEOF     4   /* EOF字符 */
#define OSAL_VTIME    5   /* 读超时（十分之一秒） */
#define OSAL_VMIN     6   /* 读最小字符数 */
#define OSAL_VSTART   8   /* START字符 */
#define OSAL_VSTOP    9   /* STOP字符 */

/*===========================================================================
 * tcsetattr的可选操作
 *===========================================================================*/
#define OSAL_TCSANOW   0  /* 立即生效 */
#define OSAL_TCSADRAIN 1  /* 输出完成后生效 */
#define OSAL_TCSAFLUSH 2  /* 输出完成后生效，并丢弃未读输入 */

/*===========================================================================
 * tcflush的队列选择器
 *===========================================================================*/
#define OSAL_TCIFLUSH  0  /* 刷新输入队列 */
#define OSAL_TCOFLUSH  1  /* 刷新输出队列 */
#define OSAL_TCIOFLUSH 2  /* 刷新输入和输出队列 */

/*===========================================================================
 * tcflow的操作
 *===========================================================================*/
#define OSAL_TCOOFF 0  /* 挂起输出 */
#define OSAL_TCOON  1  /* 恢复输出 */
#define OSAL_TCIOFF 2  /* 发送STOP字符 */
#define OSAL_TCION  3  /* 发送START字符 */

/*===========================================================================
 * 波特率常量
 *===========================================================================*/
#define OSAL_B0       0000000  /* 挂断 */
#define OSAL_B50      0000001  /* 50 baud */
#define OSAL_B75      0000002  /* 75 baud */
#define OSAL_B110     0000003  /* 110 baud */
#define OSAL_B134     0000004  /* 134.5 baud */
#define OSAL_B150     0000005  /* 150 baud */
#define OSAL_B200     0000006  /* 200 baud */
#define OSAL_B300     0000007  /* 300 baud */
#define OSAL_B600     0000010  /* 600 baud */
#define OSAL_B1200    0000011  /* 1200 baud */
#define OSAL_B1800    0000012  /* 1800 baud */
#define OSAL_B2400    0000013  /* 2400 baud */
#define OSAL_B4800    0000014  /* 4800 baud */
#define OSAL_B9600    0000015  /* 9600 baud */
#define OSAL_B19200   0000016  /* 19200 baud */
#define OSAL_B38400   0000017  /* 38400 baud */
#define OSAL_B57600   0010001  /* 57600 baud */
#define OSAL_B115200  0010002  /* 115200 baud */
#define OSAL_B230400  0010003  /* 230400 baud */
#define OSAL_B460800  0010004  /* 460800 baud */
#define OSAL_B500000  0010005  /* 500000 baud */
#define OSAL_B576000  0010006  /* 576000 baud */
#define OSAL_B921600  0010007  /* 921600 baud */
#define OSAL_B1000000 0010010  /* 1000000 baud */
#define OSAL_B1152000 0010011  /* 1152000 baud */
#define OSAL_B1500000 0010012  /* 1500000 baud */
#define OSAL_B2000000 0010013  /* 2000000 baud */
#define OSAL_B2500000 0010014  /* 2500000 baud */
#define OSAL_B3000000 0010015  /* 3000000 baud */
#define OSAL_B3500000 0010016  /* 3500000 baud */
#define OSAL_B4000000 0010017  /* 4000000 baud */

/*===========================================================================
 * termios函数
 *===========================================================================*/

/**
 * @brief 获取终端属性
 * @param fd 文件描述符
 * @param termios_p termios结构指针
 * @return 0成功，-1失败
 */
int32_t OSAL_tcgetattr(int32_t fd, osal_termios_t *termios_p);

/**
 * @brief 设置终端属性
 * @param fd 文件描述符
 * @param optional_actions 可选操作（OSAL_TCSANOW/TCSADRAIN/TCSAFLUSH）
 * @param termios_p termios结构指针
 * @return 0成功，-1失败
 */
int32_t OSAL_tcsetattr(int32_t fd, int32_t optional_actions, const osal_termios_t *termios_p);

/**
 * @brief 刷新输入/输出队列
 * @param fd 文件描述符
 * @param queue_selector 队列选择器（OSAL_TCIFLUSH/TCOFLUSH/TCIOFLUSH）
 * @return 0成功，-1失败
 */
int32_t OSAL_tcflush(int32_t fd, int32_t queue_selector);

/**
 * @brief 挂起/恢复传输
 * @param fd 文件描述符
 * @param action 操作（OSAL_TCOOFF/TCOON/TCIOFF/TCION）
 * @return 0成功，-1失败
 */
int32_t OSAL_tcflow(int32_t fd, int32_t action);

/**
 * @brief 发送BREAK
 * @param fd 文件描述符
 * @param duration 持续时间（0表示0.25-0.5秒）
 * @return 0成功，-1失败
 */
int32_t OSAL_tcsendbreak(int32_t fd, int32_t duration);

/**
 * @brief 等待输出完成
 * @param fd 文件描述符
 * @return 0成功，-1失败
 */
int32_t OSAL_tcdrain(int32_t fd);

/**
 * @brief 设置输入波特率
 * @param termios_p termios结构指针
 * @param speed 波特率（OSAL_B*）
 * @return 0成功，-1失败
 */
int32_t OSAL_cfsetispeed(osal_termios_t *termios_p, uint32_t speed);

/**
 * @brief 设置输出波特率
 * @param termios_p termios结构指针
 * @param speed 波特率（OSAL_B*）
 * @return 0成功，-1失败
 */
int32_t OSAL_cfsetospeed(osal_termios_t *termios_p, uint32_t speed);

/**
 * @brief 获取输入波特率
 * @param termios_p termios结构指针
 * @return 波特率（OSAL_B*）
 */
uint32_t OSAL_cfgetispeed(const osal_termios_t *termios_p);

/**
 * @brief 获取输出波特率
 * @param termios_p termios结构指针
 * @return 波特率（OSAL_B*）
 */
uint32_t OSAL_cfgetospeed(const osal_termios_t *termios_p);

#endif /* OSAL_TERMIOS_H */
