/************************************************************************
 * PDI user API
 *
 * PDI is the userspace interface library for the kernel PDM module.
 ************************************************************************/

#ifndef PDI_H
#define PDI_H

#include <stdint.h>

#include "pdi/pdi_ioctl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PDI_DEFAULT_DEVICE "/dev/es-middleware-pdm"

typedef struct {
	int fd;
} pdi_context_t;

int32_t pdi_open(pdi_context_t *ctx, const char *device_path);
int32_t pdi_close(pdi_context_t *ctx);
int32_t pdi_ping(pdi_context_t *ctx);
int32_t pdi_get_info(pdi_context_t *ctx, struct pdi_info *info);
int32_t pdi_get_echo(pdi_context_t *ctx, uint32_t *enabled);
int32_t pdi_set_echo(pdi_context_t *ctx, uint32_t enabled);

#ifdef __cplusplus
}
#endif

#endif /* PDI_H */
