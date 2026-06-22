// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_DEVICE_H
#define LPF_DEVICE_H

#include "osal.h"

#define LPF_DEVICE_NAME_LEN 64U

#define LPF_DEVICE_CAP_NONE          0ULL
#define LPF_DEVICE_CAP_TRANSPORT_CAN (1ULL << 0)
#define LPF_DEVICE_CAP_TRANSPORT_UART (1ULL << 1)
#define LPF_DEVICE_CAP_CONTROL_GPIO  (1ULL << 8)
#define LPF_DEVICE_CAP_CONTROL_PWM   (1ULL << 9)
#define LPF_DEVICE_CAP_USER_IOCTL    (1ULL << 16)
#define LPF_DEVICE_CAP_DEBUGFS       (1ULL << 17)

typedef enum {
	LPF_DEVICE_TYPE_INVALID = 0x00,
	LPF_DEVICE_TYPE_MCU = 0x01,
	LPF_DEVICE_TYPE_LED = 0x02,
	LPF_DEVICE_TYPE_DUMMY = 0x7F,
} lpf_device_type_t;

typedef enum {
	LPF_DEVICE_STATE_REGISTERED = 0,
	LPF_DEVICE_STATE_BOUND,
	LPF_DEVICE_STATE_ERROR,
} lpf_device_state_t;

typedef uint64_t lpf_capability_t;

typedef struct {
	/* Match key used to find the registered LPF driver. */
	lpf_device_type_t type;
	/* Stable instance index within one LPF device type. */
	uint32_t index;
	/* Typed configuration payload owned by the runtime config driver. */
	const void *entry;
	/* Optional configured name; Core falls back to driver name + index. */
	const char *name;
	lpf_capability_t capabilities;
} lpf_device_config_t;

struct lpf_driver;

typedef struct lpf_device {
	lpf_device_config_t config;
	const struct lpf_driver *driver;
	lpf_device_state_t state;
	int32_t last_error;
	uint32_t error_count;
	lpf_capability_t capabilities;
	char name[LPF_DEVICE_NAME_LEN];
} lpf_device_t;

typedef struct {
	lpf_device_type_t type;
	uint32_t index;
	lpf_device_state_t state;
	int32_t last_error;
	uint32_t error_count;
	lpf_capability_t capabilities;
	char name[LPF_DEVICE_NAME_LEN];
	char driver_name[LPF_DEVICE_NAME_LEN];
} lpf_device_info_t;

#endif /* LPF_DEVICE_H */
