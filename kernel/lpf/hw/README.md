# LPF HW

LPF HW is the framework-owned hardware access layer used by LPF peripheral
services and transports. It is linked into `lpf_peripheral_runtime.ko`; it does
not build as a standalone kernel module.

## Scope

- `lpf_hw_gpio`: GPIO request, direction, level, and interrupt helpers.
- `lpf_hw_pwm`: PWM acquire, apply, state, enable, and disable helpers.
- `lpf_hw_transport_can`: CAN transport open, send, receive, and filtering.
- `lpf_hw_transport_uart`: UART transport open, read, write, flush, and config.
- `lpf_hw_bus_i2c`: I2C open, read/write, register access, and transfer.
- `lpf_hw_bus_spi`: SPI open, read/write, transfer, and config.

All LPF HW implementations call the LPF SoC Adapter. Vendor BSP and
kernel-version conditionals belong below the SoC adapter or compat layer, not
in peripheral business code.

## Configuration

```text
CONFIG_LPF_HW_CAN=y
CONFIG_LPF_HW_UART=y
CONFIG_LPF_HW_GPIO=y
CONFIG_LPF_HW_PWM=y
CONFIG_LPF_HW_I2C=y
CONFIG_LPF_HW_SPI=y
```

The mock preset can also enable `CONFIG_LPF_HW_MOCK_SELFTEST=y`, which builds
`lpf_hw_mock_selftest.ko` for operation-path checks against the mock SoC
adapter.

## Build

```bash
make kernel_x86_modules_defconfig
make modules
```

Expected runtime artifacts:

```text
_build/modules/osal.ko
_build/modules/lpf_core.ko
_build/modules/lpf_peripheral_runtime.ko
```

`kernel_x86_mock_modules_defconfig` additionally builds
`_build/modules/lpf_hw_mock_selftest.ko`.
