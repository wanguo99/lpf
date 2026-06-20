/************************************************************************
 * MCU外设驱动内部头文件
 *
 * 职责：
 * - 定义内部数据结构
 * - 声明内部接口（CAN/串口通信层）
 ************************************************************************/

#ifndef LPF_MCU_INTERNAL_H
#define LPF_MCU_INTERNAL_H

#include "osal.h"
#include "lpf/lpf_core.h"
#include "lpf/config/lpf_config.h"
#include "lpf/lpf_mcu_service.h"
#include "lpf/transport/mcu/lpf_mcu_transport.h"

#ifndef CONFIG_LPF_MCU_MAX_DEVICES
#define CONFIG_LPF_MCU_MAX_DEVICES 4
#endif

#define LPF_MCU_MAX_DEVICES CONFIG_LPF_MCU_MAX_DEVICES

typedef struct {
	bool present;
	char name[64];
	lpf_config_mcu_interface_t interface;
	uint32_t cmd_timeout_ms;
} lpf_mcu_debug_info_t;

int32_t lpf_mcu_probe(const lpf_device_t *device);
void lpf_mcu_remove(const lpf_device_t *device);
lpf_mcu_handle_t lpf_mcu_get(uint32_t index);
int32_t lpf_mcu_debug_get(uint32_t index, lpf_mcu_debug_info_t *info);

int lpf_mcu_chrdev_register(void);
void lpf_mcu_chrdev_unregister(void);
int lpf_mcu_chrdev_register_device(const lpf_device_t *device);
void lpf_mcu_chrdev_unregister_device(const lpf_device_t *device);
void lpf_mcu_chrdev_record_error(uint32_t index, int error);
void lpf_mcu_chrdev_record_recovery(uint32_t index);
void lpf_mcu_chrdev_record_status(uint32_t index,
				  const lpf_mcu_status_t *status);
int lpf_mcu_proc_register(void);
void lpf_mcu_proc_unregister(void);
int lpf_mcu_debugfs_register(void);
void lpf_mcu_debugfs_unregister(void);

#endif /* LPF_MCU_INTERNAL_H */
