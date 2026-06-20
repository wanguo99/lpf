/************************************************************************
 * PDI MCU user API
 ************************************************************************/

#ifndef PDI_MCU_API_H
#define PDI_MCU_API_H

#include "lpf/lpf_mcu.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PDI_MCU_DEFAULT_DEVICE "/dev/lpf/mcu0"

typedef struct {
	int fd;
} pdi_mcu_context_t;

int32_t pdi_mcu_open(pdi_mcu_context_t *ctx, const char *device_path);
int32_t pdi_mcu_open_by_name(pdi_mcu_context_t *ctx, const char *name);
int32_t pdi_mcu_close(pdi_mcu_context_t *ctx);
int32_t pdi_mcu_get_info(pdi_mcu_context_t *ctx, struct lpf_mcu_info *info);
int32_t pdi_mcu_get_version(pdi_mcu_context_t *ctx,
			    struct lpf_mcu_version *version);
int32_t pdi_mcu_get_status(pdi_mcu_context_t *ctx,
			   struct lpf_mcu_status *status);
int32_t pdi_mcu_reset(pdi_mcu_context_t *ctx, uint32_t index);
int32_t pdi_mcu_command(pdi_mcu_context_t *ctx,
			struct lpf_mcu_command *command);
int32_t pdi_mcu_read_data(pdi_mcu_context_t *ctx,
			  struct lpf_mcu_data *data);
int32_t pdi_mcu_write_data(pdi_mcu_context_t *ctx,
			   const struct lpf_mcu_data *data);

#ifdef __cplusplus
}
#endif

#endif /* PDI_MCU_API_H */
