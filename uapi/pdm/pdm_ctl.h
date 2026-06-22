/*
 * PDM control UAPI
 *
 * This header defines the ioctl ABI shared by /dev/pdm_ctl and userspace
 * device-discovery clients. PDM ABI v1 is snapshot-based and does not expose
 * asynchronous userspace device events.
 */

#ifndef PDM_CTL_UAPI_H
#define PDM_CTL_UAPI_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define PDM_CTL_ABI_VERSION 0x00010000U
#define PDM_CTL_DEVICE_NAME "pdm_ctl"
#define PDM_CTL_NAME_LEN 64U

enum pdm_ctl_device_type {
	PDM_CTL_DEVICE_TYPE_INVALID = 0x00,
	PDM_CTL_DEVICE_TYPE_MCU = 0x01,
	PDM_CTL_DEVICE_TYPE_LED = 0x02,
};

enum pdm_ctl_device_state {
	PDM_CTL_DEVICE_STATE_REGISTERED = 0,
	PDM_CTL_DEVICE_STATE_BOUND = 1,
	PDM_CTL_DEVICE_STATE_ERROR = 2,
};

#define PDM_CTL_DEVICE_CAP_NONE           0ULL
#define PDM_CTL_DEVICE_CAP_TRANSPORT_CAN  (1ULL << 0)
#define PDM_CTL_DEVICE_CAP_TRANSPORT_UART (1ULL << 1)
#define PDM_CTL_DEVICE_CAP_TRANSPORT_I2C  (1ULL << 2)
#define PDM_CTL_DEVICE_CAP_TRANSPORT_SPI  (1ULL << 3)
#define PDM_CTL_DEVICE_CAP_CONTROL_GPIO   (1ULL << 8)
#define PDM_CTL_DEVICE_CAP_CONTROL_PWM    (1ULL << 9)
#define PDM_CTL_DEVICE_CAP_USER_IOCTL     (1ULL << 16)
#define PDM_CTL_DEVICE_CAP_DEBUGFS        (1ULL << 17)

struct pdm_ctl_info {
	__u32 abi_version;
	__u32 module_version_major;
	__u32 module_version_minor;
	__u32 module_version_patch;
	__u32 open_count;
	__u32 device_count;
};

struct pdm_ctl_device_info {
	__u32 type;
	__u32 index;
	__u32 state;
	__s32 last_error;
	__u32 error_count;
	__u64 capabilities;
	char name[PDM_CTL_NAME_LEN];
	char driver_name[PDM_CTL_NAME_LEN];
};

struct pdm_ctl_device_query {
	__u32 match_index;
	__u32 reserved;
	__u64 required_capabilities;
	struct pdm_ctl_device_info info;
};

struct pdm_ctl_device_name_query {
	char name[PDM_CTL_NAME_LEN];
	struct pdm_ctl_device_info info;
};

#define PDM_CTL_IOC_MAGIC 'C'
#define PDM_CTL_IOC_GET_INFO \
	_IOR(PDM_CTL_IOC_MAGIC, 0x01, struct pdm_ctl_info)
#define PDM_CTL_IOC_GET_DEVICE \
	_IOWR(PDM_CTL_IOC_MAGIC, 0x02, struct pdm_ctl_device_query)
#define PDM_CTL_IOC_GET_DEVICE_BY_NAME \
	_IOWR(PDM_CTL_IOC_MAGIC, 0x03, struct pdm_ctl_device_name_query)
#define PDM_CTL_IOC_GET_DEVICE_BY_CAPABILITY \
	_IOWR(PDM_CTL_IOC_MAGIC, 0x04, struct pdm_ctl_device_query)

#endif /* PDM_CTL_UAPI_H */
