# LPF HW

LPF HW is the framework-owned hardware access layer used by LPF peripheral
services and service-owned transport backends. It is linked into
`lpf_runtime.ko`; it does not build as a standalone kernel module.

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

## Layout

```text
kernel/lpf-runtime/hw/
├── core/
│   ├── lpf_hw.c
│   ├── lpf_hw_builtin_start.c
│   └── lpf_hw_builtin_end.c
├── gpio/
│   └── lpf_hw_gpio.c
├── pwm/
│   └── lpf_hw_pwm.c
├── i2c/
│   └── lpf_hw_bus_i2c.c
├── spi/
│   └── lpf_hw_bus_spi.c
├── can/
│   └── lpf_hw_transport_can.c
└── uart/
    └── lpf_hw_transport_uart.c
```

The public LPF HW API remains under `kernel/include/lpf/hw/`. Private HW
registration helpers such as `lpf_hw_internal.h` live under
`kernel/lpf-runtime/include/` and are included directly by runtime-owned source
files.

## Configuration

```text
CONFIG_LPF_HW_CAN=y
CONFIG_LPF_HW_UART=y
CONFIG_LPF_HW_GPIO=y
CONFIG_LPF_HW_PWM=y
CONFIG_LPF_HW_I2C=y
CONFIG_LPF_HW_SPI=y
```

The mock preset can also enable `CONFIG_LPF_HW_MOCK_SELFTEST=m`, which builds
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
_build/modules/lpf_runtime.ko
```

`kernel_x86_mock_modules_defconfig` additionally builds
`_build/modules/lpf_hw_mock_selftest.ko` and
`_build/modules/lpf_dummy_service_selftest.ko`.

## Mock Module Smoke Test

The mock preset can run a load/unload smoke path that exercises the mock SoC
backend, LPF HW self-test module, and LPF Core dummy service self-test module:

```bash
make kernel_x86_mock_modules_defconfig
make modules
sudo make mock-modules-smoke
```

The smoke target loads `osal.ko`, `lpf_core.ko`,
`lpf_runtime.ko`, `lpf_hw_mock_selftest.ko`, and
`lpf_dummy_service_selftest.ko` in order, then unloads them in reverse order.
It refuses to run if any target module is already loaded, so it does not take
ownership of modules started by another test or by a developer shell.
