# PDI

PDI is the userspace API library for kernel LPF peripheral devices.

Responsibilities:

- Own application-facing C APIs by peripheral type.
- Open and close peripheral-specific LPF device nodes.
- Marshal requests through each peripheral's UAPI ioctl commands.
- Hide ioctl details from applications.
- Discover configured LPF devices through the LPF control node.

Current peripheral APIs:

- Discovery: `pdi_ctl_*`, `pdi_list_devices`, and lookup helpers wrap the LPF
  control node `/dev/pdm_ctl`; ioctl ABI lives in `uapi/lpf/lpf_ctl.h`; the
  SDK default path is `PDI_CTL_DEFAULT_DEVICE`.
- MCU: `pdi_mcu_*` wraps `/dev/lpf/mcuN`; ioctl ABI lives in
  `uapi/lpf/lpf_mcu.h`; SDK declarations and the default path
  `PDI_MCU_DEFAULT_DEVICE` live in `pdi/mcu.h`.
- LED: `pdi_led_*` wraps `/dev/lpf/ledN`; ioctl ABI lives in
  `uapi/lpf/lpf_led.h`; SDK declarations and the default path
  `PDI_LED_DEFAULT_DEVICE` live in `pdi/led.h`.

`pdi_mcu_open_by_name()` and `pdi_led_open_by_name()` use `/dev/pdm_ctl` to
validate the LPF stable device name, then open the matching instance node by
the discovered index.

Applications should include `pdi/pdi.h` or the SDK headers under
`user/pdi/include/pdi/`. UAPI headers under `uapi/lpf/` are ABI-only and should
not grow SDK contexts, helper functions, or default-open policy.

All PDI APIs return `0` on success and `-1` on failure with `errno` set.
Internal validation maps null pointers to `EINVAL`, invalid or closed contexts
to `EBADF`, and stable-name type mismatches to `ENODEV`. System call failures
from `open`, `ioctl`, and `close` preserve the kernel/libc `errno` value.

The PDI implementation routes `open`, `ioctl`, and `close` through an internal
syscall boundary. Production builds use libc directly, while tests can replace
that boundary to validate ioctl marshaling and operation paths without live LPF
device nodes.

UAPI and ABI rules for new peripherals are documented in
`docs/LPF_UAPI_ABI.md`.

PDI must not reimplement kernel LPF HW, LPF runtime config, or LPF peripheral service
logic.
