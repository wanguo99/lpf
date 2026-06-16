/************************************************************************
 * HAL层 - 串口硬件抽象层API
 *
 * 提供统一的串口访问接口（支持UART/RS232/RS485）
 ************************************************************************/

#ifndef HAL_SERIAL_H
#define HAL_SERIAL_H


#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 类型定义
 *===========================================================================*/

/**
 * @brief 串口设备句柄（不透明指针）
 */
typedef void* hal_serial_handle_t;

/**
 * @brief 串口配置结构
 */
typedef struct
{
	uint32_t baud_rate;     /* 波特率（如9600, 115200） */
	uint8_t  data_bits;     /* 数据位（5, 6, 7, 8） */
	uint8_t  stop_bits;     /* 停止位（1, 2） */
	uint8_t  parity;        /* 校验位（见下面的宏定义） */
	uint8_t  flow_control;  /* 流控制（见下面的宏定义） */
} hal_serial_config_t;

/*===========================================================================
 * 校验位定义
 *===========================================================================*/

#define HAL_SERIAL_PARITY_NONE  0x00  /* 无校验 */
#define HAL_SERIAL_PARITY_ODD   0x01  /* 奇校验 */
#define HAL_SERIAL_PARITY_EVEN  0x02  /* 偶校验 */

/*===========================================================================
 * 流控制定义
 *===========================================================================*/

#define HAL_SERIAL_FLOW_NONE    0x00  /* 无流控 */
#define HAL_SERIAL_FLOW_HW      0x01  /* 硬件流控（RTS/CTS） */
#define HAL_SERIAL_FLOW_SW      0x02  /* 软件流控（XON/XOFF） */

/*===========================================================================
 * API函数
 *===========================================================================*/

/**
 * @brief 打开串口设备
 *
 * 打开指定的串口设备并配置参数。
 *
 * @param[in]  device  设备路径（如 "/dev/ttyS0", "/dev/ttyUSB0"）
 * @param[in]  config  串口配置参数
 * @param[out] handle  返回的串口句柄
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_NO_DEVICE     设备不存在
 * @return OSAL_ERR_GENERIC       打开失败（如权限不足、设备忙）
 *
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_SERIAL_open(const char *device, const hal_serial_config_t *config,
                        hal_serial_handle_t *handle);

/**
 * @brief 关闭串口设备
 *
 * 释放串口资源，关闭设备文件。
 *
 * @param[in] handle 串口句柄
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 句柄无效
 *
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_SERIAL_close(hal_serial_handle_t handle);

/**
 * @brief 向串口写入数据
 *
 * 发送数据到串口。
 *
 * @param[in] handle  串口句柄
 * @param[in] buffer  数据缓冲区
 * @param[in] size    数据长度（字节）
 * @param[in] timeout 超时时间（ms）
 *                    -1 = 阻塞模式（永久等待）
 *                     0 = 非阻塞模式（立即返回）
 *                    >0 = 超时等待（指定毫秒数）
 *
 * @return >= 0                   实际写入的字节数
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_TIMEOUT       超时
 * @return OSAL_ERR_GENERIC       写入失败
 *
 * @note 返回值可能小于size（部分写入），需要检查返回值
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_SERIAL_write(hal_serial_handle_t handle, const void *buffer,
                         uint32_t size, int32_t timeout);

/**
 * @brief 从串口读取数据
 *
 * 从串口接收数据。
 *
 * @param[in]  handle  串口句柄
 * @param[out] buffer  数据缓冲区
 * @param[in]  size    缓冲区大小（字节）
 * @param[in]  timeout 超时时间（ms）
 *                     -1 = 阻塞模式（永久等待）
 *                      0 = 非阻塞模式（立即返回）
 *                     >0 = 超时等待（指定毫秒数）
 *
 * @return >= 0                   实际读取的字节数
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_TIMEOUT       超时
 * @return OSAL_ERR_GENERIC       读取失败
 *
 * @note 返回值可能小于size（部分读取），需要检查返回值
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_SERIAL_read(hal_serial_handle_t handle, void *buffer,
                        uint32_t size, int32_t timeout);

/**
 * @brief 刷新串口缓冲区
 *
 * 清空输入和输出缓冲区中的数据。
 *
 * @param[in] handle 串口句柄
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 句柄无效
 * @return OSAL_ERR_GENERIC       刷新失败
 *
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_SERIAL_flush(hal_serial_handle_t handle);

/**
 * @brief 设置串口配置
 *
 * 动态修改串口参数（波特率、数据位、校验等）。
 *
 * @param[in] handle 串口句柄
 * @param[in] config 新的串口配置
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC       配置失败
 *
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_SERIAL_set_config(hal_serial_handle_t handle,
                              const hal_serial_config_t *config);

/**
 * @brief 串口测试调用（调试接口）
 *
 * @param[in] handle 串口句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 *
 * @note 预留的调试接口，用于验证 HAL 层调用链
 */
int32_t HAL_SERIAL_test_call(hal_serial_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* HAL_SERIAL_H */
