/************************************************************************
 * PDI discovery user API
 ************************************************************************/

#ifndef PDI_DISCOVERY_H
#define PDI_DISCOVERY_H

#include "pdi/pdi_ctl.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int fd;
} pdi_ctl_context_t;

int32_t pdi_ctl_open(pdi_ctl_context_t *ctx, const char *device_path);
int32_t pdi_ctl_close(pdi_ctl_context_t *ctx);
int32_t pdi_ctl_get_info(pdi_ctl_context_t *ctx, struct pdi_ctl_info *info);
int32_t pdi_list_devices(pdi_ctl_context_t *ctx,
			 struct pdi_ctl_device_info *devices,
			 uint32_t *count);
int32_t pdi_get_device_by_name(pdi_ctl_context_t *ctx, const char *name,
			       struct pdi_ctl_device_info *info);
int32_t pdi_get_device_by_capability(pdi_ctl_context_t *ctx,
				     uint64_t required_capabilities,
				     uint32_t match_index,
				     struct pdi_ctl_device_info *info);

#ifdef __cplusplus
}
#endif

#endif /* PDI_DISCOVERY_H */
