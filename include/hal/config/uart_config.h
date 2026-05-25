/************************************************************************
 * UART配置
 ************************************************************************/

#ifndef UART_CONFIG_H
#define UART_CONFIG_H

/* UART设备 */
#define UART_DEVICE             "/dev/ttyS0"

/* UART波特率 */
#define UART_BAUDRATE           115200

/* UART数据位 */
#define UART_DATABITS           8

/* UART停止位 */
#define UART_STOPBITS           1

/* UART校验位 */
#define UART_PARITY             0  /* 0=无, 1=奇, 2=偶 */

/* UART超时时间(ms) */
#define UART_TIMEOUT_MS         2000

#endif /* UART_CONFIG_H */
