# PDI

PDI is the userspace API library for kernel PDM peripheral devices.

Responsibilities:

- Own application-facing C APIs by peripheral type.
- Open and close peripheral-specific PDM device nodes.
- Marshal requests through each peripheral's UAPI ioctl commands.
- Hide ioctl details from applications.
- Discover configured LPF devices through the PDM control node.

Current peripheral APIs:

- Discovery: `pdi_ctl_*`, `pdi_list_devices`, and lookup helpers wrap
  `/dev/pdm_ctl`; ioctl ABI lives in `uapi/lpf/lpf_ctl.h`.
- MCU: `pdi_mcu_*` wraps `/dev/pdm_mcu`; ioctl ABI lives in
  `uapi/lpf/lpf_mcu.h`; SDK declarations live in `pdi/mcu.h`.
- LED: `pdi_led_*` wraps `/dev/pdm_led`; ioctl ABI lives in
  `uapi/lpf/lpf_led.h`; SDK declarations live in `pdi/led.h`.

`pdi_mcu_open_by_name()` and `pdi_led_open_by_name()` use `/dev/pdm_ctl` to
validate the LPF stable device name before opening the current aggregated
peripheral node.

Applications should include `pdi/pdi.h` or the SDK headers under
`user/pdi/include/pdi/`. UAPI headers under `uapi/lpf/` are ABI-only.

All PDI APIs return `0` on success and `-1` on failure with `errno` set.
Internal validation maps null pointers to `EINVAL`, invalid or closed contexts
to `EBADF`, and stable-name type mismatches to `ENODEV`. System call failures
from `open`, `ioctl`, and `close` preserve the kernel/libc `errno` value.

UAPI and ABI rules for new peripherals are documented in
`docs/LPF_UAPI_ABI.md`.

PDI must not reimplement kernel HAL, PConfig, or PDM business logic.
