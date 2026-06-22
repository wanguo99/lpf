// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_SOC_ADAPTER_H
#define LPF_SOC_ADAPTER_H

#include "lpf/types/lpf_can_types.h"
#include "lpf/types/lpf_gpio_types.h"
#include "lpf/types/lpf_i2c_types.h"
#include "lpf/types/lpf_pwm_types.h"
#include "lpf/types/lpf_serial_types.h"
#include "lpf/types/lpf_spi_types.h"

typedef struct {
	int32_t (*request)(uint32_t gpio_num, const char *label);
	void (*free)(uint32_t gpio_num);
	int32_t (*set_direction)(uint32_t gpio_num,
				 lpf_gpio_direction_t direction,
				 lpf_gpio_level_t initial_level);
	int32_t (*set_level)(uint32_t gpio_num, lpf_gpio_level_t level);
	int32_t (*get_level)(uint32_t gpio_num, lpf_gpio_level_t *level);
	int32_t (*request_interrupt)(uint32_t gpio_num, lpf_gpio_edge_t edge,
				     lpf_gpio_irq_callback_t callback,
				     void *user_data, void **irq_handle);
	void (*free_interrupt)(void *irq_handle);
	int32_t (*enable_interrupt)(void *irq_handle);
	int32_t (*disable_interrupt)(void *irq_handle);
} lpf_soc_gpio_ops_t;

typedef struct {
	int32_t (*get)(const char *consumer, lpf_pwm_handle_t *handle);
	void (*put)(lpf_pwm_handle_t handle);
	int32_t (*apply)(lpf_pwm_handle_t handle,
			 const lpf_pwm_state_t *state);
	int32_t (*get_state)(lpf_pwm_handle_t handle,
			     lpf_pwm_state_t *state);
	int32_t (*enable)(lpf_pwm_handle_t handle);
	int32_t (*disable)(lpf_pwm_handle_t handle);
} lpf_soc_pwm_ops_t;

typedef struct {
	int32_t (*open)(const char *device, lpf_i2c_handle_t *handle);
	void (*close)(lpf_i2c_handle_t handle);
	int32_t (*transfer)(lpf_i2c_handle_t handle, lpf_i2c_msg_t *msgs,
			    uint32_t num);
} lpf_soc_i2c_ops_t;

typedef struct {
	int32_t (*open)(const lpf_spi_config_t *config,
			lpf_spi_handle_t *handle);
	void (*close)(lpf_spi_handle_t handle);
	int32_t (*transfer)(lpf_spi_handle_t handle,
			    const lpf_spi_transfer_t *transfers,
			    uint32_t num);
	int32_t (*set_config)(lpf_spi_handle_t handle,
			      const lpf_spi_config_t *config);
} lpf_soc_spi_ops_t;

typedef struct {
	int32_t (*init)(const lpf_can_config_t *config,
			lpf_can_handle_t *handle);
	int32_t (*deinit)(lpf_can_handle_t handle);
	int32_t (*send)(lpf_can_handle_t handle,
			const lpf_can_frame_t *frame);
	int32_t (*recv)(lpf_can_handle_t handle, lpf_can_frame_t *frame,
			int32_t timeout);
	int32_t (*set_filter)(lpf_can_handle_t handle, uint32_t filter_id,
			      uint32_t filter_mask);
} lpf_soc_can_ops_t;

typedef struct {
	int32_t (*open)(const char *device, const lpf_serial_config_t *config,
			lpf_serial_handle_t *handle);
	int32_t (*close)(lpf_serial_handle_t handle);
	int32_t (*write)(lpf_serial_handle_t handle, const void *buffer,
			 uint32_t size, int32_t timeout);
	int32_t (*read)(lpf_serial_handle_t handle, void *buffer,
			uint32_t size, int32_t timeout);
	int32_t (*flush)(lpf_serial_handle_t handle);
	int32_t (*set_config)(lpf_serial_handle_t handle,
			      const lpf_serial_config_t *config);
} lpf_soc_serial_ops_t;

typedef struct {
	const char *name;
	lpf_soc_gpio_ops_t gpio;
	lpf_soc_pwm_ops_t pwm;
	lpf_soc_i2c_ops_t i2c;
	lpf_soc_spi_ops_t spi;
	lpf_soc_can_ops_t can;
	lpf_soc_serial_ops_t serial;
} lpf_soc_adapter_t;

