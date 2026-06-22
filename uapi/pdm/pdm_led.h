/*
 * LPF LED UAPI
 */

#ifndef LPF_LED_UAPI_H
#define LPF_LED_UAPI_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define LPF_LED_ABI_VERSION 0x00010000U
#define LPF_LED_DEVICE_NAME "lpf_led"

struct lpf_led_info {
	__u32 abi_version;
	__u32 module_version_major;
	__u32 module_version_minor;
	__u32 module_version_patch;
	__u32 open_count;
	__u32 max_devices;
};

struct lpf_led_state {
	__u32 index;
	__u32 brightness;
	__u32 max_brightness;
	__u32 enabled;
};

struct lpf_led_brightness {
	__u32 index;
	__u32 brightness;
};

#define LPF_LED_IOC_MAGIC 'L'
#define LPF_LED_IOC_GET_INFO \
	_IOR(LPF_LED_IOC_MAGIC, 0x01, struct lpf_led_info)
#define LPF_LED_IOC_GET_STATE \
	_IOWR(LPF_LED_IOC_MAGIC, 0x02, struct lpf_led_state)
#define LPF_LED_IOC_SET_BRIGHTNESS \
	_IOW(LPF_LED_IOC_MAGIC, 0x03, struct lpf_led_brightness)
#define LPF_LED_IOC_ENABLE _IOW(LPF_LED_IOC_MAGIC, 0x04, __u32)
#define LPF_LED_IOC_DISABLE _IOW(LPF_LED_IOC_MAGIC, 0x05, __u32)

#endif /* LPF_LED_UAPI_H */
