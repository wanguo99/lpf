/*
 * ES-Middleware PDI UAPI
 *
 * This header defines the ioctl ABI shared by the kernel module and
 * userspace client library.
 */

#ifndef PDI_IOCTL_H
#define PDI_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define PDI_ABI_VERSION 0x00010000U
#define PDI_DEVICE_NAME_LEN 32

struct pdi_info {
	__u32 abi_version;
	__u32 module_version_major;
	__u32 module_version_minor;
	__u32 module_version_patch;
	__u32 open_count;
	__u32 echo_enabled;
	char device_name[PDI_DEVICE_NAME_LEN];
};

#define PDI_IOC_MAGIC 'P'
#define PDI_IOC_GET_INFO _IOR(PDI_IOC_MAGIC, 0x01, struct pdi_info)
#define PDI_IOC_SET_ECHO _IOW(PDI_IOC_MAGIC, 0x02, __u32)
#define PDI_IOC_GET_ECHO _IOR(PDI_IOC_MAGIC, 0x03, __u32)
#define PDI_IOC_PING _IO(PDI_IOC_MAGIC, 0x04)

#endif /* PDI_IOCTL_H */
