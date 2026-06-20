// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_HW_INTERNAL_H
#define LPF_HW_INTERNAL_H

#define LPF_HW_BUILTIN_DRIVER_SECTION ".lpf_hw_builtin_driver"

typedef struct {
	const char *name;
	int (*init)(void);
	void (*exit)(void);
} lpf_hw_builtin_driver_t;

#define LPF_HW_BUILTIN_DRIVER(_id, _init, _exit)                         \
	static const lpf_hw_builtin_driver_t __lpf_hw_builtin_driver_##_id \
		__attribute__((used, aligned(sizeof(void *)),           \
			       section(LPF_HW_BUILTIN_DRIVER_SECTION))) = { \
			.name = #_id,                                  \
			.init = _init,                                 \
			.exit = _exit,                                 \
		}

extern const lpf_hw_builtin_driver_t lpf_hw_builtin_driver_start;
extern const lpf_hw_builtin_driver_t lpf_hw_builtin_driver_end;

#endif /* LPF_HW_INTERNAL_H */
