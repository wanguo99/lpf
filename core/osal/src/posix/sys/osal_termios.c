/************************************************************************
 * OSAL - 终端I/O控制封装实现（POSIX）
 ************************************************************************/

#include <termios.h>
#include <string.h>
#include "sys/osal_termios.h"

/*===========================================================================
 * 波特率映射（OSAL -> POSIX）
 *===========================================================================*/

static speed_t osal_speed_to_posix(uint32_t osal_speed)
{
    switch (osal_speed) {
        case OSAL_B0:       return B0;
        case OSAL_B50:      return B50;
        case OSAL_B75:      return B75;
        case OSAL_B110:     return B110;
        case OSAL_B134:     return B134;
        case OSAL_B150:     return B150;
        case OSAL_B200:     return B200;
        case OSAL_B300:     return B300;
        case OSAL_B600:     return B600;
        case OSAL_B1200:    return B1200;
        case OSAL_B1800:    return B1800;
        case OSAL_B2400:    return B2400;
        case OSAL_B4800:    return B4800;
        case OSAL_B9600:    return B9600;
        case OSAL_B19200:   return B19200;
        case OSAL_B38400:   return B38400;
        case OSAL_B57600:   return B57600;
        case OSAL_B115200:  return B115200;
        case OSAL_B230400:  return B230400;
        case OSAL_B460800:  return B460800;
        case OSAL_B500000:  return B500000;
        case OSAL_B576000:  return B576000;
        case OSAL_B921600:  return B921600;
        case OSAL_B1000000: return B1000000;
        case OSAL_B1152000: return B1152000;
        case OSAL_B1500000: return B1500000;
        case OSAL_B2000000: return B2000000;
        case OSAL_B2500000: return B2500000;
        case OSAL_B3000000: return B3000000;
        case OSAL_B3500000: return B3500000;
        case OSAL_B4000000: return B4000000;
        default:            return B9600;  /* 默认波特率 */
    }
}

/*===========================================================================
 * 终端I/O控制操作
 *===========================================================================*/

int32_t OSAL_tcgetattr(int32_t fd, osal_termios_t *termios_p)
{
    struct termios *posix_termios = (struct termios *)termios_p->opaque;
    int32_t result = tcgetattr(fd, posix_termios);
    return result;
}

int32_t OSAL_tcsetattr(int32_t fd, int32_t optional_actions, const osal_termios_t *termios_p)
{
    const struct termios *posix_termios = (const struct termios *)termios_p->opaque;
    int32_t result = tcsetattr(fd, optional_actions, posix_termios);
    return result;
}

int32_t OSAL_tcflush(int32_t fd, int32_t queue_selector)
{
    int32_t result = tcflush(fd, queue_selector);
    return result;
}

int32_t OSAL_cfsetispeed(osal_termios_t *termios_p, uint32_t speed)
{
    struct termios *posix_termios = (struct termios *)termios_p->opaque;
    speed_t posix_speed = osal_speed_to_posix(speed);
    int32_t result = cfsetispeed(posix_termios, posix_speed);
    return result;
}

int32_t OSAL_cfsetospeed(osal_termios_t *termios_p, uint32_t speed)
{
    struct termios *posix_termios = (struct termios *)termios_p->opaque;
    speed_t posix_speed = osal_speed_to_posix(speed);
    int32_t result = cfsetospeed(posix_termios, posix_speed);
    return result;
}
