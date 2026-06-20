# Transitional HAL APIs

This directory currently contains transitional `hal_*` hardware access APIs used
by LPF peripheral services. The standalone HAL module boundary has been removed;
these objects are linked into `lpf_peripheral_runtime.ko`.

## Scope

- Builds as part of `lpf_peripheral_runtime.ko` through `make modules`.
- Keeps the public HAL C API under `kernel/include/hal/` until the next
  migration renames it to LPF HW APIs.
- Has no userspace HAL library or userspace HAL test product.

Current kernel implementations:

- CAN uses LPF SoC Adapter APIs.
- Serial uses LPF SoC Adapter APIs.
- GPIO uses LPF SoC Adapter APIs.
- PWM uses LPF SoC Adapter APIs.
- I2C uses LPF SoC Adapter APIs.
- SPI uses LPF SoC Adapter APIs.

## Configuration

```text
CONFIG_OSAL=y
CONFIG_LPF_CORE=y
CONFIG_LPF_PERIPHERAL_RUNTIME=y
CONFIG_HAL_CAN=y
CONFIG_HAL_UART=y
CONFIG_HAL_GPIO=y
CONFIG_HAL_PWM=y
CONFIG_HAL_I2C=y
CONFIG_HAL_SPI=y
```

## Build

```bash
make kernel_x86_modules_defconfig
make modules
```

Expected module artifacts include:

```text
_build/modules/osal.ko
_build/modules/lpf_core.ko
_build/modules/lpf_peripheral_runtime.ko
```

## Layout

```text
kernel/hal/
├── Config.in
├── Makefile
├── CMakeLists.txt      # header-only interface for host-side consumers
└── src/
    ├── hal.c
    ├── hal_can.c
    ├── hal_serial.c
    ├── hal_gpio.c
    ├── hal_pwm.c
    ├── hal_i2c.c
    └── hal_spi.c
```

## Layering

LPF peripheral services include HAL headers and call `hal_*` APIs as a
transition step. Those APIs own temporary hardware capability semantics and
call LPF SoC Adapter APIs instead of calling Linux or vendor BSP APIs directly.
Current HAL CAN, serial, GPIO, PWM, I2C, and SPI implementations all follow
this rule. New code should target the upcoming LPF HW API naming instead of
expanding the HAL naming surface.
