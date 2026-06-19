# PDI

PDI is the userspace API library for kernel PDM peripheral devices.

Responsibilities:

- Own application-facing C APIs by peripheral type.
- Open and close peripheral-specific PDM device nodes.
- Marshal requests through each peripheral's UAPI ioctl commands.
- Hide ioctl details from applications.

Current peripheral APIs:

- MCU: `pdi_mcu_*` wraps `/dev/pdm_mcu`; ioctl ABI lives in
  `uapi/pdi/pdi_mcu.h`.
- LED: `pdi_led_*` wraps `/dev/pdm_led`; ioctl ABI lives in
  `uapi/pdi/pdi_led.h`.

UAPI and ABI rules for new peripherals are documented in
`docs/PDI_UAPI_ABI.md`.

PDI must not reimplement kernel HAL, PConfig, or PDM business logic.
