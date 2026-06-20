# HAL

HAL is the kernel-mode Hardware Abstraction Layer used by LPF peripheral
services.

## Scope

- Builds as `hal.ko` through `make modules`.
- Exports kernel symbols for the public HAL C API under
  `kernel/include/hal/`.
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
CONFIG_HAL=y
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
_build/modules/hal.ko
_build/modules/pdm.ko
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

LPF peripheral services include HAL headers and call exported HAL symbols. HAL
owns hardware capability APIs. New HAL paths should call LPF SoC Adapter APIs
instead of calling Linux or vendor BSP APIs directly. Current HAL CAN, serial,
GPIO, PWM, I2C, and SPI implementations all follow this rule.
