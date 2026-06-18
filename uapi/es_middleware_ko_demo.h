/*
 * ES-Middleware kernel demo UAPI
 *
 * This header defines the ioctl ABI for the kernel module demo.
 * It is intentionally small so it can later be reused by a userspace
 * client and by future PDL/kernel backends.
 */

#ifndef ES_MIDDLEWARE_KO_DEMO_H
#define ES_MIDDLEWARE_KO_DEMO_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define ESMW_KO_DEMO_ABI_VERSION 0x00010000U
#define ESMW_KO_DEMO_DEVICE_NAME_LEN 32

struct es_mw_ko_demo_info {
	__u32 abi_version;
	__u32 module_version_major;
	__u32 module_version_minor;
	__u32 module_version_patch;
	__u32 open_count;
	__u32 echo_enabled;
	char device_name[ESMW_KO_DEMO_DEVICE_NAME_LEN];
};

#define ESMW_KO_DEMO_IOC_MAGIC 'E'
#define ESMW_KO_DEMO_IOC_GET_INFO \
	_IOR(ESMW_KO_DEMO_IOC_MAGIC, 0x01, struct es_mw_ko_demo_info)
#define ESMW_KO_DEMO_IOC_SET_ECHO \
	_IOW(ESMW_KO_DEMO_IOC_MAGIC, 0x02, __u32)
#define ESMW_KO_DEMO_IOC_GET_ECHO \
	_IOR(ESMW_KO_DEMO_IOC_MAGIC, 0x03, __u32)
#define ESMW_KO_DEMO_IOC_PING _IO(ESMW_KO_DEMO_IOC_MAGIC, 0x04)

#endif /* ES_MIDDLEWARE_KO_DEMO_H */