int32_t lpf_soc_adapter_init(void);
void lpf_soc_adapter_deinit(void);
int32_t lpf_soc_adapter_register(const lpf_soc_adapter_t *adapter);
void lpf_soc_adapter_unregister(const lpf_soc_adapter_t *adapter);
const lpf_soc_adapter_t *lpf_soc_adapter_current(void);
const char *lpf_soc_adapter_name(void);

int32_t lpf_soc_gpio_request(uint32_t gpio_num, const char *label);
void lpf_soc_gpio_free(uint32_t gpio_num);
int32_t lpf_soc_gpio_set_direction(uint32_t gpio_num,
				   lpf_gpio_direction_t direction,
				   lpf_gpio_level_t initial_level);
int32_t lpf_soc_gpio_set_level(uint32_t gpio_num, lpf_gpio_level_t level);
int32_t lpf_soc_gpio_get_level(uint32_t gpio_num, lpf_gpio_level_t *level);
int32_t lpf_soc_gpio_request_interrupt(uint32_t gpio_num,
				       lpf_gpio_edge_t edge,
				       lpf_gpio_irq_callback_t callback,
				       void *user_data, void **irq_handle);
void lpf_soc_gpio_free_interrupt(void *irq_handle);
int32_t lpf_soc_gpio_enable_interrupt(void *irq_handle);
int32_t lpf_soc_gpio_disable_interrupt(void *irq_handle);

int32_t lpf_soc_pwm_get(const char *consumer, lpf_pwm_handle_t *handle);
void lpf_soc_pwm_put(lpf_pwm_handle_t handle);
int32_t lpf_soc_pwm_apply(lpf_pwm_handle_t handle,
			  const lpf_pwm_state_t *state);
int32_t lpf_soc_pwm_get_state(lpf_pwm_handle_t handle,
			      lpf_pwm_state_t *state);
int32_t lpf_soc_pwm_enable(lpf_pwm_handle_t handle);
int32_t lpf_soc_pwm_disable(lpf_pwm_handle_t handle);

int32_t lpf_soc_i2c_open(const char *device, lpf_i2c_handle_t *handle);
void lpf_soc_i2c_close(lpf_i2c_handle_t handle);
int32_t lpf_soc_i2c_transfer(lpf_i2c_handle_t handle,
			     lpf_i2c_msg_t *msgs, uint32_t num);

int32_t lpf_soc_spi_open(const lpf_spi_config_t *config,
			 lpf_spi_handle_t *handle);
void lpf_soc_spi_close(lpf_spi_handle_t handle);
int32_t lpf_soc_spi_transfer(lpf_spi_handle_t handle,
			     const lpf_spi_transfer_t *transfers,
			     uint32_t num);
int32_t lpf_soc_spi_set_config(lpf_spi_handle_t handle,
			       const lpf_spi_config_t *config);

int32_t lpf_soc_can_init(const lpf_can_config_t *config,
			 lpf_can_handle_t *handle);
int32_t lpf_soc_can_deinit(lpf_can_handle_t handle);
int32_t lpf_soc_can_send(lpf_can_handle_t handle,
			 const lpf_can_frame_t *frame);
int32_t lpf_soc_can_recv(lpf_can_handle_t handle, lpf_can_frame_t *frame,
			 int32_t timeout);
int32_t lpf_soc_can_set_filter(lpf_can_handle_t handle, uint32_t filter_id,
			       uint32_t filter_mask);

int32_t lpf_soc_serial_open(const char *device,
			    const lpf_serial_config_t *config,
			    lpf_serial_handle_t *handle);
int32_t lpf_soc_serial_close(lpf_serial_handle_t handle);
int32_t lpf_soc_serial_write(lpf_serial_handle_t handle, const void *buffer,
			     uint32_t size, int32_t timeout);
int32_t lpf_soc_serial_read(lpf_serial_handle_t handle, void *buffer,
			    uint32_t size, int32_t timeout);
int32_t lpf_soc_serial_flush(lpf_serial_handle_t handle);
int32_t lpf_soc_serial_set_config(lpf_serial_handle_t handle,
				  const lpf_serial_config_t *config);

#endif /* LPF_SOC_ADAPTER_H */
