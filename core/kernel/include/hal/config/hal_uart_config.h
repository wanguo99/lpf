/************************************************************************
 * UART配置
 ************************************************************************/

#ifndef HAL_UART_CONFIG_H
#define HAL_UART_CONFIG_H

/* UART设备 */
#define HAL_UART_DEVICE "/dev/ttyS0"

/* UART波特率 */
#define HAL_UART_BAUDRATE 0x1C200

/* UART数据位 */
#define HAL_UART_DATABITS 0x08

/* UART停止位 */
#define HAL_UART_STOPBITS 0x01

/* UART校验位 */
#define HAL_UART_PARITY 0x00 /* 0=无, 1=奇, 2=偶 */

/* UART超时时间(ms) */
#define HAL_UART_TIMEOUT_MS 0x7D0

#endif /* HAL_UART_CONFIG_H */
