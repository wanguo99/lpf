/*
 * LPF MCU UAPI
 *
 * This header defines the ioctl ABI shared by /dev/lpf/mcuN and the
 * userspace PDI MCU client.
 */

#ifndef LPF_MCU_UAPI_H
#define LPF_MCU_UAPI_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define LPF_MCU_ABI_VERSION 0x00010000U
#define LPF_MCU_DEVICE_NAME "lpf_mcu"
#define LPF_MCU_MAX_TRANSFER_SIZE 256U
#define LPF_MCU_MAX_WRITE_SIZE 248U

enum lpf_mcu_state {
	LPF_MCU_STATE_UNINITIALIZED = 0x00,
	LPF_MCU_STATE_INIT = 0x01,
	LPF_MCU_STATE_READY = 0x02,
	LPF_MCU_STATE_BUSY = 0x03,
	LPF_MCU_STATE_ERROR = 0x04,
	LPF_MCU_STATE_OFFLINE = 0x05,
};

struct lpf_mcu_info {
	__u32 abi_version;
	__u32 module_version_major;
	__u32 module_version_minor;
	__u32 module_version_patch;
	__u32 open_count;
	__u32 max_devices;
};

struct lpf_mcu_version {
	__u32 index;
	__u8 major;
	__u8 minor;
	__u8 patch;
	__u8 build;
	char version_string[32];
};

struct lpf_mcu_status {
	__u32 index;
	__u32 online;
	__u32 state;
	__u32 uptime_sec;
	__u32 error_code;
	__s32 temperature_milli_celsius;
	__u32 voltage_mv;
	__u64 timestamp_us;
};

struct lpf_mcu_command {
	__u32 index;
	__u32 command;
	__u32 tx_len;
	__u32 rx_len;
	__u8 tx_data[LPF_MCU_MAX_TRANSFER_SIZE];
	__u8 rx_data[LPF_MCU_MAX_TRANSFER_SIZE];
};

struct lpf_mcu_data {
	__u32 index;
	__u32 address;
	__u32 len;
	__u8 data[LPF_MCU_MAX_TRANSFER_SIZE];
};

#define LPF_MCU_IOC_MAGIC 'M'
#define LPF_MCU_IOC_GET_INFO \
	_IOR(LPF_MCU_IOC_MAGIC, 0x01, struct lpf_mcu_info)
#define LPF_MCU_IOC_GET_VERSION \
	_IOWR(LPF_MCU_IOC_MAGIC, 0x02, struct lpf_mcu_version)
#define LPF_MCU_IOC_GET_STATUS \
	_IOWR(LPF_MCU_IOC_MAGIC, 0x03, struct lpf_mcu_status)
#define LPF_MCU_IOC_RESET _IOW(LPF_MCU_IOC_MAGIC, 0x04, __u32)
#define LPF_MCU_IOC_COMMAND \
	_IOWR(LPF_MCU_IOC_MAGIC, 0x05, struct lpf_mcu_command)
#define LPF_MCU_IOC_READ_DATA \
	_IOWR(LPF_MCU_IOC_MAGIC, 0x06, struct lpf_mcu_data)
#define LPF_MCU_IOC_WRITE_DATA \
	_IOW(LPF_MCU_IOC_MAGIC, 0x07, struct lpf_mcu_data)

#endif /* LPF_MCU_UAPI_H */
