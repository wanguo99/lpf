/************************************************************************
 * PDI LED user API
 ************************************************************************/

#ifndef PDI_LED_API_H
#define PDI_LED_API_H

#include "lpf/lpf_led.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PDI_LED_DEFAULT_DEVICE "/dev/lpf/led0"

typedef struct {
	int fd;
} pdi_led_context_t;

int32_t pdi_led_open(pdi_led_context_t *ctx, const char *device_path);
int32_t pdi_led_open_by_name(pdi_led_context_t *ctx, const char *name);
int32_t pdi_led_close(pdi_led_context_t *ctx);
int32_t pdi_led_get_info(pdi_led_context_t *ctx, struct lpf_led_info *info);
int32_t pdi_led_get_state(pdi_led_context_t *ctx,
			  struct lpf_led_state *state);
int32_t pdi_led_set_brightness(pdi_led_context_t *ctx, uint32_t index,
			       uint32_t brightness);
int32_t pdi_led_enable(pdi_led_context_t *ctx, uint32_t index);
int32_t pdi_led_disable(pdi_led_context_t *ctx, uint32_t index);

#ifdef __cplusplus
}
#endif

#endif /* PDI_LED_API_H */
