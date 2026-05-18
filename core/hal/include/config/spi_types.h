/************************************************************************
 * HAL层 - SPI类型定义
 *
 * 定义SPI硬件接口使用的基础数据类型
 ************************************************************************/

#ifndef SPI_TYPES_H
#define SPI_TYPES_H

#include "osal_types.h"

/*
 * ============================================================================
 * SPI传输结构（硬件层）
 * ============================================================================
 */

/* SPI模式定义 (兼容 Linux 内核定义) */
#ifndef SPI_MODE_0
#define SPI_MODE_0      0x00    /* CPOL=0, CPHA=0 */
#define SPI_MODE_1      0x01    /* CPOL=0, CPHA=1 */
#define SPI_MODE_2      0x02    /* CPOL=1, CPHA=0 */
#define SPI_MODE_3      0x03    /* CPOL=1, CPHA=1 */
#endif

/* SPI传输消息 */
typedef struct
{
    const uint8_t *tx_buf;      /* 发送缓冲区 (NULL表示只接收) */
    uint8_t       *rx_buf;      /* 接收缓冲区 (NULL表示只发送) */
    uint32_t       len;         /* 传输长度 */
    uint32_t       speed_hz;    /* 传输速率 (0表示使用默认) */
    uint16_t       delay_usecs; /* 传输后延迟(us) */
    uint8_t        bits_per_word; /* 每字位数 (0表示使用默认) */
    uint8_t        cs_change;   /* 传输后改变CS状态 */
} spi_transfer_t;

#endif /* SPI_TYPES_H */
