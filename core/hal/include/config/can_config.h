/************************************************************************
 * CAN硬件配置
 ************************************************************************/

#ifndef CAN_CONFIG_H
#define CAN_CONFIG_H

/* CAN接口 */
#define CAN_INTERFACE           "can0"

/* CAN波特率 */
#define CAN_BAUDRATE            500000  /* 500Kbps */

/* CAN接收队列深度 */
#define CAN_RX_QUEUE_DEPTH      100

/* CAN发送队列深度 */
#define CAN_TX_QUEUE_DEPTH      100

/* CAN消息最大长度 */
#define CAN_MAX_DATA_LEN        8

/* CAN超时时间(ms) */
#define CAN_TIMEOUT_MS          1000

#endif /* CAN_CONFIG_H */
