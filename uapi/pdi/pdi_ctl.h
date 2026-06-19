/*
 * LPF PDI control UAPI
 *
 * This header defines the ioctl ABI shared by /dev/pdm_ctl and userspace
 * device-discovery clients.
 */

#ifndef PDI_CTL_UAPI_H
#define PDI_CTL_UAPI_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define PDI_CTL_ABI_VERSION 0x00010000U
#define PDI_CTL_DEVICE_NAME "pdm_ctl"
#define PDI_CTL_DEFAULT_DEVICE "/dev/pdm_ctl"
#define PDI_CTL_NAME_LEN 64U

enum pdi_ctl_device_type {
	PDI_CTL_DEVICE_TYPE_INVALID = 0x00,
	PDI_CTL_DEVICE_TYPE_MCU = 0x01,
	PDI_CTL_DEVICE_TYPE_LED = 0x02,
};

enum pdi_ctl_device_state {
	PDI_CTL_DEVICE_STATE_REGISTERED = 0,
	PDI_CTL_DEVICE_STATE_BOUND = 1,
	PDI_CTL_DEVICE_STATE_ERROR = 2,
};

#define PDI_CTL_DEVICE_CAP_NONE           0ULL
#define PDI_CTL_DEVICE_CAP_TRANSPORT_CAN  (1ULL << 0)
#define PDI_CTL_DEVICE_CAP_TRANSPORT_UART (1ULL << 1)
#define PDI_CTL_DEVICE_CAP_CONTROL_GPIO   (1ULL << 8)
#define PDI_CTL_DEVICE_CAP_CONTROL_PWM    (1ULL << 9)
#define PDI_CTL_DEVICE_CAP_USER_IOCTL     (1ULL << 16)
#define PDI_CTL_DEVICE_CAP_DEBUG_PROCFS   (1ULL << 17)

struct pdi_ctl_info {
	__u32 abi_version;
	__u32 module_version_major;
	__u32 module_version_minor;
	__u32 module_version_patch;
	__u32 open_count;
	__u32 device_count;
};

struct pdi_ctl_device_info {
	__u32 type;
	__u32 index;
	__u32 state;
	__s32 last_error;
	__u64 capabilities;
	char name[PDI_CTL_NAME_LEN];
	char driver_name[PDI_CTL_NAME_LEN];
};

struct pdi_ctl_device_query {
	__u32 match_index;
	__u32 reserved;
	__u64 required_capabilities;
	struct pdi_ctl_device_info info;
};

struct pdi_ctl_device_name_query {
	char name[PDI_CTL_NAME_LEN];
	struct pdi_ctl_device_info info;
};

#define PDI_CTL_IOC_MAGIC 'C'
#define PDI_CTL_IOC_GET_INFO \
	_IOR(PDI_CTL_IOC_MAGIC, 0x01, struct pdi_ctl_info)
#define PDI_CTL_IOC_GET_DEVICE \
	_IOWR(PDI_CTL_IOC_MAGIC, 0x02, struct pdi_ctl_device_query)
#define PDI_CTL_IOC_GET_DEVICE_BY_NAME \
	_IOWR(PDI_CTL_IOC_MAGIC, 0x03, struct pdi_ctl_device_name_query)
#define PDI_CTL_IOC_GET_DEVICE_BY_CAPABILITY \
	_IOWR(PDI_CTL_IOC_MAGIC, 0x04, struct pdi_ctl_device_query)

#endif /* PDI_CTL_UAPI_H */
